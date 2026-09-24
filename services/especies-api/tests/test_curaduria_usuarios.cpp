// El cruce entre los usuarios del auth-service y las asignaciones de curaduría
// de esta base (Fase 9, PR 12). Sin red ni Postgres: el cliente HTTP y el
// repositorio se reemplazan por dobles, porque lo que interesa es qué URL se
// arma, qué se lee de la respuesta y qué devuelve la pantalla cuando el
// auth-service no contesta.
#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "services/curaduria_usuarios_service.hpp"

namespace {

CategoriaModeracion categoria(int id, const std::string& nombre, Reino reino) {
    CategoriaModeracion c;
    c.setId(id);
    c.setSlug(nombre);
    c.setNombre(nombre);
    c.setReino(reino);
    return c;
}

class FakeAuthClient : public IAuthUsuariosClient {
public:
    PaginaDeUsuarios respuesta;
    bool falla = false;
    FiltroDeUsuarios filtroRecibido;
    std::string authorizationRecibido;
    int llamadas = 0;

    PaginaDeUsuarios listar(const FiltroDeUsuarios& filtro,
                            const std::string& authorization) override {
        ++llamadas;
        filtroRecibido = filtro;
        authorizationRecibido = authorization;
        if (falla) throw AuthUsuariosNoDisponible("caído");
        return respuesta;
    }
};

class FakeModeradorRepo : public IModeradorCategoriaRepository {
public:
    std::map<int, std::vector<CategoriaModeracion>> porUsuario;
    std::vector<int> idsPedidos;

    bool esModeradorDe(int, int) override { return false; }
    std::vector<CategoriaModeracion> categoriasDe(int) override { return {}; }
    bool asignar(int, int, int) override { return true; }
    bool quitar(int, int) override { return true; }

    std::map<int, std::vector<CategoriaModeracion>> asignacionesDe(
        const std::vector<int>& usuarioIds) override {
        idsPedidos = usuarioIds;
        std::map<int, std::vector<CategoriaModeracion>> salida;
        for (const int id : usuarioIds) {
            auto encontrado = porUsuario.find(id);
            if (encontrado != porUsuario.end()) salida[id] = encontrado->second;
        }
        return salida;
    }

    std::vector<int> usuariosConCuraduria() override {
        std::vector<int> ids;
        for (const auto& [id, categorias] : porUsuario) ids.push_back(id);
        return ids;
    }
};

struct CuraduriaUsuariosTest : public ::testing::Test {
    std::shared_ptr<FakeAuthClient> auth = std::make_shared<FakeAuthClient>();
    std::shared_ptr<FakeModeradorRepo> repo = std::make_shared<FakeModeradorRepo>();
    CuraduriaUsuariosService service{auth, repo};

    static UsuarioDeAuth usuario(int id, const std::string& nombre) {
        UsuarioDeAuth u;
        u.id = id;
        u.nombre = nombre;
        u.email = nombre + "@ejemplo.cl";
        u.rol = "user";
        return u;
    }
};

// --- la URL que se le manda al auth-service ---

TEST(UrlDeListadoDeUsuarios, LlevaSiempreLaPagina) {
    const FiltroDeUsuarios filtro;
    EXPECT_EQ(urlDeListadoDeUsuarios("http://auth:8081", filtro),
              "http://auth:8081/api/v1/auth/admin/users?pagina=1");
}

TEST(UrlDeListadoDeUsuarios, NoDuplicaLaBarraDeLaBase) {
    const FiltroDeUsuarios filtro;
    EXPECT_EQ(urlDeListadoDeUsuarios("http://auth:8081///", filtro),
              "http://auth:8081/api/v1/auth/admin/users?pagina=1");
}

TEST(UrlDeListadoDeUsuarios, OmiteElPorPaginaParaDejarElDefaultDelAuthService) {
    FiltroDeUsuarios filtro;
    filtro.porPagina = 0;
    EXPECT_EQ(urlDeListadoDeUsuarios("http://auth", filtro).find("por_pagina"),
              std::string::npos);

    filtro.porPagina = 50;
    EXPECT_NE(urlDeListadoDeUsuarios("http://auth", filtro).find("por_pagina=50"),
              std::string::npos);
}

TEST(UrlDeListadoDeUsuarios, CodificaLaBusqueda) {
    // Un `&` o un espacio sin codificar partirían la query en dos parámetros.
    FiltroDeUsuarios filtro;
    filtro.buscar = "María & Juan";
    const auto url = urlDeListadoDeUsuarios("http://auth", filtro);

    EXPECT_NE(url.find("buscar=Mar%C3%ADa%20%26%20Juan"), std::string::npos);
    EXPECT_EQ(url.find("Juan&"), std::string::npos);
}

// --- lo que se lee de la respuesta ---

TEST(ParsearPaginaDeUsuarios, LeeLaPaginaCompleta) {
    const auto pagina = parsearPaginaDeUsuarios(R"({
        "usuarios": [
            {"id": 7, "email": "a@b.cl", "name": "Ana", "role": "admin",
             "status": "active", "perfil_publico": true,
             "created_at": "2026-01-02T03:04:05Z"}
        ],
        "total": 31, "pagina": 2, "por_pagina": 25
    })");

