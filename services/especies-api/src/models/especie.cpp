#include "../../include/models/especie.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

// Helper para escribir una sección "key": value en JSON solo si el optional
// tiene valor; lo escribe como null en caso contrario. Centralizado para
// mantener un mismo shape de salida.
template <typename T>
void writeOpt(nlohmann::json& j, const char* key, const std::optional<T>& v) {
    if (v.has_value()) {
        j[key] = *v;
    } else {
        j[key] = nullptr;
    }
}

}  // namespace

Especie::Especie()
    : id(0),
      reino(Reino::Plantae),  // default coherente con backfill de 0002_multi_reino
      genero_id(0),
      endemica(false),
      fuentes(nlohmann::json::array()),
      atributos_especificos(nlohmann::json::object()),
      fotos_keys(nlohmann::json::array()),
      fotos_creditos(nlohmann::json::object()),
      // Fail closed: una ficha recién construida no es pública hasta que
      // alguien con curaduría la publique explícitamente.
      estado(EspecieEstado::Borrador) {}

void Especie::addImagenUrl(const std::string& imagen_url) {
    if (std::find(imagenes_urls.begin(), imagenes_urls.end(), imagen_url)
        == imagenes_urls.end()) {
        imagenes_urls.push_back(imagen_url);
    }
}

void Especie::removeImagenUrl(const std::string& imagen_url) {
    imagenes_urls.erase(
        std::remove(imagenes_urls.begin(), imagenes_urls.end(), imagen_url),
        imagenes_urls.end());
}

std::string Especie::getImagenPrincipal() const {
    return imagenes_urls.empty() ? "" : imagenes_urls.front();
}

bool Especie::esValida() const {
    if (nombre_cientifico.empty() || nombre_cientifico.length() > 200) {
        return false;
    }
    if (nombre_comun.length() > 200) return false;
    if (autor_cientifico.length() > 200) return false;
    if (estado_conservacion.length() > 60) return false;
    if (genero_id <= 0) return false;
    if (!atributos_especificos.is_object()) return false;
    if (!fuentes.is_array()) return false;
    if (!fotos_keys.is_array()) return false;
    if (foto_portada_key && foto_portada_key->length() > 500) return false;
    // Espejo del CHECK especies_borrador_sin_publicacion: un borrador no
    // arrastra la firma de una publicación anterior.
    if (estado == EspecieEstado::Borrador
        && (publicado_por || fecha_publicacion)) {
        return false;
    }
    return true;
}

nlohmann::json Especie::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["reino"] = reinoToString(reino);
    j["genero_id"] = genero_id;
    j["nombre_cientifico"] = nombre_cientifico;
    j["nombre_comun"] = nombre_comun;
    j["autor_cientifico"] = autor_cientifico;
    j["descripcion"] = descripcion;
    j["habitat"] = habitat;
    j["distribucion_chiloe"] = distribucion_chiloe;
    j["endemica"] = endemica;
    j["estado_conservacion"] = estado_conservacion;
    j["fuentes"] = fuentes;
    writeOpt(j, "geo_lat", geo_lat);
    writeOpt(j, "geo_lng", geo_lng);
    j["atributos_especificos"] = atributos_especificos;
    writeOpt(j, "foto_portada_key", foto_portada_key);
    j["fotos_keys"] = fotos_keys;
    j["fotos_creditos"] = fotos_creditos;
    writeOpt(j, "categoria_id", categoria_id);
    j["estado"] = especieEstadoToString(estado);
    writeOpt(j, "publicado_por", publicado_por);
    writeOpt(j, "fecha_publicacion", fecha_publicacion);
    writeOpt(j, "creado_por", creado_por);
    writeOpt(j, "revisado_por", revisado_por);
    writeOpt(j, "fecha_revision", fecha_revision);
    writeOpt(j, "created_at", created_at);
    writeOpt(j, "updated_at", updated_at);
    j["imagenes_urls"] = imagenes_urls;
    if (genero_nombre) {
        j["genero_nombre"] = *genero_nombre;
    }
    return j;
}

