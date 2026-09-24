// Tests de autorización por categoría (ADR #14). Repositorio en memoria: la
// regla que interesa es quién pasa, no cómo se consulta Postgres.
#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "services/moderacion_service.hpp"

namespace {

class FakeModeradorRepository : public IModeradorCategoriaRepository {
public:
    std::set<std::pair<int, int>> asignaciones;

    bool esModeradorDe(int usuarioId, int categoriaId) override {
        return asignaciones.count({usuarioId, categoriaId}) > 0;
    }

    std::vector<CategoriaModeracion> categoriasDe(int) override { return {}; }

    // El cruce con los usuarios del auth-service se prueba en
    // test_curaduria_usuarios.cpp; acá solo interesa la regla de permiso.
    std::map<int, std::vector<CategoriaModeracion>> asignacionesDe(
        const std::vector<int>&) override {
        return {};
    }

    std::vector<int> usuariosConCuraduria() override { return {}; }

    bool asignar(int usuarioId, int categoriaId, int) override {
        return asignaciones.insert({usuarioId, categoriaId}).second;
    }

    bool quitar(int usuarioId, int categoriaId) override {
        return asignaciones.erase({usuarioId, categoriaId}) > 0;
    }
};

struct ModeracionServiceTest : public ::testing::Test {
    std::shared_ptr<FakeModeradorRepository> repo =
        std::make_shared<FakeModeradorRepository>();
    ModeracionService service{repo};
};

TEST_F(ModeracionServiceTest, AdminPuedeEnCualquierCategoria) {
    EXPECT_TRUE(service.puedeEditarCategoria(1, "admin", 42));
    EXPECT_TRUE(service.puedeEditarCategoria(1, "admin", 7));
}

TEST_F(ModeracionServiceTest, ModeratorSigueSiendoGlobal) {
    EXPECT_TRUE(service.puedeEditarCategoria(2, "moderator", 42));
}

TEST_F(ModeracionServiceTest, GlobalesPasanSinCategoria) {
    // Fichas anteriores a la migración 0004 pueden no tener categoría; alguien
    // tiene que poder arreglarlas.
    EXPECT_TRUE(service.puedeEditarCategoria(1, "admin", std::nullopt));
    EXPECT_TRUE(service.puedeEditarCategoria(2, "moderator", std::nullopt));
}

TEST_F(ModeracionServiceTest, CuradorSoloEnSuCategoria) {
    repo->asignaciones.insert({10, 42});

    EXPECT_TRUE(service.puedeEditarCategoria(10, "user", 42));
    EXPECT_FALSE(service.puedeEditarCategoria(10, "user", 7));
}

TEST_F(ModeracionServiceTest, CuradorNoPasaSinCategoria) {
    repo->asignaciones.insert({10, 42});

    EXPECT_FALSE(service.puedeEditarCategoria(10, "user", std::nullopt));
}

TEST_F(ModeracionServiceTest, UsuarioRasoNoPuede) {
    EXPECT_FALSE(service.puedeEditarCategoria(99, "user", 42));
}

TEST_F(ModeracionServiceTest, LaCuraduriaEsPorUsuarioNoPorCategoria) {
    // Que alguien cure la categoría 42 no habilita al resto del mundo en ella.
    repo->asignaciones.insert({10, 42});

    EXPECT_FALSE(service.puedeEditarCategoria(11, "user", 42));
}

TEST_F(ModeracionServiceTest, RolDesconocidoNoEsGlobal) {
    // Si el auth-service llegara a emitir un rol nuevo, el default es negar.
    EXPECT_FALSE(service.puedeEditarCategoria(3, "superadmin", 42));
}

TEST_F(ModeracionServiceTest, AsignarEsIdempotente) {
    EXPECT_TRUE(service.asignarCurador(10, 42, 1));
    EXPECT_FALSE(service.asignarCurador(10, 42, 1));
    EXPECT_TRUE(service.puedeEditarCategoria(10, "user", 42));
}

TEST_F(ModeracionServiceTest, QuitarRetiraElPermiso) {
    service.asignarCurador(10, 42, 1);

    EXPECT_TRUE(service.quitarCurador(10, 42));
    EXPECT_FALSE(service.puedeEditarCategoria(10, "user", 42));
    EXPECT_FALSE(service.quitarCurador(10, 42));
}

} // namespace
