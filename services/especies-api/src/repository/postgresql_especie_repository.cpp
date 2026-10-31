#include "../../include/repository/postgresql_especie_repository.hpp"

#include <iostream>
#include <stdexcept>

#include "../../include/utils/timestamps.hpp"

namespace {

// Columnas seleccionadas en orden estable para reutilizar en SELECT,
// SELECT * y mapRowToEspecie. Excluye la lectura de imágenes legacy
// (especies_imagenes) que se hace aparte.
constexpr const char* kSelectCols =
    "id, reino, genero_id, nombre_cientifico, nombre_comun, autor_cientifico, "
    "descripcion, habitat, distribucion_chiloe, endemica, estado_conservacion, "
    "fuentes::text AS fuentes_text, geo_lat, geo_lng, "
    "atributos_especificos::text AS atributos_text, "
    "foto_portada_key, fotos_keys::text AS fotos_keys_text, "
    "fotos_creditos::text AS fotos_creditos_text, "
    "categoria_id, estado, publicado_por, fecha_publicacion, "
    "creado_por, revisado_por, "
    "fecha_revision, created_at, updated_at";

nlohmann::json parseJsonbText(const pqxx::field& f,
                              const nlohmann::json& fallback) {
    if (f.is_null()) return fallback;
    return nlohmann::json::parse(f.c_str());
}

std::optional<std::string> optStr(const pqxx::field& f) {
    if (f.is_null()) return std::nullopt;
    return std::string(f.c_str());
}

std::optional<int> optInt(const pqxx::field& f) {
    if (f.is_null()) return std::nullopt;
    return f.as<int>();
}

std::optional<double> optDouble(const pqxx::field& f) {
    if (f.is_null()) return std::nullopt;
    return f.as<double>();
}

}  // namespace

PostgreSQLEspecieRepository::PostgreSQLEspecieRepository(
    std::shared_ptr<Database> database)
    : database(database) {}

Especie PostgreSQLEspecieRepository::mapRowToEspecie(const pqxx::row& row) {
    Especie e;
    e.setId(row["id"].as<int>());
    e.setReino(reinoFromString(row["reino"].c_str()));
    e.setGeneroId(row["genero_id"].as<int>());
    e.setNombreCientifico(row["nombre_cientifico"].c_str());
    e.setNombreComun(row["nombre_comun"].is_null() ? "" : row["nombre_comun"].c_str());
    e.setAutorCientifico(
        row["autor_cientifico"].is_null() ? "" : row["autor_cientifico"].c_str());
    e.setDescripcion(row["descripcion"].is_null() ? "" : row["descripcion"].c_str());
    e.setHabitat(row["habitat"].is_null() ? "" : row["habitat"].c_str());
    e.setDistribucionChiloe(
        row["distribucion_chiloe"].is_null() ? "" : row["distribucion_chiloe"].c_str());
    e.setEndemica(row["endemica"].as<bool>());
    e.setEstadoConservacion(
        row["estado_conservacion"].is_null() ? "" : row["estado_conservacion"].c_str());

    e.setFuentes(parseJsonbText(row["fuentes_text"], nlohmann::json::array()));
    e.setGeoLat(optDouble(row["geo_lat"]));
    e.setGeoLng(optDouble(row["geo_lng"]));
    e.setAtributosEspecificos(
        parseJsonbText(row["atributos_text"], nlohmann::json::object()));
    e.setFotoPortadaKey(optStr(row["foto_portada_key"]));
    e.setFotosKeys(parseJsonbText(row["fotos_keys_text"], nlohmann::json::array()));
    e.setFotosCreditos(
        parseJsonbText(row["fotos_creditos_text"], nlohmann::json::object()));
    e.setCategoriaId(optInt(row["categoria_id"]));
    e.setEstado(especieEstadoFromString(row["estado"].c_str()));
    e.setPublicadoPor(optInt(row["publicado_por"]));
    e.setFechaPublicacion(utils::toIso8601Opt(optStr(row["fecha_publicacion"])));
    e.setCreadoPor(optInt(row["creado_por"]));
    e.setRevisadoPor(optInt(row["revisado_por"]));
    e.setFechaRevision(utils::toIso8601Opt(optStr(row["fecha_revision"])));
    e.setCreatedAt(utils::toIso8601Opt(optStr(row["created_at"])));
    e.setUpdatedAt(utils::toIso8601Opt(optStr(row["updated_at"])));

    // Imágenes legacy: tabla especies_imagenes. Se mantiene este PR.
    auto imagenes = getImagenes(e.getId());
    for (const auto& url : imagenes) {
        e.addImagenUrl(url);
    }

    return e;
}