Especie Especie::fromJson(const nlohmann::json& j) {
    Especie e;

    // id solo viaja en updates; en POST nuevo es ignorado.
    if (j.contains("id") && j["id"].is_number_integer()) {
        e.id = j["id"].get<int>();
    }

    // Obligatorios
    if (!j.contains("reino") || !j["reino"].is_string()) {
        throw std::invalid_argument("El campo 'reino' es obligatorio");
    }
    e.reino = reinoFromString(j["reino"].get<std::string>());

    if (!j.contains("nombre_cientifico") || !j["nombre_cientifico"].is_string()
        || j["nombre_cientifico"].get<std::string>().empty()) {
        throw std::invalid_argument("El campo 'nombre_cientifico' es obligatorio");
    }
    e.nombre_cientifico = j["nombre_cientifico"].get<std::string>();

    if (!j.contains("genero_id") || !j["genero_id"].is_number_integer()
        || j["genero_id"].get<int>() <= 0) {
        throw std::invalid_argument("El campo 'genero_id' es obligatorio y > 0");
    }
    e.genero_id = j["genero_id"].get<int>();

    // Opcionales con default
    e.nombre_comun         = j.value("nombre_comun", "");
    e.autor_cientifico     = j.value("autor_cientifico", "");
    e.descripcion          = j.value("descripcion", "");
    e.habitat              = j.value("habitat", "");
    e.distribucion_chiloe  = j.value("distribucion_chiloe", "");
    e.endemica             = j.value("endemica", false);
    e.estado_conservacion  = j.value("estado_conservacion", "");

    // JSONB opacos: aceptar solo formas válidas.
    if (j.contains("fuentes")) {
        if (!j["fuentes"].is_array()) {
            throw std::invalid_argument("'fuentes' debe ser un array JSON");
        }
        e.fuentes = j["fuentes"];
    }
    if (j.contains("atributos_especificos")) {
        if (!j["atributos_especificos"].is_object()) {
            throw std::invalid_argument(
                "'atributos_especificos' debe ser un objeto JSON");
        }
        e.atributos_especificos = j["atributos_especificos"];
    }
    if (j.contains("fotos_keys")) {
        if (!j["fotos_keys"].is_array()) {
            throw std::invalid_argument("'fotos_keys' debe ser un array JSON");
        }
        e.fotos_keys = j["fotos_keys"];
    }

    // Geo y opcionales
    if (j.contains("geo_lat") && !j["geo_lat"].is_null()) {
        e.geo_lat = j["geo_lat"].get<double>();
    }
    if (j.contains("geo_lng") && !j["geo_lng"].is_null()) {
        e.geo_lng = j["geo_lng"].get<double>();
    }
    if (j.contains("foto_portada_key") && !j["foto_portada_key"].is_null()) {
        e.foto_portada_key = j["foto_portada_key"].get<std::string>();
    }
    if (j.contains("categoria_id") && !j["categoria_id"].is_null()) {
        e.categoria_id = j["categoria_id"].get<int>();
    }
    if (j.contains("creado_por") && !j["creado_por"].is_null()) {
        e.creado_por = j["creado_por"].get<int>();
    }
    if (j.contains("revisado_por") && !j["revisado_por"].is_null()) {
        e.revisado_por = j["revisado_por"].get<int>();
    }
    if (j.contains("fecha_revision") && !j["fecha_revision"].is_null()) {
        e.fecha_revision = j["fecha_revision"].get<std::string>();
    }

    // `estado`, `publicado_por` y `fecha_publicacion` no se leen del cuerpo:
    // el estado editorial solo cambia por POST /especies/:id/publicar y
    // /despublicar, que registran quién publicó. Mandarlos en un POST o PUT
    // no hace nada.

    // Campos llenados por la BD (created_at, updated_at) no se aceptan
    // desde input externo: el repo los rellena en mapRow.

    if (j.contains("imagenes_urls") && j["imagenes_urls"].is_array()) {
        e.imagenes_urls = j["imagenes_urls"].get<std::vector<std::string>>();
    }

    if (!e.esValida()) {
        throw std::invalid_argument("Los datos de la especie no son válidos");
    }
    return e;
}
