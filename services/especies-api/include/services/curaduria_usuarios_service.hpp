#ifndef CURADURIA_USUARIOS_SERVICE_HPP
#define CURADURIA_USUARIOS_SERVICE_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../repository/moderador_categoria_repository.hpp"
#include "auth_usuarios_client.hpp"

// Cruza los usuarios del auth-service con las asignaciones de curaduría que
// viven en esta base. La pantalla de usuarios del panel los necesita en una
// sola tabla y el cruce no se deja en el navegador (ADR #27).

struct UsuarioDeCuraduria {
    UsuarioDeAuth usuario;
    std::vector<CategoriaModeracion> categorias;

    nlohmann::json toJson() const;
};

struct PaginaDeCuraduria {
    std::vector<UsuarioDeCuraduria> usuarios;
    long total = 0;
    int pagina = 1;
    int porPagina = 0;

    // false cuando el auth-service no respondió: la página trae solo los ids
    // que curan algo, sin nombre ni email, y el panel avisa en vez de romperse.
    bool authDisponible = true;

    nlohmann::json toJson() const;
};

class CuraduriaUsuariosService {
private:
    std::shared_ptr<IAuthUsuariosClient> authClient;
    std::shared_ptr<IModeradorCategoriaRepository> moderadorRepository;

public:
    CuraduriaUsuariosService(
        std::shared_ptr<IAuthUsuariosClient> authClient,
        std::shared_ptr<IModeradorCategoriaRepository> moderadorRepository);

    // `authorization` es la cabecera de quien preguntó, que se reenvía tal cual
    // al auth-service. Nunca lanza por culpa del auth-service: si no responde,
    // devuelve la página degradada.
    PaginaDeCuraduria listar(const FiltroDeUsuarios& filtro,
                             const std::string& authorization);
};

#endif // CURADURIA_USUARIOS_SERVICE_HPP