namespace {

// Whitelist de columnas permitidas en ORDER BY. Cualquier otro valor
// hace fallar la búsqueda con std::invalid_argument antes de ejecutar
// SQL (evita inyección por concatenación).
bool isValidOrderBy(const std::string& col) {
    return col == "nombre_cientifico" || col == "nombre_comun"
        || col == "created_at" || col == "updated_at";
}

bool isValidOrderDir(const std::string& dir) {
    return dir == "asc" || dir == "desc";
}

}  // namespace

EspecieSearchResult PostgreSQLEspecieRepository::find(
    const EspecieFilters& filters) {
    if (!isValidOrderBy(filters.orderby)) {
        throw std::invalid_argument(
            "orderby inválido: " + filters.orderby
            + " (permitidos: nombre_cientifico, nombre_comun, created_at, updated_at)");
    }
    if (!isValidOrderDir(filters.orderdir)) {
        throw std::invalid_argument(
            "orderdir inválido: " + filters.orderdir + " (asc | desc)");
    }
    if (filters.limit < 1 || filters.limit > 200) {
        throw std::invalid_argument("limit debe estar en [1, 200]");
    }
    if (filters.offset < 0) {
        throw std::invalid_argument("offset debe ser >= 0");
    }

    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        // Construimos el WHERE inline usando txn.quote() para escapar cada
        // valor. pqxx::params + placeholders dinámicos no tiene soporte
        // directo en libpqxx 7.8 para queries no preparadas; quote() es
        // el camino seguro y compatible.
        std::string where = " WHERE 1=1";
        std::string joins;

        if (filters.reino) {
            where += " AND e.reino = " + txn.quote(reinoToString(*filters.reino))
                   + "::reino_enum";
        }
        if (filters.genero_id) {
            where += " AND e.genero_id = " + txn.quote(*filters.genero_id);
        }
        if (filters.familia_id) {
            joins += " JOIN generos g ON e.genero_id = g.id";
            where += " AND g.familia_id = " + txn.quote(*filters.familia_id);
        }
        if (filters.categoria_id) {
            where += " AND e.categoria_id = " + txn.quote(*filters.categoria_id);
        }
        if (filters.conservacion) {
            where += " AND e.estado_conservacion = "
                   + txn.quote(*filters.conservacion);
        }
        if (filters.endemica) {
            where += std::string(" AND e.endemica = ")
                   + (*filters.endemica ? "true" : "false");
        }
        // Visibilidad primero: los borradores no son contenido público, y el
        // resto de filtros nunca debe poder ensancharla.
        if (!filters.visibilidad.verTodo) {
            std::string visible = " AND (e.estado = 'publicada'";
            if (!filters.visibilidad.categoriasCuradas.empty()) {
                visible += " OR e.categoria_id IN (";
                for (size_t i = 0; i < filters.visibilidad.categoriasCuradas.size(); ++i) {
                    if (i > 0) visible += ", ";
                    visible += txn.quote(filters.visibilidad.categoriasCuradas[i]);
                }
                visible += ")";
            }
            where += visible + ")";
        }
        if (filters.estado) {
            where += " AND e.estado = "
                   + txn.quote(especieEstadoToString(*filters.estado))
                   + "::especie_estado_enum";
        }
        if (filters.q) {
            // ILIKE en nombre_comun y nombre_cientifico. El índice GIN
            // pg_trgm acelera este patrón.
            const auto needle = txn.quote("%" + *filters.q + "%");
            where += " AND (e.nombre_comun ILIKE " + needle
                  + " OR e.nombre_cientifico ILIKE " + needle + ")";
        }

        // 1) Total (sin LIMIT/OFFSET).
        const std::string countSql =
            "SELECT COUNT(*) FROM especies e" + joins + where;
        pqxx::result countRes = txn.exec(countSql);
        const int total = countRes[0][0].as<int>();

        // 2) Página actual.
        const std::string orderClause =
            " ORDER BY e." + filters.orderby + " " + filters.orderdir;
        const std::string limitClause =
            " LIMIT " + std::to_string(filters.limit)
            + " OFFSET " + std::to_string(filters.offset);

        std::string dataSql = "SELECT ";
        // Reescribir kSelectCols con alias e.* (la JOIN puede haber añadido g.*).
        dataSql +=
            "e.id, e.reino, e.genero_id, e.nombre_cientifico, e.nombre_comun, "
            "e.autor_cientifico, e.descripcion, e.habitat, e.distribucion_chiloe, "
            "e.endemica, e.estado_conservacion, e.fuentes::text AS fuentes_text, "
            "e.geo_lat, e.geo_lng, "
            "e.atributos_especificos::text AS atributos_text, "
            "e.foto_portada_key, e.fotos_keys::text AS fotos_keys_text, "
            "e.fotos_creditos::text AS fotos_creditos_text, "
            "e.categoria_id, e.estado, e.publicado_por, e.fecha_publicacion, "
            "e.creado_por, e.revisado_por, e.fecha_revision, "
            "e.created_at, e.updated_at";
        dataSql += " FROM especies e" + joins + where + orderClause + limitClause;

        pqxx::result rows = txn.exec(dataSql);

        EspecieSearchResult out;
        out.total = total;
        out.data.reserve(rows.size());
        for (const auto& row : rows) {
            out.data.push_back(mapRowToEspecie(row));
        }
        return out;
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error en find(filters): " << e.what() << std::endl;
        throw;
    }
}

