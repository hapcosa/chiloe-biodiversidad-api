#ifndef AUTH_USUARIOS_CLIENT_HPP
#define AUTH_USUARIOS_CLIENT_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

// Cliente del listado de usuarios del auth-service.
//
// Los usuarios viven en el auth-service (Go) y las asignaciones de curaduría
// acá (`moderador_categorias`). La pantalla de usuarios del panel los necesita
// en una sola tabla, y la decisión tomada fue que esta API hidrate los nombres
// llamando al auth-service, en vez de dejar el cruce en el navegador.
//
// La llamada **reenvía el Authorization de quien preguntó**: el listado del
// auth-service exige rol admin y esta pantalla también, así que no hace falta
// una credencial de servicio ni se gana permiso que el llamador no tuviera.

struct UsuarioDeAuth {
    int id = 0;
    std::string email;
    std::string nombre;
    std::string avatar;
    std::string rol;
    std::string estado;
    std::string profesion;
    bool perfilPublico = false;
    std::string creadoEn;

    nlohmann::json toJson() const;
};

struct FiltroDeUsuarios {
    int pagina = 1;
    int porPagina = 0;  // 0 = el default del auth-service
    std::string buscar;
    std::string rol;
};

struct PaginaDeUsuarios {
    std::vector<UsuarioDeAuth> usuarios;
    long total = 0;
    int pagina = 1;
    int porPagina = 0;
};

// El auth-service no contestó, o contestó algo que no se puede leer. Se
// distingue de un error propio porque la pantalla degrada en vez de fallar.
class AuthUsuariosNoDisponible : public std::runtime_error {
public:
    explicit AuthUsuariosNoDisponible(const std::string& motivo)
        : std::runtime_error(motivo) {}
};

class IAuthUsuariosClient {
public:
    virtual ~IAuthUsuariosClient() = default;

    // Lanza AuthUsuariosNoDisponible si el auth-service no responde o
    // responde un cuerpo ilegible.
    virtual PaginaDeUsuarios listar(const FiltroDeUsuarios& filtro,
                                    const std::string& authorization) = 0;
};

// Arma el filtro con los valores crudos de la query. Vive acá y no en el
// controller para poder probarla sin enlazar Pistache.
FiltroDeUsuarios filtroDesdeQuery(const std::string& pagina,
                                  const std::string& porPagina,
                                  const std::string& buscar,
                                  const std::string& rol);

// Arma la URL del listado. Separada del transporte para poder probarla.
std::string urlDeListadoDeUsuarios(const std::string& baseUrl,
                                   const FiltroDeUsuarios& filtro);

// Lee la respuesta del auth-service. Lanza AuthUsuariosNoDisponible si el
// cuerpo no tiene la forma esperada.
PaginaDeUsuarios parsearPaginaDeUsuarios(const std::string& cuerpo);

class AuthUsuariosClient : public IAuthUsuariosClient {
private:
    std::string baseUrl;
    long timeoutMs;

public:
    // Por defecto sale de AUTH_SERVICE_URL, igual que el resto de la
    // configuración del servicio.
    AuthUsuariosClient();
    AuthUsuariosClient(std::string baseUrl, long timeoutMs);

    PaginaDeUsuarios listar(const FiltroDeUsuarios& filtro,
                            const std::string& authorization) override;
};

#endif // AUTH_USUARIOS_CLIENT_HPP
