#include "../../include/services/auth_usuarios_client.hpp"

#include <curl/curl.h>

#include <cstdlib>
#include <sstream>
#include <utility>

#include "../../include/utils/query_params.hpp"
#include "../../include/utils/timestamps.hpp"

using json = nlohmann::json;

namespace {

constexpr long kTimeoutPorDefectoMs = 3000;

std::string envOr(const char* nombre, const std::string& porDefecto) {
    const char* valor = std::getenv(nombre);
    if (valor == nullptr || std::string(valor).empty()) {
        return porDefecto;
    }
    return valor;
}

std::size_t acumularCuerpo(void* datos, std::size_t tamano, std::size_t cantidad,
                           void* destino) {
    const std::size_t total = tamano * cantidad;
    static_cast<std::string*>(destino)->append(static_cast<char*>(datos), total);
    return total;
}

// Un string del JSON que puede venir ausente o null. El auth-service omite
// campos vacíos en algunas respuestas y `get<std::string>()` sobre null lanza.
std::string strOr(const json& objeto, const char* clave,
                  const std::string& porDefecto = "") {
    if (!objeto.contains(clave) || !objeto.at(clave).is_string()) {
        return porDefecto;
    }
    return objeto.at(clave).get<std::string>();
}

// El motivo que trae un 4xx del auth-service, para reenviarlo al panel en vez
// de un genérico. Se prefiere `message`: el `error` del auth-service es el
// nombre del estado HTTP ("Bad Request"), que no le dice nada a nadie. Si el
// cuerpo no es el JSON de error esperado, se cae al código.
std::string mensajeDeError(const std::string& cuerpo, long codigo) {
    try {
        const auto datos = json::parse(cuerpo);
        if (datos.is_object()) {
            for (const char* clave : {"message", "error"}) {
                if (datos.contains(clave) && datos.at(clave).is_string() &&
                    !datos.at(clave).get<std::string>().empty()) {
                    return datos.at(clave).get<std::string>();
                }
            }
        }
    } catch (const std::exception&) {
        // Cae al mensaje genérico.
    }
    return "el auth-service rechazó la consulta (HTTP " + std::to_string(codigo) +
           ")";
}

// El auth-service ya normaliza los valores fuera de rango (su default y su
// máximo de por_pagina), así que acá basta con descartar lo que no es número.
int enteroOCero(const std::string& valor) {
    try {
        return valor.empty() ? 0 : std::stoi(valor);
    } catch (const std::exception&) {
        return 0;
    }
}

} // namespace

nlohmann::json UsuarioDeAuth::toJson() const {
    return json{{"id", id},
                {"email", email},
                {"nombre", nombre},
                {"avatar", avatar},
                {"rol", rol},
                {"estado", estado},
                {"profesion", profesion},
                {"perfil_publico", perfilPublico},
                {"creado_en", creadoEn}};
}

FiltroDeUsuarios filtroDesdeQuery(const std::string& pagina,
                                  const std::string& porPagina,
                                  const std::string& buscar,
                                  const std::string& rol) {
    FiltroDeUsuarios filtro;
    const int paginaPedida = enteroOCero(pagina);
    filtro.pagina = paginaPedida > 0 ? paginaPedida : 1;
    filtro.porPagina = enteroOCero(porPagina);
    filtro.buscar = buscar;
    filtro.rol = rol;
    return filtro;
}

std::string urlDeListadoDeUsuarios(const std::string& baseUrl,
                                   const FiltroDeUsuarios& filtro) {
    std::string base = baseUrl;
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }

    std::ostringstream url;
    url << base << "/api/v1/auth/admin/users?pagina="
        << (filtro.pagina > 0 ? filtro.pagina : 1);
    if (filtro.porPagina > 0) {
        url << "&por_pagina=" << filtro.porPagina;
    }
    if (!filtro.buscar.empty()) {
        url << "&buscar=" << utils::percentEncode(filtro.buscar);
    }
    if (!filtro.rol.empty()) {
        url << "&rol=" << utils::percentEncode(filtro.rol);
    }
    return url.str();
}