std::optional<Especie> PostgreSQLEspecieRepository::findById(int id) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            std::string("SELECT ") + kSelectCols + " FROM especies WHERE id = $1",
            id);

        if (result.empty()) return std::nullopt;
        return mapRowToEspecie(result[0]);
    } catch (const std::exception& e) {
        std::cerr << "Error al obtener especie por ID: " << e.what() << std::endl;
        throw;
    }
}

std::optional<Especie> PostgreSQLEspecieRepository::getByNombreCientifico(
    const std::string& nombre) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            std::string("SELECT ") + kSelectCols
            + " FROM especies WHERE nombre_cientifico ILIKE $1",
            "%" + nombre + "%");

        if (result.empty()) return std::nullopt;
        return mapRowToEspecie(result[0]);
    } catch (const std::exception& e) {
        std::cerr << "Error al buscar especies por nombre científico: " << e.what()
                  << std::endl;
        throw;
    }
}

Especie PostgreSQLEspecieRepository::create(const Especie& especie) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        // Unicidad de nombre_cientifico
        pqxx::result check = txn.exec_params(
            "SELECT COUNT(*) FROM especies WHERE nombre_cientifico = $1",
            especie.getNombreCientifico());
        if (check[0][0].as<int>() > 0) {
            throw std::invalid_argument(
                "El nombre científico ya existe en la base de datos");
        }

        pqxx::result result = txn.exec_params(
            std::string("INSERT INTO especies (")
                + "reino, genero_id, nombre_cientifico, nombre_comun, "
                + "autor_cientifico, descripcion, habitat, distribucion_chiloe, "
                + "endemica, estado_conservacion, fuentes, geo_lat, geo_lng, "
                + "atributos_especificos, foto_portada_key, fotos_keys, "
                + "categoria_id, creado_por, revisado_por, fecha_revision, estado"
                + ") VALUES ("
                + "$1::reino_enum, $2, $3, $4, $5, $6, $7, $8, $9, $10, "
                + "$11::jsonb, $12, $13, $14::jsonb, $15, $16::jsonb, $17, $18, $19, $20, "
                + "$21::especie_estado_enum"
                + ") RETURNING " + kSelectCols,
            reinoToString(especie.getReino()),
            especie.getGeneroId(),
            especie.getNombreCientifico(),
            especie.getNombreComun(),
            especie.getAutorCientifico(),
            especie.getDescripcion(),
            especie.getHabitat(),
            especie.getDistribucionChiloe(),
            especie.isEndemica(),
            especie.getEstadoConservacion(),
            especie.getFuentes().dump(),
            especie.getGeoLat(),
            especie.getGeoLng(),
            especie.getAtributosEspecificos().dump(),
            especie.getFotoPortadaKey(),
            especie.getFotosKeys().dump(),
            especie.getCategoriaId(),
            especie.getCreadoPor(),
            especie.getRevisadoPor(),
            especie.getFechaRevision(),
            especieEstadoToString(especie.getEstado()));

        txn.commit();
        return mapRowToEspecie(result[0]);
    } catch (const std::exception& e) {
        std::cerr << "Error al crear especie: " << e.what() << std::endl;
        throw;
    }
}