    ASSERT_EQ(pagina.usuarios.size(), 1u);
    EXPECT_EQ(pagina.usuarios[0].id, 7);
    EXPECT_EQ(pagina.usuarios[0].nombre, "Ana");
    EXPECT_EQ(pagina.usuarios[0].email, "a@b.cl");
    EXPECT_EQ(pagina.usuarios[0].rol, "admin");
    EXPECT_TRUE(pagina.usuarios[0].perfilPublico);
    EXPECT_EQ(pagina.total, 31);
    EXPECT_EQ(pagina.pagina, 2);
    EXPECT_EQ(pagina.porPagina, 25);
}

TEST(ParsearPaginaDeUsuarios, ToleraCamposAusentesONull) {
    const auto pagina = parsearPaginaDeUsuarios(
        R"({"usuarios": [{"id": 3, "avatar": null, "profesion": null}]})");

    ASSERT_EQ(pagina.usuarios.size(), 1u);
    EXPECT_EQ(pagina.usuarios[0].id, 3);
    EXPECT_EQ(pagina.usuarios[0].avatar, "");
    EXPECT_EQ(pagina.usuarios[0].profesion, "");
    EXPECT_FALSE(pagina.usuarios[0].perfilPublico);
}

TEST(ParsearPaginaDeUsuarios, DescartaFilasSinId) {
    // Sin id no hay con qué cruzar las asignaciones, pero el resto de la página
    // sigue sirviendo.
    const auto pagina = parsearPaginaDeUsuarios(
        R"({"usuarios": [{"name": "sin id"}, {"id": 5}]})");

    ASSERT_EQ(pagina.usuarios.size(), 1u);
    EXPECT_EQ(pagina.usuarios[0].id, 5);
}

TEST(ParsearPaginaDeUsuarios, CuerpoIlegibleEsAuthNoDisponible) {
    EXPECT_THROW(parsearPaginaDeUsuarios("no soy json"), AuthUsuariosNoDisponible);
    EXPECT_THROW(parsearPaginaDeUsuarios(R"({"error": "unauthorized"})"),
                 AuthUsuariosNoDisponible);
    EXPECT_THROW(parsearPaginaDeUsuarios(R"({"usuarios": 3})"),
                 AuthUsuariosNoDisponible);
}

// --- el filtro que llega por query string ---

TEST(FiltroDesdeQuery, PaginaVaciaOInvalidaEsLaPrimera) {
    EXPECT_EQ(filtroDesdeQuery("", "", "", "").pagina, 1);
    EXPECT_EQ(filtroDesdeQuery("abc", "", "", "").pagina, 1);
    EXPECT_EQ(filtroDesdeQuery("-4", "", "", "").pagina, 1);
    EXPECT_EQ(filtroDesdeQuery("3", "", "", "").pagina, 3);
}

TEST(FiltroDesdeQuery, PorPaginaNoNumericoQuedaEnElDefaultDelAuthService) {
    EXPECT_EQ(filtroDesdeQuery("1", "muchos", "", "").porPagina, 0);
    EXPECT_EQ(filtroDesdeQuery("1", "40", "", "").porPagina, 40);
}

TEST(FiltroDesdeQuery, BuscarYRolViajanTalCual) {
    // Validar el rol es asunto del auth-service, que devuelve 400; repetir la
    // lista de roles acá sería una segunda fuente de verdad.
    const auto filtro = filtroDesdeQuery("1", "", "  Ana  ", "curador");
    EXPECT_EQ(filtro.buscar, "  Ana  ");
    EXPECT_EQ(filtro.rol, "curador");
}

// --- el cruce ---