PaginaDeUsuarios parsearPaginaDeUsuarios(const std::string& cuerpo) {
    json datos;
    try {
        datos = json::parse(cuerpo);
    } catch (const std::exception& error) {
        throw AuthUsuariosNoDisponible(
            std::string("respuesta ilegible del auth-service: ") + error.what());
    }

    if (!datos.is_object() || !datos.contains("usuarios") ||
        !datos.at("usuarios").is_array()) {
        throw AuthUsuariosNoDisponible(
            "el auth-service no devolvió una página de usuarios");
    }

    PaginaDeUsuarios pagina;
    if (datos.contains("total") && datos.at("total").is_number()) {
        pagina.total = datos.at("total").get<long>();
    }
    if (datos.contains("pagina") && datos.at("pagina").is_number()) {
        pagina.pagina = datos.at("pagina").get<int>();
    }
    if (datos.contains("por_pagina") && datos.at("por_pagina").is_number()) {
        pagina.porPagina = datos.at("por_pagina").get<int>();
    }

    for (const auto& fila : datos.at("usuarios")) {
        // Una fila sin id no sirve para cruzar con las asignaciones; se
        // descarta en vez de tumbar la página entera.
        if (!fila.is_object() || !fila.contains("id") ||
            !fila.at("id").is_number()) {
            continue;
        }
        UsuarioDeAuth usuario;
        usuario.id = fila.at("id").get<int>();
        usuario.email = strOr(fila, "email");
        usuario.nombre = strOr(fila, "name");
        usuario.avatar = strOr(fila, "avatar");
        usuario.rol = strOr(fila, "role");
        usuario.estado = strOr(fila, "status");
        usuario.profesion = strOr(fila, "profesion");
        usuario.perfilPublico = fila.contains("perfil_publico") &&
                                fila.at("perfil_publico").is_boolean() &&
                                fila.at("perfil_publico").get<bool>();
        // El auth-service (Go) entrega la fecha con seis dígitos de fracción y
        // desplazamiento local; el ADR #16 exige que todo lo que sale de esta
        // API vaya en el mismo perfil ISO 8601, o Hermes la rechaza.
        usuario.creadoEn = utils::toIso8601(strOr(fila, "created_at"));
        pagina.usuarios.push_back(std::move(usuario));
    }

    return pagina;
}

AuthUsuariosClient::AuthUsuariosClient()
    : baseUrl(envOr("AUTH_SERVICE_URL", "http://auth-service:8081")),
      timeoutMs(kTimeoutPorDefectoMs) {}

AuthUsuariosClient::AuthUsuariosClient(std::string baseUrl, long timeoutMs)
    : baseUrl(std::move(baseUrl)), timeoutMs(timeoutMs) {}

PaginaDeUsuarios AuthUsuariosClient::listar(const FiltroDeUsuarios& filtro,
                                            const std::string& authorization) {
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        throw AuthUsuariosNoDisponible("no se pudo inicializar el cliente HTTP");
    }

    const std::string url = urlDeListadoDeUsuarios(baseUrl, filtro);
    std::string cuerpo;
    long codigo = 0;

    struct curl_slist* cabeceras = nullptr;
    if (!authorization.empty()) {
        cabeceras = curl_slist_append(
            cabeceras, (std::string("Authorization: ") + authorization).c_str());
    }
    cabeceras = curl_slist_append(cabeceras, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, cabeceras);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, acumularCuerpo);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cuerpo);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    const CURLcode resultado = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &codigo);
    curl_slist_free_all(cabeceras);
    curl_easy_cleanup(curl);

    if (resultado != CURLE_OK) {
        throw AuthUsuariosNoDisponible(std::string("el auth-service no respondió: ") +
                                       curl_easy_strerror(resultado));
    }
    if (codigo >= 400 && codigo < 500) {
        throw AuthUsuariosRechazo(codigo, mensajeDeError(cuerpo, codigo));
    }
    if (codigo < 200 || codigo >= 300) {
        throw AuthUsuariosNoDisponible("el auth-service respondió HTTP " +
                                       std::to_string(codigo));
    }

    return parsearPaginaDeUsuarios(cuerpo);
}
