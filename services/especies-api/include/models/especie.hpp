#ifndef ESPECIE_HPP
#define ESPECIE_HPP

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

#include "especie_estado.hpp"
#include "reino.hpp"

// Modelo de especie post Fase 1 multi-reino. Mapea 1:1 con la tabla
// `especies` definida en migraciones/0001_initial.sql y 0002_multi_reino.sql.
//
// Convenciones:
//  - Strings que pueden ser NULL en BD se representan vacíos si llegan así,
//    salvo los marcados como std::optional (foto_portada_key, fechas).
//  - atributos_especificos y fuentes son nlohmann::json opacos en este PR.
//    La validación por reino vía JSON Schema se introduce en el PR siguiente
//    (ver docs/PLAN_MAESTRO.md Fase 1).
class Especie {
private:
    // Identificación
    int id;
    Reino reino;
    int genero_id;
    std::string nombre_cientifico;
    std::string nombre_comun;
    std::string autor_cientifico;

    // Contenido divulgativo
    std::string descripcion;
    std::string habitat;
    std::string distribucion_chiloe;
    bool endemica;
    std::string estado_conservacion;

    // Fuentes y geolocalización
    nlohmann::json fuentes;  // array JSONB, default []
    std::optional<double> geo_lat;
    std::optional<double> geo_lng;

    // Atributos específicos del reino, JSONB opaco hasta el PR de JSON Schemas
    nlohmann::json atributos_especificos;  // objeto JSONB, default {}

    // Fotos: claves de object storage (MinIO/S3). Las URLs derivadas se
    // construyen en la capa de presentación cuando MinIO esté operativo.
    std::optional<std::string> foto_portada_key;
    nlohmann::json fotos_keys;  // array de strings JSONB, default []
    // Autor y licencia por foto, indexado por key (migración 0015). Solo
    // lectura para los clientes: fromJson no lo toca.
    nlohmann::json fotos_creditos;  // objeto JSONB, default {}

    // Categoría de moderación a la que pertenece la ficha. Determina quién
    // puede editarla (ver migrations/0004_categorias_moderacion.sql). Opcional
    // mientras queden fichas anteriores a la migración sin clasificar.
    std::optional<int> categoria_id;

    // Estado editorial (ver migrations/0006_especies_estado.sql). Solo lo
    // cambian los endpoints publicar/despublicar, nunca un PUT de la ficha.
    EspecieEstado estado;
    std::optional<int> publicado_por;
    std::optional<std::string> fecha_publicacion;  // ISO 8601

    // Auditoría y revisión curatorial
    std::optional<int> creado_por;
    std::optional<int> revisado_por;
    std::optional<std::string> fecha_revision;  // ISO 8601
    std::optional<std::string> created_at;      // ISO 8601, llenado por la BD
    std::optional<std::string> updated_at;      // ISO 8601, llenado por trigger

    // Legacy: tabla especies_imagenes con URLs completas. Se mantiene en
    // este PR para no romper el flujo existente. Se deprecará y migrará
    // a fotos_keys en la Fase 2 cuando exista MinIO.
    std::vector<std::string> imagenes_urls;
    std::optional<std::string> genero_nombre;

public:
    Especie();