TEST_F(CuraduriaUsuariosTest, AdjuntaLasCategoriasQueCadaUnoCura) {
    auth->respuesta.usuarios = {usuario(10, "Ana"), usuario(11, "Beto")};
    auth->respuesta.total = 2;
    auth->respuesta.pagina = 1;
    auth->respuesta.porPagina = 25;
    repo->porUsuario[10] = {categoria(1, "aves", Reino::Animalia),
                            categoria(2, "hongos", Reino::Fungi)};

    const auto pagina = service.listar(FiltroDeUsuarios{}, "Bearer x");

    ASSERT_EQ(pagina.usuarios.size(), 2u);
    EXPECT_EQ(pagina.usuarios[0].usuario.nombre, "Ana");
    EXPECT_EQ(pagina.usuarios[0].categorias.size(), 2u);
    EXPECT_EQ(pagina.usuarios[1].usuario.nombre, "Beto");
    EXPECT_TRUE(pagina.usuarios[1].categorias.empty());
    EXPECT_TRUE(pagina.authDisponible);
    EXPECT_EQ(pagina.total, 2);
}

TEST_F(CuraduriaUsuariosTest, PideLasAsignacionesEnUnaSolaConsulta) {
    // Una consulta por fila sería N+1 contra Postgres en cada carga del panel.
    auth->respuesta.usuarios = {usuario(10, "Ana"), usuario(11, "Beto"),
                                usuario(12, "Carla")};

    service.listar(FiltroDeUsuarios{}, "Bearer x");

    EXPECT_EQ(repo->idsPedidos, std::vector<int>({10, 11, 12}));
}

TEST_F(CuraduriaUsuariosTest, ReenviaElAuthorizationDeQuienPregunto) {
    service.listar(FiltroDeUsuarios{}, "Bearer el-jwt-del-admin");

    EXPECT_EQ(auth->authorizationRecibido, "Bearer el-jwt-del-admin");
    EXPECT_EQ(auth->llamadas, 1);
}

TEST_F(CuraduriaUsuariosTest, PasaElFiltroSinTocarlo) {
    FiltroDeUsuarios filtro;
    filtro.pagina = 3;
    filtro.porPagina = 10;
    filtro.buscar = "ana";
    filtro.rol = "admin";

    service.listar(filtro, "Bearer x");

    EXPECT_EQ(auth->filtroRecibido.pagina, 3);
    EXPECT_EQ(auth->filtroRecibido.porPagina, 10);
    EXPECT_EQ(auth->filtroRecibido.buscar, "ana");
    EXPECT_EQ(auth->filtroRecibido.rol, "admin");
}

// --- degradación ---

TEST_F(CuraduriaUsuariosTest, SinAuthServiceDevuelveLosIdsQueCuranAlgo) {
    // El panel no puede quedar en blanco porque el auth-service esté caído:
    // devuelve lo que esta base sí sabe, marcado como degradado.
    auth->falla = true;
    repo->porUsuario[10] = {categoria(1, "aves", Reino::Animalia)};
    repo->porUsuario[42] = {categoria(2, "hongos", Reino::Fungi)};

    const auto pagina = service.listar(FiltroDeUsuarios{}, "Bearer x");

    EXPECT_FALSE(pagina.authDisponible);
    ASSERT_EQ(pagina.usuarios.size(), 2u);
    EXPECT_EQ(pagina.usuarios[0].usuario.id, 10);
    EXPECT_EQ(pagina.usuarios[0].usuario.nombre, "");
    EXPECT_EQ(pagina.usuarios[0].categorias.size(), 1u);
    EXPECT_EQ(pagina.usuarios[1].usuario.id, 42);
    EXPECT_EQ(pagina.total, 2);
}

TEST_F(CuraduriaUsuariosTest, SinAuthServiceNiCuradoresDevuelveVacioNoUnError) {
    auth->falla = true;

    const auto pagina = service.listar(FiltroDeUsuarios{}, "Bearer x");

    EXPECT_FALSE(pagina.authDisponible);
    EXPECT_TRUE(pagina.usuarios.empty());
    EXPECT_EQ(pagina.total, 0);
}

TEST_F(CuraduriaUsuariosTest, ElJsonDejaVerSiElListadoEstaDegradado) {
    auth->falla = true;
    repo->porUsuario[10] = {categoria(1, "aves", Reino::Animalia)};

    const auto cuerpo = service.listar(FiltroDeUsuarios{}, "Bearer x").toJson();

    EXPECT_FALSE(cuerpo.at("auth_disponible").get<bool>());
    ASSERT_TRUE(cuerpo.at("usuarios").is_array());
    EXPECT_EQ(cuerpo.at("usuarios")[0].at("id").get<int>(), 10);
    EXPECT_EQ(cuerpo.at("usuarios")[0].at("categorias_curadas").size(), 1u);
}

} // namespace