Especie PostgreSQLEspecieRepository::update(const Especie& especie) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        pqxx::result check = txn.exec_params(
            "SELECT COUNT(*) FROM especies WHERE id = $1", especie.getId());
        if (check[0][0].as<int>() == 0) {
            throw std::invalid_argument(
                "Especie no encontrada con ID: " + std::to_string(especie.getId()));
        }

        check = txn.exec_params(
            "SELECT COUNT(*) FROM especies WHERE nombre_cientifico = $1 AND id != $2",
            especie.getNombreCientifico(), especie.getId());
        if (check[0][0].as<int>() > 0) {
            throw std::invalid_argument(
                "El nombre científico ya existe para otra especie");
        }

        // `estado`, `publicado_por` y `fecha_publicacion` no aparecen en el
        // SET a propósito: la transición editorial es de setEstado(), y así un
        // PUT con un cuerpo viejo no puede despublicar una ficha sin querer.
        pqxx::result result = txn.exec_params(
            std::string("UPDATE especies SET ")
                + "reino = $1::reino_enum, genero_id = $2, nombre_cientifico = $3, "
                + "nombre_comun = $4, autor_cientifico = $5, descripcion = $6, "
                + "habitat = $7, distribucion_chiloe = $8, endemica = $9, "
                + "estado_conservacion = $10, fuentes = $11::jsonb, geo_lat = $12, "
                + "geo_lng = $13, atributos_especificos = $14::jsonb, "
                + "foto_portada_key = $15, fotos_keys = $16::jsonb, "
                + "categoria_id = $17, creado_por = $18, revisado_por = $19, "
                + "fecha_revision = $20 "
                + "WHERE id = $21 RETURNING " + kSelectCols,
            reinoToString(especie.getReino()),
            especie.getGeneroId(),
            especie.getNombreCientifico(),
            especie.getNombreComun(),
            especie.getAutorCientifico(),
            especie.getDescripcion(),
            especie.getHabitat(),
            especie.getDistribucionChiloe(),
            especie.isEndemica(),
            especie.getEstadoConservacion(),
            especie.getFuentes().dump(),
            especie.getGeoLat(),
            especie.getGeoLng(),
            especie.getAtributosEspecificos().dump(),
            especie.getFotoPortadaKey(),
            especie.getFotosKeys().dump(),
            especie.getCategoriaId(),
            especie.getCreadoPor(),
            especie.getRevisadoPor(),
            especie.getFechaRevision(),
            especie.getId());

        txn.commit();
        return mapRowToEspecie(result[0]);
    } catch (const std::exception& e) {
        std::cerr << "Error al actualizar especie: " << e.what() << std::endl;
        throw;
    }
}

