#include "../../include/repository/postgres_moderador_categoria_repository.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "../../include/utils/timestamps.hpp"

namespace {

constexpr const char* kCategoriaCols =
    "c.id, c.slug, c.nombre, c.reino, c.descripcion, c.created_at, c.updated_at";

std::optional<std::string> optStr(const pqxx::field& field) {
    if (field.is_null()) return std::nullopt;
    return std::string(field.c_str());
}

CategoriaModeracion mapRowToCategoria(const pqxx::row& row) {
    CategoriaModeracion categoria;
    categoria.setId(row["id"].as<int>());
    categoria.setSlug(row["slug"].c_str());
    categoria.setNombre(row["nombre"].c_str());
    categoria.setReino(reinoFromString(row["reino"].c_str()));
    categoria.setDescripcion(optStr(row["descripcion"]));
    categoria.setCreatedAt(utils::toIso8601Opt(optStr(row["created_at"])));
    categoria.setUpdatedAt(utils::toIso8601Opt(optStr(row["updated_at"])));
    return categoria;
}

} // namespace

PostgresModeradorCategoriaRepository::PostgresModeradorCategoriaRepository(
    std::shared_ptr<Database> database)
    : database(std::move(database)) {}

bool PostgresModeradorCategoriaRepository::esModeradorDe(int usuarioId, int categoriaId) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        const auto result = txn.exec_params(
            "SELECT 1 FROM moderador_categorias"
            " WHERE usuario_id = $1 AND categoria_id = $2",
            usuarioId, categoriaId);
        return !result.empty();
    } catch (const std::exception& error) {
        std::cerr << "Error al consultar curaduría: " << error.what() << std::endl;
        throw;
    }
}

std::vector<CategoriaModeracion> PostgresModeradorCategoriaRepository::categoriasDe(
    int usuarioId) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        const auto rows = txn.exec_params(
            std::string("SELECT ") + kCategoriaCols
                + " FROM moderador_categorias mc"
                  " JOIN categorias_moderacion c ON c.id = mc.categoria_id"
                  " WHERE mc.usuario_id = $1"
                  " ORDER BY c.reino, c.nombre",
            usuarioId);

        std::vector<CategoriaModeracion> categorias;
        categorias.reserve(rows.size());
        for (const auto& row : rows) {
            categorias.push_back(mapRowToCategoria(row));
        }
        return categorias;
    } catch (const std::exception& error) {
        std::cerr << "Error al listar categorías del curador: " << error.what() << std::endl;
        throw;
    }
}

std::map<int, std::vector<CategoriaModeracion>>
PostgresModeradorCategoriaRepository::asignacionesDe(
    const std::vector<int>& usuarioIds) {
    std::map<int, std::vector<CategoriaModeracion>> porUsuario;
    if (usuarioIds.empty()) return porUsuario;

    try {
        // Los ids son enteros ya parseados, así que el literal de array se
        // arma sin riesgo de inyección; pqxx no tiene binding de arrays.
        std::ostringstream literal;
        literal << '{';
        for (std::size_t i = 0; i < usuarioIds.size(); ++i) {
            if (i > 0) literal << ',';
            literal << usuarioIds[i];
        }
        literal << '}';

        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        const auto rows = txn.exec_params(
            std::string("SELECT mc.usuario_id, ") + kCategoriaCols
                + " FROM moderador_categorias mc"
                  " JOIN categorias_moderacion c ON c.id = mc.categoria_id"
                  " WHERE mc.usuario_id = ANY($1::int[])"
                  " ORDER BY mc.usuario_id, c.reino, c.nombre",
            literal.str());

        for (const auto& row : rows) {
            porUsuario[row["usuario_id"].as<int>()].push_back(
                mapRowToCategoria(row));
        }
        return porUsuario;
    } catch (const std::exception& error) {
        std::cerr << "Error al listar asignaciones de curaduría: " << error.what()
                  << std::endl;
        throw;
    }
}

std::vector<int> PostgresModeradorCategoriaRepository::usuariosConCuraduria() {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        const auto rows = txn.exec(
            "SELECT DISTINCT usuario_id FROM moderador_categorias"
            " ORDER BY usuario_id");

        std::vector<int> ids;
        ids.reserve(rows.size());
        for (const auto& row : rows) {
            ids.push_back(row["usuario_id"].as<int>());
        }
        return ids;
    } catch (const std::exception& error) {
        std::cerr << "Error al listar curadores: " << error.what() << std::endl;
        throw;
    }
}

bool PostgresModeradorCategoriaRepository::asignar(int usuarioId,
                                                   int categoriaId,
                                                   int asignadoPor) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        // ON CONFLICT DO NOTHING: reasignar no es un error, pero el caller
        // distingue creación (201) de no-op (200) por el número de filas.
        const auto result = txn.exec_params(
            "INSERT INTO moderador_categorias (usuario_id, categoria_id, asignado_por)"
            " VALUES ($1, $2, $3)"
            " ON CONFLICT (usuario_id, categoria_id) DO NOTHING"
            " RETURNING usuario_id",
            usuarioId, categoriaId, asignadoPor);

        txn.commit();
        return !result.empty();
    } catch (const pqxx::foreign_key_violation&) {
        throw std::out_of_range("categoría no encontrada");
    } catch (const std::exception& error) {
        std::cerr << "Error al asignar curaduría: " << error.what() << std::endl;
        throw;
    }
}

bool PostgresModeradorCategoriaRepository::quitar(int usuarioId, int categoriaId) {
    try {
        auto conn = database->createConnection();
        pqxx::work txn(*conn);
        const auto result = txn.exec_params(
            "DELETE FROM moderador_categorias"
            " WHERE usuario_id = $1 AND categoria_id = $2"
            " RETURNING usuario_id",
            usuarioId, categoriaId);

        txn.commit();
        return !result.empty();
    } catch (const std::exception& error) {
        std::cerr << "Error al quitar curaduría: " << error.what() << std::endl;
        throw;
    }
}
