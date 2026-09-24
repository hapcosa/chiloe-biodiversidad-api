#ifndef MODERADOR_CATEGORIA_REPOSITORY_HPP
#define MODERADOR_CATEGORIA_REPOSITORY_HPP

#include <map>
#include <vector>

#include "../models/categoria_moderacion.hpp"

// Asignaciones curador ↔ categoría (tabla `moderador_categorias`).
// `usuarioId` referencia lógicamente a `usuarios` del auth-service: sin FK,
// igual que `especies.creado_por`.
class IModeradorCategoriaRepository {
public:
    virtual ~IModeradorCategoriaRepository() = default;

    virtual bool esModeradorDe(int usuarioId, int categoriaId) = 0;

    // Categorías completas y no solo sus ids: quien pregunta (el panel, o el
    // propio usuario) siempre necesita el nombre para mostrarlo.
    virtual std::vector<CategoriaModeracion> categoriasDe(int usuarioId) = 0;

    // Las asignaciones de varios usuarios en una consulta. La pantalla de
    // usuarios del panel muestra una página entera de golpe: una consulta por
    // fila sería N+1 contra Postgres. Los usuarios sin asignación no aparecen
    // en el mapa.
    virtual std::map<int, std::vector<CategoriaModeracion>> asignacionesDe(
        const std::vector<int>& usuarioIds) = 0;

    // Ids que tienen al menos una categoría asignada. Es lo único que esta API
    // sabe de los usuarios cuando el auth-service no responde.
    virtual std::vector<int> usuariosConCuraduria() = 0;

    // Devuelven false si la asignación ya existía / no existía. Lanzan
    // std::out_of_range si la categoría no existe.
    virtual bool asignar(int usuarioId, int categoriaId, int asignadoPor) = 0;
    virtual bool quitar(int usuarioId, int categoriaId) = 0;
};

#endif // MODERADOR_CATEGORIA_REPOSITORY_HPP