Especie PostgreSQLEspecieRepository::setEstado(int id,
                                               EspecieEstado estado,
                                               std::optional<int> publicadoPor) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        // Al despublicar se limpian firma y fecha (lo exige el CHECK
        // especies_borrador_sin_publicacion); al publicar se sellan con NOW().
        const bool publicando = estado == EspecieEstado::Publicada;

        pqxx::result result = txn.exec_params(
            std::string("UPDATE especies SET estado = $1::especie_estado_enum, ")
                + "publicado_por = $2, "
                + "fecha_publicacion = CASE WHEN $3::boolean THEN NOW() ELSE NULL END "
                + "WHERE id = $4 RETURNING " + kSelectCols,
            especieEstadoToString(estado),
            publicando ? publicadoPor : std::optional<int>{},
            publicando,
            id);

        if (result.empty()) {
            throw std::out_of_range("Especie no encontrada");
        }

        txn.commit();
        return mapRowToEspecie(result[0]);
    } catch (const std::out_of_range&) {
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error al cambiar el estado de la especie: " << e.what()
                  << std::endl;
        throw;
    }
}

bool PostgreSQLEspecieRepository::remove(int id) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        pqxx::result result = txn.exec_params(
            "DELETE FROM especies WHERE id = $1 RETURNING id", id);

        txn.commit();
        return !result.empty();
    } catch (const std::exception& e) {
        std::cerr << "Error al eliminar especie: " << e.what() << std::endl;
        throw;
    }
}

// =============================================================================
// Imágenes legacy: tabla especies_imagenes. Intacto en este PR. Será
// migrado a foto_portada_key + fotos_keys en la Fase 2 (object storage).
// =============================================================================

bool PostgreSQLEspecieRepository::agregarImagen(int especie_id,
                                                 const std::string& imagen_url,
                                                 bool es_principal) {
    auto conn = database->createConnection();
    pqxx::work txn(*conn);

    try {
        if (es_principal) {
            txn.exec_params(
                "UPDATE especies_imagenes SET es_principal = false "
                "WHERE especie_id = $1",
                especie_id);
        }

        txn.exec_params(
            "INSERT INTO especies_imagenes (especie_id, url, es_principal) "
            "VALUES ($1, $2, $3)",
            especie_id, imagen_url, es_principal);

        txn.commit();
        return true;
    } catch (const std::exception&) {
        txn.abort();
        return false;
    }
}

std::vector<std::string> PostgreSQLEspecieRepository::getImagenes(int especie_id) {
    auto conn = database->createConnection();
    pqxx::read_transaction txn(*conn);

    std::vector<std::string> imagenes;
    auto result = txn.exec_params(
        "SELECT url FROM especies_imagenes "
        "WHERE especie_id = $1 ORDER BY es_principal DESC",
        especie_id);

    for (const auto& row : result) {
        imagenes.push_back(row["url"].as<std::string>());
    }
    return imagenes;
}

bool PostgreSQLEspecieRepository::eliminarImagen(int especie_id,
                                                  const std::string& imagen_url) {
    auto conn = database->createConnection();
    try {
        pqxx::work txn(*conn);
        // NOTE: la tabla legacy se llama `especies_imagenes` y la columna `url`
        // (no `especie_imagenes` ni `imagen_url`). Bug del código previo.
        auto result = txn.exec_params(
            "DELETE FROM especies_imagenes WHERE especie_id = $1 AND url = $2",
            especie_id, imagen_url);
        txn.commit();
        return result.affected_rows() > 0;
    } catch (const std::exception&) {
        return false;
    }
}

bool PostgreSQLEspecieRepository::setImagenPrincipal(int especie_id,
                                                      const std::string& imagen_url) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);

        txn.exec_params(
            "UPDATE especies_imagenes SET es_principal = false "
            "WHERE especie_id = $1",
            especie_id);

        auto result = txn.exec_params(
            "UPDATE especies_imagenes SET es_principal = true "
            "WHERE especie_id = $1 AND url = $2",
            especie_id, imagen_url);

        txn.commit();
        return result.affected_rows() > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error al establecer imagen principal: " << e.what() << std::endl;
        return false;
    }
}
