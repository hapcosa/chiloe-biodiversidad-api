#include "../../include/controllers/curaduria_usuarios_controller.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>

#include "../../include/utils/query_params.hpp"
#include "../../include/utils/request_identity.hpp"

using json = nlohmann::json;

namespace {

void sendJson(Pistache::Http::ResponseWriter& response,
              Pistache::Http::Code code,
              const json& payload) {
    response.headers().add<Pistache::Http::Header::ContentType>(
        MIME(Application, Json));
    response.send(code, payload.dump());
}

// Pistache entrega el valor tal como viajó por la red; hay que decodificarlo.
std::string queryOVacio(const Pistache::Http::Uri::Query& query,
                        const std::string& clave) {
    if (!query.has(clave)) return "";
    return utils::percentDecode(query.get(clave).value());
}

} // namespace

CuraduriaUsuariosController::CuraduriaUsuariosController(
    std::shared_ptr<CuraduriaUsuariosService> service)
    : service(std::move(service)) {}

void CuraduriaUsuariosController::listar(const Pistache::Rest::Request& request,
                                         Pistache::Http::ResponseWriter response) {
    auto identity = extractIdentity(request);
    if (!identity) {
        sendJson(response, Pistache::Http::Code::Unauthorized,
                 {{"success", false},
                  {"error", "No se pudo verificar la sesión del usuario"}});
        return;
    }
    if (!identity->isAdmin()) {
        sendJson(response, Pistache::Http::Code::Forbidden,
                 {{"success", false}, {"error", "Se requiere rol admin"}});
        return;
    }

    // El auth-service exige admin en su propio listado, así que le reenviamos el
    // JWT de quien preguntó en vez de darle a esta API una credencial propia.
    auto authorization = request.headers().tryGetRaw("Authorization");
    if (!authorization) {
        sendJson(response, Pistache::Http::Code::Unauthorized,
                 {{"success", false},
                  {"error", "Falta la cabecera Authorization"}});
        return;
    }

    try {
        const auto& query = request.query();
        const auto filtro = filtroDesdeQuery(queryOVacio(query, "pagina"),
                                             queryOVacio(query, "por_pagina"),
                                             queryOVacio(query, "buscar"),
                                             queryOVacio(query, "rol"));

        const auto pagina = service->listar(filtro, authorization.value().value());

        json cuerpo = pagina.toJson();
        cuerpo["success"] = true;
        sendJson(response, Pistache::Http::Code::Ok, cuerpo);
    } catch (const AuthUsuariosRechazo& error) {
        // El auth-service rechazó la consulta: el filtro venía mal o el JWT
        // reenviado no le sirve. Se propaga su código en vez de inventar un 500.
        sendJson(response, static_cast<Pistache::Http::Code>(error.codigo),
                 {{"success", false}, {"error", error.what()}});
    } catch (const std::exception& error) {
        sendJson(response, Pistache::Http::Code::Internal_Server_Error,
                 {{"success", false}, {"error", error.what()}});
    }
}
