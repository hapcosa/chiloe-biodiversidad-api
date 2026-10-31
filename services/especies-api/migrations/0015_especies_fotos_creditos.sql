-- =============================================================================
-- 0015_especies_fotos_creditos.sql
-- =============================================================================
-- Créditos y licencia de las fotos de especies (Fase 8, carga de contenido).
--
-- Las fotos de fuentes abiertas (iNaturalist, Wikimedia Commons) vienen con
-- licencias CC-BY o CC-BY-SA, que obligan a citar al autor y la licencia. Hasta
-- ahora `fotos_keys` guardaba solo la clave de object storage, así que no había
-- dónde dejar esa atribución y publicar la foto la incumpliría.
--
-- Forma: un objeto cuya clave es la key de la foto (la misma de `fotos_keys`):
--   { "especies-fotos/pudu-puda/1.jpg": {
--       "autor": "Aira Faúndez", "licencia": "cc-by",
--       "url_origen": "https://www.inaturalist.org/photos/..." } }
-- Es un objeto y no una columna por foto porque una ficha tiene N fotos, y no
-- viaja dentro de `fotos_keys` para no cambiar el tipo (array de strings) que
-- los clientes ya instalados parsean.
--
-- Una foto sin entrada aquí es propia o de origen no abierto: no se asume
-- ninguna licencia por omisión.
-- =============================================================================

BEGIN;

ALTER TABLE especies
    ADD COLUMN IF NOT EXISTS fotos_creditos JSONB NOT NULL DEFAULT '{}'::jsonb;

COMMIT;