    // Getters
    int                                getId()                  const { return id; }
    Reino                              getReino()               const { return reino; }
    int                                getGeneroId()            const { return genero_id; }
    const std::string&                 getNombreCientifico()    const { return nombre_cientifico; }
    const std::string&                 getNombreComun()         const { return nombre_comun; }
    const std::string&                 getAutorCientifico()     const { return autor_cientifico; }
    const std::string&                 getDescripcion()         const { return descripcion; }
    const std::string&                 getHabitat()             const { return habitat; }
    const std::string&                 getDistribucionChiloe()  const { return distribucion_chiloe; }
    bool                               isEndemica()             const { return endemica; }
    const std::string&                 getEstadoConservacion()  const { return estado_conservacion; }
    const nlohmann::json&              getFuentes()             const { return fuentes; }
    const std::optional<double>&       getGeoLat()              const { return geo_lat; }
    const std::optional<double>&       getGeoLng()              const { return geo_lng; }
    const nlohmann::json&              getAtributosEspecificos()const { return atributos_especificos; }
    const std::optional<std::string>&  getFotoPortadaKey()      const { return foto_portada_key; }
    const nlohmann::json&              getFotosKeys()           const { return fotos_keys; }
    const nlohmann::json&              getFotosCreditos()       const { return fotos_creditos; }
    const std::optional<int>&          getCategoriaId()         const { return categoria_id; }
    EspecieEstado                      getEstado()              const { return estado; }
    bool                               esBorrador()             const { return estado == EspecieEstado::Borrador; }
    const std::optional<int>&          getPublicadoPor()        const { return publicado_por; }
    const std::optional<std::string>&  getFechaPublicacion()    const { return fecha_publicacion; }
    const std::optional<int>&          getCreadoPor()           const { return creado_por; }
    const std::optional<int>&          getRevisadoPor()         const { return revisado_por; }
    const std::optional<std::string>&  getFechaRevision()       const { return fecha_revision; }
    const std::optional<std::string>&  getCreatedAt()           const { return created_at; }
    const std::optional<std::string>&  getUpdatedAt()           const { return updated_at; }
    const std::vector<std::string>&    getImagenesUrls()        const { return imagenes_urls; }
    const std::optional<std::string>&  getGeneroNombre()        const { return genero_nombre; }

    // Setters
    void setId(int v)                                           { id = v; }
    void setReino(Reino v)                                      { reino = v; }
    void setGeneroId(int v)                                     { genero_id = v; }
    void setNombreCientifico(const std::string& v)              { nombre_cientifico = v; }
    void setNombreComun(const std::string& v)                   { nombre_comun = v; }
    void setAutorCientifico(const std::string& v)               { autor_cientifico = v; }
    void setDescripcion(const std::string& v)                   { descripcion = v; }
    void setHabitat(const std::string& v)                       { habitat = v; }
    void setDistribucionChiloe(const std::string& v)            { distribucion_chiloe = v; }
    void setEndemica(bool v)                                    { endemica = v; }
    void setEstadoConservacion(const std::string& v)            { estado_conservacion = v; }
    void setFuentes(const nlohmann::json& v)                    { fuentes = v; }
    void setGeoLat(std::optional<double> v)                     { geo_lat = std::move(v); }
    void setGeoLng(std::optional<double> v)                     { geo_lng = std::move(v); }
    void setAtributosEspecificos(const nlohmann::json& v)       { atributos_especificos = v; }
    void setFotoPortadaKey(std::optional<std::string> v)        { foto_portada_key = std::move(v); }
    void setFotosKeys(const nlohmann::json& v)                  { fotos_keys = v; }
    void setFotosCreditos(const nlohmann::json& v)              { fotos_creditos = v; }
    void setCategoriaId(std::optional<int> v)                   { categoria_id = v; }
    void setEstado(EspecieEstado v)                             { estado = v; }
    void setPublicadoPor(std::optional<int> v)                  { publicado_por = v; }
    void setFechaPublicacion(std::optional<std::string> v)      { fecha_publicacion = std::move(v); }
    void setCreadoPor(std::optional<int> v)                     { creado_por = v; }
    void setRevisadoPor(std::optional<int> v)                   { revisado_por = v; }
    void setFechaRevision(std::optional<std::string> v)         { fecha_revision = std::move(v); }
    void setCreatedAt(std::optional<std::string> v)             { created_at = std::move(v); }
    void setUpdatedAt(std::optional<std::string> v)             { updated_at = std::move(v); }
    void setImagenesUrls(const std::vector<std::string>& v)     { imagenes_urls = v; }
    void setGeneroNombre(const std::string& v)                  { genero_nombre = v; }

    // Helpers de la lista legacy de imágenes (intacto en este PR).
    void addImagenUrl(const std::string& imagen_url);
    void removeImagenUrl(const std::string& imagen_url);
    std::string getImagenPrincipal() const;

    // Validación mínima de invariantes locales (longitudes, requeridos).
    // La validación de atributos_especificos por reino vive en otra capa.
    bool esValida() const;

    // Serialización
    nlohmann::json toJson() const;
    static Especie fromJson(const nlohmann::json& j);
};

#endif // ESPECIE_HPP
