#ifndef CURADURIA_USUARIOS_CONTROLLER_HPP
#define CURADURIA_USUARIOS_CONTROLLER_HPP

#include <memory>
#include <pistache/http.h>
#include <pistache/router.h>

#include "../services/curaduria_usuarios_service.hpp"

// El listado que consume la pantalla de usuarios del panel de curaduría: una
// fila por usuario con sus categorías asignadas. Solo admin.
class CuraduriaUsuariosController {
private:
    std::shared_ptr<CuraduriaUsuariosService> service;

public:
    explicit CuraduriaUsuariosController(
        std::shared_ptr<CuraduriaUsuariosService> service);

    void listar(const Pistache::Rest::Request& request,
                Pistache::Http::ResponseWriter response);

    static void setupRoutes(
        Pistache::Rest::Router& router,
        std::shared_ptr<CuraduriaUsuariosController> controller) {
        using namespace Pistache::Rest;

        Routes::Get(router, "/api/v1/curaduria/usuarios",
                    Routes::bind(&CuraduriaUsuariosController::listar, controller));
    }
};

#endif // CURADURIA_USUARIOS_CONTROLLER_HPP
