#include "../../include/services/curaduria_usuarios_service.hpp"

#include <iostream>
#include <utility>

using json = nlohmann::json;

nlohmann::json UsuarioDeCuraduria::toJson() const {
    json salida = usuario.toJson();
    json asignadas = json::array();
    for (const auto& categoria : categorias) {
        asignadas.push_back(categoria.toJson());
    }
    salida["categorias_curadas"] = asignadas;
    return salida;
}

nlohmann::json PaginaDeCuraduria::toJson() const {
    json filas = json::array();
    for (const auto& fila : usuarios) {
        filas.push_back(fila.toJson());
    }
    return json{{"usuarios", filas},
                {"total", total},
                {"pagina", pagina},
                {"por_pagina", porPagina},
                {"auth_disponible", authDisponible}};
}

CuraduriaUsuariosService::CuraduriaUsuariosService(
    std::shared_ptr<IAuthUsuariosClient> authClient,
    std::shared_ptr<IModeradorCategoriaRepository> moderadorRepository)
    : authClient(std::move(authClient)),
      moderadorRepository(std::move(moderadorRepository)) {}

PaginaDeCuraduria CuraduriaUsuariosService::listar(
    const FiltroDeUsuarios& filtro, const std::string& authorization) {
    PaginaDeCuraduria pagina;

    PaginaDeUsuarios deAuth;
    try {
        deAuth = authClient->listar(filtro, authorization);
    } catch (const AuthUsuariosNoDisponible& error) {
        // El panel de curaduría no puede quedarse en blanco porque el
        // auth-service esté caído: devolvemos lo que esta base sí sabe, que son
        // los ids con curaduría asignada. El filtro y la paginación del
        // auth-service no aplican acá; se devuelven todos.
        std::cerr << "auth-service no disponible, listado degradado: "
                  << error.what() << std::endl;

        const auto ids = moderadorRepository->usuariosConCuraduria();
        auto asignaciones = moderadorRepository->asignacionesDe(ids);

        pagina.authDisponible = false;
        pagina.pagina = 1;
        pagina.porPagina = static_cast<int>(ids.size());
        pagina.total = static_cast<long>(ids.size());
        for (const int id : ids) {
            UsuarioDeCuraduria fila;
            fila.usuario.id = id;
            auto encontrado = asignaciones.find(id);
            if (encontrado != asignaciones.end()) {
                fila.categorias = encontrado->second;
            }
            pagina.usuarios.push_back(std::move(fila));
        }
        return pagina;
    }

    pagina.total = deAuth.total;
    pagina.pagina = deAuth.pagina;
    pagina.porPagina = deAuth.porPagina;

    std::vector<int> ids;
    ids.reserve(deAuth.usuarios.size());
    for (const auto& usuario : deAuth.usuarios) {
        ids.push_back(usuario.id);
    }
    auto asignaciones = moderadorRepository->asignacionesDe(ids);

    for (auto& usuario : deAuth.usuarios) {
        UsuarioDeCuraduria fila;
        auto encontrado = asignaciones.find(usuario.id);
        if (encontrado != asignaciones.end()) {
            fila.categorias = encontrado->second;
        }
        fila.usuario = std::move(usuario);
        pagina.usuarios.push_back(std::move(fila));
    }

    return pagina;
}
