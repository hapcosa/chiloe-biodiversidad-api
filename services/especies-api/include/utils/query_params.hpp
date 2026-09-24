#ifndef UTILS_QUERY_PARAMS_HPP
#define UTILS_QUERY_PARAMS_HPP

#include <string>

namespace utils {

// Decodifica los %XX de un valor de query string.
//
// Pistache entrega el valor tal como viajó por la red, sin decodificar. Un
// cliente que codifica la coma como %2C —lo que manda la RFC 3986 y lo que hace
// encodeURIComponent sin que se le pida— dejaba al `bbox` del mapa sin comas
// que separar, y la API respondía 400 a cada movimiento del mapa.
//
// Todo valor de query pasa por aquí, no solo el bbox: una búsqueda como
// `?q=zorro%20chilote` o `?nombre=Le%C3%B3n` llegaba igual de rota, con el
// %20 literal dentro del LIKE.
//
// Un '%' que no encabeza un par hexadecimal se deja tal cual: es un porcentaje
// literal, no una secuencia rota. El '+' tampoco se toca: solo significa
// espacio en formularios (x-www-form-urlencoded), y el cliente arma la query
// con encodeURIComponent, que codifica el espacio como %20.
std::string percentDecode(const std::string& valor);

// El camino inverso: deja intactos los caracteres no reservados de la RFC 3986
// y codifica todo lo demás. Se usa al armar la query de una llamada saliente
// (el listado de usuarios del auth-service): un nombre con espacio o con `&`
// rompería la URL.
std::string percentEncode(const std::string& valor);

} // namespace utils

#endif // UTILS_QUERY_PARAMS_HPP
