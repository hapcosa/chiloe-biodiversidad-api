// Estado editorial de una ficha: serialización del enum y las reglas del
// modelo que no delegamos en la BD (qué acepta fromJson, qué es una ficha
// coherente). La decisión de quién ve un borrador vive en el controlador y se
// cubre en el smoke test end-to-end.
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <stdexcept>

#include "models/especie.hpp"
#include "models/especie_estado.hpp"

using json = nlohmann::json;

namespace {

json fichaMinima() {
    return json{{"reino", "plantae"},
                {"nombre_cientifico", "Drimys winteri"},
                {"genero_id", 3}};
}

TEST(EspecieEstadoTest, RoundTrip) {
    EXPECT_EQ(especieEstadoToString(EspecieEstado::Borrador), "borrador");
    EXPECT_EQ(especieEstadoToString(EspecieEstado::Publicada), "publicada");

    EXPECT_EQ(especieEstadoFromString("borrador"), EspecieEstado::Borrador);
    EXPECT_EQ(especieEstadoFromString("publicada"), EspecieEstado::Publicada);
}

TEST(EspecieEstadoTest, EstadoInventadoLanza) {
    EXPECT_THROW(especieEstadoFromString("publicado"), std::invalid_argument);
    EXPECT_THROW(especieEstadoFromString("draft"), std::invalid_argument);
    EXPECT_THROW(especieEstadoFromString(""), std::invalid_argument);

    EXPECT_TRUE(isValidEspecieEstado("borrador"));
    EXPECT_FALSE(isValidEspecieEstado("BORRADOR"));
}

TEST(EspecieEstadoTest, FichaNuevaNaceBorrador) {
    // Fail closed: nada es público hasta que alguien lo publica.
    const Especie recienConstruida;
    EXPECT_EQ(recienConstruida.getEstado(), EspecieEstado::Borrador);
    EXPECT_TRUE(recienConstruida.esBorrador());

    const auto desdeJson = Especie::fromJson(fichaMinima());
    EXPECT_EQ(desdeJson.getEstado(), EspecieEstado::Borrador);
}

TEST(EspecieEstadoTest, FromJsonIgnoraElEstadoDelCliente) {
    // Publicar es un endpoint propio; mandarlo en el cuerpo no publica nada.
    auto cuerpo = fichaMinima();
    cuerpo["estado"] = "publicada";
    cuerpo["publicado_por"] = 42;
    cuerpo["fecha_publicacion"] = "2026-08-03T00:00:00Z";

    const auto especie = Especie::fromJson(cuerpo);

    EXPECT_EQ(especie.getEstado(), EspecieEstado::Borrador);
    EXPECT_FALSE(especie.getPublicadoPor().has_value());
    EXPECT_FALSE(especie.getFechaPublicacion().has_value());
}

TEST(EspecieEstadoTest, EstadoInvalidoEnElCuerpoNoRompeElParseo) {
    // Se ignora el campo entero, así que ni siquiera se valida su valor.
    auto cuerpo = fichaMinima();
    cuerpo["estado"] = "vaporware";
    EXPECT_NO_THROW(Especie::fromJson(cuerpo));
}

TEST(EspecieEstadoTest, BorradorNoArrastraFirmaDePublicacion) {
    // Espejo del CHECK especies_borrador_sin_publicacion.
    auto especie = Especie::fromJson(fichaMinima());
    EXPECT_TRUE(especie.esValida());

    especie.setPublicadoPor(7);
    EXPECT_FALSE(especie.esValida()) << "un borrador con publicado_por no es coherente";

    especie.setPublicadoPor(std::nullopt);
    especie.setFechaPublicacion("2026-08-03T00:00:00Z");
    EXPECT_FALSE(especie.esValida());

    especie.setEstado(EspecieEstado::Publicada);
    especie.setPublicadoPor(7);
    EXPECT_TRUE(especie.esValida());
}

TEST(EspecieEstadoTest, PublicadaSinFirmaSigueSiendoValida) {
    // Las fichas anteriores a la migración 0006 están publicadas y no se sabe
    // por quién: el modelo no puede exigir la firma en esa dirección.
    auto especie = Especie::fromJson(fichaMinima());
    especie.setEstado(EspecieEstado::Publicada);

    EXPECT_TRUE(especie.esValida());
}

TEST(EspecieEstadoTest, ToJsonExponeEstadoYFirma) {
    auto especie = Especie::fromJson(fichaMinima());

    auto serializada = especie.toJson();
    EXPECT_EQ(serializada["estado"], "borrador");
    EXPECT_TRUE(serializada["publicado_por"].is_null());
    EXPECT_TRUE(serializada["fecha_publicacion"].is_null());

    especie.setEstado(EspecieEstado::Publicada);
    especie.setPublicadoPor(9);
    especie.setFechaPublicacion("2026-08-03T12:00:00Z");

    serializada = especie.toJson();
    EXPECT_EQ(serializada["estado"], "publicada");
    EXPECT_EQ(serializada["publicado_por"], 9);
    EXPECT_EQ(serializada["fecha_publicacion"], "2026-08-03T12:00:00Z");
}

TEST(EspecieFotosCreditosTest, VacioPorDefectoYNoSeLeeDelCliente) {
    // Un cliente no puede afirmar la licencia de una foto: solo la carga el servidor.
    auto j = fichaMinima();
    j["fotos_creditos"] = json{{"k", json{{"licencia", "cc0"}}}};
    const auto e = Especie::fromJson(j);
    EXPECT_TRUE(e.getFotosCreditos().is_object());
    EXPECT_TRUE(e.getFotosCreditos().empty());
    EXPECT_TRUE(e.toJson().at("fotos_creditos").is_object());
}

} // namespace
