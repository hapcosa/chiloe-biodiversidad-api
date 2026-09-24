#include <gtest/gtest.h>

#include <string>

#include "../include/utils/query_params.hpp"

// El bbox del mapa viajaba con las comas como %2C —encodeURIComponent lo hace
// solo— y el parseo no encontraba dónde cortar: la app pedía celdas y recibía
// 400 en cada movimiento del mapa. El test fija el contrato del lado del
// servidor, que es donde faltaba: los tests del cliente ya afirmaban el %2C.

TEST(QueryParamsTest, DecodificaLasComasDeUnBbox) {
    EXPECT_EQ(utils::percentDecode("-74.2%2C-43%2C-73.4%2C-42"),
              "-74.2,-43,-73.4,-42");
}

TEST(QueryParamsTest, DejaIntactoLoQueYaVieneSinCodificar) {
    // Un cliente puede mandar la coma literal: la RFC la permite en la query.
    EXPECT_EQ(utils::percentDecode("-74.2,-43,-73.4,-42"), "-74.2,-43,-73.4,-42");
}

TEST(QueryParamsTest, AceptaHexadecimalEnMinuscula) {
    EXPECT_EQ(utils::percentDecode("a%2cb"), "a,b");
}

TEST(QueryParamsTest, UnPorcentajeQueNoAbreUnParHexadecimalEsLiteral) {
    // Sin esto, un '%' suelto se comería los dos caracteres siguientes.
    EXPECT_EQ(utils::percentDecode("100%"), "100%");
    EXPECT_EQ(utils::percentDecode("50%ZZ"), "50%ZZ");
    EXPECT_EQ(utils::percentDecode("%2"), "%2");
}

TEST(QueryParamsTest, NoConvierteElMasEnEspacio) {
    // '+' solo significa espacio en formularios; en el bbox sería un signo.
    EXPECT_EQ(utils::percentDecode("+42%2C-73"), "+42,-73");
}

TEST(QueryParamsTest, DecodificaEspaciosYAcentosDeUnaBusqueda) {
    EXPECT_EQ(utils::percentDecode("zorro%20chilote"), "zorro chilote");
    EXPECT_EQ(utils::percentDecode("mu%C3%B1eco"), "muñeco");
}

TEST(QueryParamsTest, ElValorVacioNoRompe) {
    EXPECT_EQ(utils::percentDecode(""), "");
}

// percentEncode se usa al armar la query de una llamada saliente (el listado de
// usuarios del auth-service, Fase 9 PR 12), donde el nombre buscado lo escribe
// un humano y puede traer cualquier cosa.

TEST(QueryParamsTest, CodificaEspaciosYAcentos) {
    EXPECT_EQ(utils::percentEncode("zorro chilote"), "zorro%20chilote");
    EXPECT_EQ(utils::percentEncode("muñeco"), "mu%C3%B1eco");
}

TEST(QueryParamsTest, CodificaLoQueRomperiaLaQuery) {
    // Un '&' o un '=' sin codificar partirían la query en otro parámetro.
    EXPECT_EQ(utils::percentEncode("a&b=c"), "a%26b%3Dc");
    EXPECT_EQ(utils::percentEncode("100%"), "100%25");
}

TEST(QueryParamsTest, DejaIntactosLosNoReservados) {
    EXPECT_EQ(utils::percentEncode("Ana-Maria_1.0~x"), "Ana-Maria_1.0~x");
}

TEST(QueryParamsTest, CodificarYDecodificarDevuelveElOriginal) {
    for (const std::string original : {"zorro chilote", "María & Juan", "100%",
                                       "a+b", "", "ñandú/x?y=z"}) {
        EXPECT_EQ(utils::percentDecode(utils::percentEncode(original)), original);
    }
}
