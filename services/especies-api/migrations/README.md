# Migraciones SQL — especies-api

Sistema de migraciones del esquema PostgreSQL del servicio `especies-api`. Las migraciones son **la única fuente de verdad** del schema; el código C++ ya no crea tablas.

## Layout

```
services/especies-api/
├── migrations/
│   ├── 0001_initial.sql       # baseline (extraído del antiguo initDatabase())
│   ├── 0002_multi_reino.sql   # modelo multi-reino
│   ├── 0003_avistamientos.sql # registros subidos por la app móvil
│   ├── 0004_categorias_moderacion.sql # categorías de curaduría y sus moderadores
│   ├── 0005_postulaciones_curador.sql  # postularse a curador; un admin resuelve
│   ├── 0006_especies_estado.sql # borrador/publicada + firma de publicación
│   ├── 0007_identificaciones.sql # identificación comunitaria de avistamientos
│   ├── 0008_avistamientos_visibilidad.sql # público/privado por avistamiento
│   ├── 0009_normaliza_atributos_seed.sql  # corrige atributos del seed 0001 ya insertados
│   ├── 0010_avistamientos_precision_declarada.sql # cuánto se fía el autor de la ubicación
│   ├── 0011_avistamientos_indice_espacial.sql # índice del bbox del mapa
│   ├── 0012_areas_protegidas.sql             # parques y áreas protegidas + seed
│   ├── 0015_especies_fotos_creditos.sql      # autor y licencia por foto
│   └── README.md              # este archivo
└── scripts/
    └── migrate.sh             # runner
```

## Convenciones

- Nombre del archivo: `00NN_descripcion_corta.sql`, 4 dígitos zero-padded, snake_case.
- Numeración estricta y monótona. Cuatro dígitos para mantener orden lexicográfico (`0001` < `0010` < `0100`).
- Cada migración envuelta en `BEGIN; … COMMIT;` para rollback automático en fallo.
- Una migración **nunca se edita** una vez mergeada. Si hay que corregir algo, se crea una migración nueva.
- Una migración debe ser **idempotente cuando sea posible**: `CREATE TABLE IF NOT EXISTS`, `ALTER TABLE … ADD COLUMN IF NOT EXISTS`. Esto facilita re-runs accidentales.
- No usar funciones definidas en el código C++ ni ORM-specific. SQL crudo.
- Sin `DROP TABLE` salvo en cleanup explícito acordado en PR aparte.

## Cómo se aplican

### Local (host)

```bash
# Variables de entorno con la conexión a Postgres:
export DB_HOST=localhost DB_PORT=5432 DB_NAME=chiloe_flora_dev \
       DB_USER=dev_user DB_PASSWORD=dev_password

./services/especies-api/scripts/migrate.sh
```

### Local (docker-compose dev)

```bash
make dev   # arranca postgres + especies-api-migrate + especies-api
```

El servicio `especies-api-migrate` corre el script y termina con `exit 0`. `especies-api` lo espera con `depends_on: service_completed_successfully` antes de arrancar el binario C++.

### CI (GitHub Actions)

El job `test-especies-api` levanta un Postgres efímero y corre `migrate.sh` como step antes de compilar. Si una migración falla, el job rojo.

### Producción (k3s)

Pendiente Fase 7: un `Job` de Kubernetes que corre antes del rollout del Deployment de `especies-api`, con la imagen `postgres:15` montando el ConfigMap/SHA de las migraciones.

## Tracking

El script crea (si no existe) la tabla:

```sql
CREATE TABLE schema_migrations (
    version    VARCHAR(255) PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

Cada migración aplicada inserta una fila con `version = nombre del archivo sin .sql`. Re-runs del script saltan las versiones ya presentes.

## Crear una migración nueva

1. `git checkout -b feat/migracion-XXX`
2. Crear `services/especies-api/migrations/00NN_descripcion.sql`. Tomar el siguiente número libre.
3. Probar localmente con `./scripts/migrate.sh` contra el postgres dev.
4. Si rompes algo, **no edites la migración**; revierte el cambio en la BD a mano (la transacción debió haber hecho rollback automáticamente) y arregla el SQL.
5. Push + PR. CI levanta su propio Postgres y verifica.

## Qué NO va en una migración

- Datos de negocio (especies, familias). Eso va en seeds aparte, no en `migrations/`.
  La excepción es **corregir filas que un seed ya insertó**: editar el seed no
  toca las BD pobladas (`ON CONFLICT DO NOTHING`), así que la corrección viaja en
  una migración de datos con `UPDATE` idempotente —filtrada por el valor
  defectuoso— como `0009_normaliza_atributos_seed.sql`. Contenido *nuevo* sigue
  yendo a `seeds/`.
- Lógica condicional dependiente del entorno (dev/prod). Las migraciones deben producir el mismo schema en todas partes.
- Operaciones costosas sin pensar (FULL TABLE LOCKS sobre tablas grandes en prod). Cuando haya datos reales, evaluar con `CONCURRENTLY` o batched updates.
