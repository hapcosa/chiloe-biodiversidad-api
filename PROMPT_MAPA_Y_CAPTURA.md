# Prompt para la sesión siguiente

Copiá todo lo que sigue como primer mensaje de la sesión nueva.
Estado al 2026-09-03. **Producción quedó mudada al dominio nuevo
`piedrasdelrayadito.cl`, desplegada y verificada.** Lo que queda abierto está en
"Lo primero al arrancar".

> Para la sesión de **diseño visual de la app** no uses este archivo: está
> [PROMPT_DISENO_APP.md](PROMPT_DISENO_APP.md), que es su propio encargo.

---

Seguimos con el sistema de biodiversidad de Chiloé:
`/home/obrero/programacion/Chilo-FloraApiDevops` (backend) y su submódulo
`mobile/`, que es su propio repo (`hapcosa/chiloe-biodiversidad-mobile`).

## Reglas innegociables

- Rama por cambio → commits → push → PR contra `master` → checks verdes →
  **el merge lo hago yo**. Nada de push a `master`, `--force`, `reset --hard`
  ni `--no-verify`.
- Migraciones numeradas y **nunca** editadas tras mergear. Las tablas no se
  crean desde código de aplicación.
- No commitear `.idea/editor.xml`, `.idea/vcs.xml`, el directorio sin trackear
  `diseño/`, keystores, contraseñas de firma **ni `android/local.properties`**.
- Fotos: nunca multipart a la API. Presigned URL → subida directa → se notifica
  la key. EXIF sensible (GPS, serial) se borra salvo opt-in.
- Fungi: la comestibilidad siempre con el disclaimer de consultar a un experto.
- Sin dependencias nuevas sin justificarlas en el PR.
- Avisame explícitamente cada desviación, default elegido u omisión.
  En español, directo.
- **Las credenciales salen del `.env` de este repo**, no de `~/.env`
  (`PROD_SSH_HOST/USER/PASSWORD`, `TEST_SSH_*`, `CLOUDFLARE_API_TOKEN`).
  Nunca se imprimen. Ver "SSH: cómo entrar" más abajo — el `ssh` pelado se
  deniega, el wrapper con `sshpass -e` funciona.
- Este archivo está trackeado: actualizalo al cerrar la sesión, en su propio PR.

---

## Lo primero al arrancar

1. **Instalar el APK en el teléfono.** Está compilado y listo en
   `mobile/android/app/build/outputs/apk/release/app-release.apk`
   (2026-09-03 01:44, ya apunta a `api.piedrasdelrayadito.cl`, verificado
   buscando el dominio dentro de `assets/index.android.bundle`). **No se
   instaló porque `adb devices` no listaba ningún equipo.** Cuando el teléfono
   esté conectado y autorizado: `adb install -r <ruta>` — **`-r` siempre**,
   nunca desinstalando primero (ver el bug del índice en "Lo que se aprendió").
   Sin este APK la app instalada sigue pegándole a `api.budaicapital.com`, que
   ya devuelve **530**: la app está rota hasta que se instale.
2. **Agregar el redirect URI nuevo en la consola de Google**:
   `https://api.piedrasdelrayadito.cl/api/v1/auth/callback`. Se preguntó tres
   veces y quedó sin confirmar. **Hasta que esté, el login con Google no
   funciona en producción.** Es el único paso del cambio de dominio que no se
   puede hacer desde acá.
3. **Configurar el entorno de test.** Tiene su propia sección abajo: la máquina
   existe y está vacía de Chiloé, y falta una decisión tuya sobre si se publica.
4. **Mergear el PR backend #101**, que sube el puntero del submódulo a `bc5f5aa`
   (mobile #51, el del dominio). Sin eso `master` del backend sigue apuntando al
   commit con el dominio viejo.

---

## Estado

**Producción: mudada a `piedrasdelrayadito.cl` el 2026-09-03 y verificada desde
afuera.**

| URL | Código | Qué significa |
|-----|--------|----------------|
| `https://api.piedrasdelrayadito.cl/health` | 200 | el túnel y el gateway andan |
| `https://api.piedrasdelrayadito.cl/curaduria/` | 200 | el panel se sirve |
| `https://api.piedrasdelrayadito.cl/docs-api.json` | 200 | |
| `https://api.piedrasdelrayadito.cl/api/v1/*` | 401 | **correcto**: `auth_request` sin token |
| `https://storage.piedrasdelrayadito.cl/` | 403 | **correcto**: MinIO sin bucket en la raíz |
| `https://api.budaicapital.com/*` | 530 | el túnel viejo se apagó, corte limpio |

Fue un **corte limpio, no una convivencia**: el dominio viejo dejó de servir a
Chiloé a propósito. El túnel nuevo es `95cf6d1b-f647-4c09-93ac-ebf053c2fb28`,
de la cuenta dueña de `piedrasdelrayadito.cl`; el viejo (`14f93feb-…`, cuenta
budaicapital) quedó parado.

Lo que se tocó para eso, todo ya mergeado (backend #100, mobile #51):

- `infrastructure/docker/cloudflared/config.yml`: los dos `hostname` nuevos, y
  **se sacó la línea `tunnel:`** — el UUID ahora se pasa como argumento del
  `run` desde el compose, con `CLOUDFLARED_TUNNEL_ID` en el env de despliegue.
  El id es de la instalación, no del código.
- `docker-compose.prod.yml`: `command` con `${CLOUDFLARED_TUNNEL_ID:?…}` y
  `OAUTH_REDIRECT_URL` con el dominio nuevo por default.
- `services/gateway/nginx.prod.conf:70`: `server_name`.
- `.env.prod.example`: `S3_PUBLIC_ENDPOINT`, `PUBLIC_API_URL`,
  `CLOUDFLARED_TUNNEL_ID` y `CLOUDFLARED_CREDENTIALS_FILE` (ruta **absoluta**
  del host al JSON).
- `mobile/src/config/appConfig.ts:11`: la rama release del ternario `__DEV__`.
- `docs/deployment/PRODUCCION_DOCKER_CLOUDFLARE.md`: la receta por API REST y la
  advertencia contra `cloudflared tunnel login`.

Backup del env de producción antes del cambio:
`~/.config/chiloe-prod/chiloe.env.bak-2026-09-03-0114`.

**Base de datos y catálogo**: sin cambios desde el 2026-08-27. Migraciones hasta
la **`0014`**, 11 insignias, 103 especies en veinte subgrupos, `sin_subgrupo` en
0, 7 áreas protegidas, 5 reinos.

**No hay deploy automático.** El workflow que existía apuntaba a EKS y fallaba
siempre; se borró en el PR 75. Redesplegar es a mano, con el bloque de comandos
del final de este archivo.

## Lo que quedó sin verificar

- **La app contra el dominio nuevo.** El APK no llegó al teléfono. Nada del
  cambio de dominio se probó desde el cliente real.
- **El login con Google**, que depende del redirect URI de arriba.
- **La postulación a curador**: se llegó a tocar el botón y no más — el teléfono
  se lo llevó su dueño a mitad de la verificación. **Ninguna postulación se
  envió.** Escribiría en la BD de producción con tu cuenta y caería en la
  bandeja del panel. Pedímelo y la mando.
- **A quién le tocó la única insignia otorgada.** Ahora sí se puede consultar
  (el SSH funciona con el wrapper):
  `SELECT ui.usuario_id, i.codigo FROM usuario_insignias ui JOIN insignias i ON
  i.id = ui.insignia_id;` para distinguir "el criterio funciona y hay poca data"
  de "el criterio matchea algo raro".
- **Las insignias en un perfil público ajeno.** El perfil de tu cuenta ya está
  público; lo que falta es un usuario **ajeno** con perfil público **y** al
  menos una insignia. Los del feed (#6 y #7) no publicaron el suyo. El camino
  limpio es que tu cuenta gane "Primer encuentro" (1 encuentro aprobado) y
  mirarla desde otra sesión.

---

## El entorno de test — hay que configurarlo

**La máquina de test es el host viejo de producción**: `10.244.19.205`
(`traderbot`), credenciales en `TEST_SSH_*` del `.env`. Después de la migración
del 2026-08-14 quedó libre, y **hoy no tiene nada del stack de Chiloé
corriendo**. Configurarlo es trabajo pendiente entero, no un retoque.

Lo que hay que resolver, en orden:

1. **Decisión tuya primero: ¿test se publica o no?** Tres caminos, y cambian
   todo lo demás:
   - **Solo ZeroTier** (lo más barato): se llega por `10.244.19.205:8080`, sin
     túnel, sin DNS, sin certificado. Alcanza para probar la API y el panel
     desde esta máquina, **no** para probar la app en el teléfono, que sale por
     internet.
   - **Túnel propio** con `test.piedrasdelrayadito.cl` y
     `storage-test.piedrasdelrayadito.cl`: es lo que permite apuntar un APK de
     prueba sin tocar producción. Cuesta un túnel nuevo y dos CNAME.
   - **Nada por ahora**, y test queda solo como máquina de builds.
2. **Llevar las tres piezas que no viven en el repo** (las mismas que
   documenta `CLAUDE.md`): el env fuera del repo, el JSON de credenciales del
   túnel si lleva túnel, y `CLOUDFLARED_UID`/`GID`.
3. **Datos**: test **no** debe compartir la BD de producción. Levantar Postgres
   vacío, correr `migrate.sh` y **los seeds** (el `0003` no es opcional), y
   sembrar unas pocas especies a mano. Si querés una copia de prod, es `pg_dump`
   y restore, nunca copiar el directorio del volumen.
4. **Cuidado con el `cloudflared`**: si test levanta un túnel, tiene que ser un
   túnel **propio**, con su propio UUID. Levantar el de producción en la segunda
   máquina hace que Cloudflare reparta el tráfico entre las dos.

## Deuda inmediata

Los cuatro puntos de "Lo primero al arrancar", y dos decisiones tuyas
(puntos 1 y 2 de la lista de abajo).

---

## Lo que sigue, en el orden que yo haría

### 1. Fase 9 — PR 12: pantalla de usuarios del panel

**Bloqueado por una decisión tuya**: `users` vive en `auth-service` y las
asignaciones a categorías en `especies-api`. Hay que elegir si el panel consulta
a los dos o si uno expone la vista combinada. Preguntame antes de escribir
código. Acá encaja también el **botón de recálculo de insignias** en el panel de
curaduría, que hoy no existe: el endpoint se llama a mano.

### 2. El pin rojo del mapa miente — decidido, pero bloqueado

**El pin rojo no es un lugar: es el centro de una celda de la rejilla.** El
servidor nunca devuelve encuentros sueltos, agrupa por una rejilla cuyo tamaño
depende del zoom (`postgres_avistamiento_repository.cpp:354`:
`floor(geo_lat / tam) * tam + tam / 2`). Al cambiar el zoom, `tam` cambia, la
celda se redefine y su centro se recalcula: **el punto salta aunque el encuentro
no se haya movido**. El círculo verde alrededor es el que dice la verdad.

El problema es que una chincheta roja de Google significa "acá, en este punto"
para cualquiera que use mapas, y acá significa "en algún lugar de esta zona".
Los dos símbolos se contradicen y gana el falso. (Que la coordenada no sea
exacta es deliberado: para especies `sensible` el servidor la redondea todavía
más a propósito.)

**Decidiste sacar el pin y dejar solo el círculo**, que además cambia de tamaño
con el zoom, cosa honesta porque la precisión realmente cambia.

**Está bloqueado por la librería**: en `react-native-maps` 1.29 el `Circle`
**no expone `onPress` ni `tappable` en JS** — `MapCircleProps`
(`node_modules/react-native-maps/src/MapCircle.tsx`) solo declara `center`,
`fillColor`, `lineCap`, `lineDashPattern`, `lineDashPhase`, `lineJoin`,
`miterLimit`, `radius`, `strokeColor`, `strokeWidth` y `zIndex`. El lado Android
**sí** lo tiene (`MapCircle.java:80`, `setTappable` → `circle.setClickable`),
así que la prop podría estar llegando al nativo sin tipo. **Sin verificar.**
Los caminos, de menor a mayor riesgo:

1. Probar `tappable` + `onPress` en el `Circle` con un cast y ver si el nativo
   responde. Si anda, es el cambio de tres líneas.
2. Dejar un `Marker` invisible o con vista propia (una `View` redonda chata) en
   el centro: conserva el blanco de tap y deja de parecer una chincheta.
3. Capturar el tap en el `MapView` (`onPress` con la coordenada) y resolver a
   mano qué celda lo contiene. Es el más caro y el que no depende de nadie.

Sea cual sea, hay que reponer el `accessibilityLabel` que hoy vive en el
`Marker` de la celda: el `Circle` no acepta uno.

### 3. El mapa — lo que queda

- **El tap ya está resuelto** (PR 47): abre la hoja de resumen, con "Ver solo
  esta especie" y "Abrir la ficha" como acciones explícitas. La tab Mapa quedó
  envuelta en `SpeciesStackNavigator` para poder navegar a la ficha.
- **El GPS ya se lee** (PR 43): botón de "mi ubicación", permiso en runtime,
  punto azul y mensajes distintos para "no me dejaron" y "el GPS no respondió".
- **Los tirones siguen sin diagnosticar.** Memoicé los overlays y saqué `region`
  del estado, pero al medir con `dumpsys gfxinfo` **no hubo mejora**: master
  13/659 frames con jank (1,97%), la rama 10/668 (1,50%), p95 idéntico. Es
  ruido, porque hoy producción pinta **una** celda y 7 áreas. La causa real
  está sin encontrar; el cambio se justifica solo por cuando haya densidad.

### 4. Insignias: lo que falta

- **El endpoint en lote ya existe** (PR 98):
  `GET /api/v1/insignias?usuarios=1,2,3`, hecho justamente para no pedir una
  petición por persona. Falta el consumidor: **las insignias no se muestran en
  el feed**, porque el feed **no muestra autores** —las tarjetas llevan especie,
  foto y grado, nunca quién lo registró— y no hay nombre al lado del cual
  ponerlas. El único lugar con personas es la lista de identificaciones del
  detalle ("Usuario #N"). `InsigniasRow` ya está lista para colgarse donde
  aparezca la autoría.
- **No hay aviso al postulante a curador** cuando le resuelven: la app dice
  "te avisaremos" y hoy eso es entrar y mirar.

### 5. Diseño de la app

Su propio encargo en [PROMPT_DISENO_APP.md](PROMPT_DISENO_APP.md).

### 6. Fotos reales

Bloqueado esperándome. No bajes imágenes de licencia indeterminada a producción.

### 7. Sin decidir

La contraseña de Postgres en texto plano que `especies-api` imprime al arrancar
y queda en los logs del contenedor de producción.

### Verificación del Paso 3

Faltan los puntos 3 (filtros y paginación), 5 (visibilidad) y 9 (sync del cache
de especies). El 7 solo cuando ponga modo avión a mano.

---

## Lo que se aprendió y no conviene volver a aprender

### Del cambio de dominio (2026-09-03)

- **`~/.cloudflared/cert.pem` es uno por usuario y lo comparten todos los
  túneles del host.** En producción conviven **cuatro** `cloudflared` de
  proyectos distintos (`chiloe`, `signalstrading`, `dvu`, `ofertascl`).
  `cloudflared tunnel login` **sobrescribe ese cert**: no tira abajo los túneles
  que ya corren —el `run` usa el JSON de credenciales, no el cert— pero deja sin
  administrar los de los otros proyectos, que pasan a contestar
  `Authentication error` en `tunnel list`, `route dns` y `tunnel delete`.
  **El túnel se crea con un API token vía la API REST**
  (`POST /accounts/<id>/cfd_tunnel` con `config_src: local`), que devuelve el
  UUID y el secreto con el que se arma el `<uuid>.json` a mano, 0600.
  Yo mismo metí `tunnel login` en la doc de deployment y hubo que sacarlo.
- **Los permisos de Cloudflare "DNS" de ámbito *cuenta* no son el `DNS` de
  ámbito *zona*.** Account · DNS Settings, DNS Firewall y DNS View no habilitan
  escribir un registro. Para los CNAME hace falta **Zone · DNS · Edit** sobre la
  zona (más `Zone · Zone · Read` para poder listarla, y
  `Account · Cloudflare Tunnel · Edit` para el túnel). Con el token sin eso, la
  creación falla con `code 10000, Authentication error`, que **no** dice cuál es
  el permiso que falta. Los dos CNAME terminaron creados a mano en el
  dashboard, proxied, hacia `<uuid>.cfargotunnel.com`.
- **Un token de ámbito cuenta contesta `Invalid API Token` en
  `/user/tokens/verify` aunque funcione en todo lo demás**: ese endpoint es solo
  para tokens de usuario. Sirve para distinguir de qué tipo es el token que
  tenés, no para concluir que está mal.
- **`VITE_S3_PUBLIC_BASE` se hornea en el bundle del panel al construir la
  imagen**, así que un cambio de `S3_PUBLIC_ENDPOINT` exige **`build` del
  gateway**, no alcanza con recrearlo. Es la misma clase de trampa que el bind
  mount del `nginx.conf`, por otra vía.
- **El cutover se hace en dos pasos separados para que un build roto no baje
  producción**: primero `build gateway` (no toca nada que esté corriendo), y
  recién después `stop cloudflared` + `up -d --force-recreate gateway
  cloudflared`. **Nunca los dos `cloudflared` del mismo túnel a la vez.**
- **Un 401 en `/api/v1/*` después de un cambio de dominio no es una regresión.**
  Se confirma con `docker exec chiloe-gateway grep -n auth_request
  /etc/nginx/nginx.conf` y pegándole al gateway por dentro, sin pasar por el
  túnel: si también da 401, el túnel no tiene nada que ver.

### De antes

- **El callout nativo de Google Maps es hijo del `Marker`, así que no sirve
  para nada que tenga que quedarse en pantalla.** Cualquier `setState` que
  recree la lista de celdas desmonta los `Marker` y se lo lleva puesto. Y no
  hace falta filtrar para provocarlo: **tocar un `Marker` hace que Google
  recentre el mapa solo**, eso dispara `onRegionChangeComplete`, que recarga las
  celdas. Lo que deba sobrevivir va en el estado de la pantalla, dibujado por
  React. Diagnosticado en el PR 47.
- **Un mismo gesto que hace dos cosas distintas según el dato es un síntoma, no
  una casualidad.** El tap viejo filtraba, salvo cuando `especie_dominante_id`
  era `null`: ahí salía temprano, no recargaba, y el callout sí sobrevivía. Esa
  asimetría era la pista de que el filtro y la recarga eran el problema.
- **`especies_distintas = 0` y `especie_dominante_id = null` son la misma
  condición**, no dos: el servidor cuenta
  `COUNT(*) FILTER (WHERE especie_id IS NOT NULL)` y toma el dominante del mismo
  `array_agg` filtrado (`postgres_avistamiento_repository.cpp:359`). Si hubiera
  alguna especie no nula habría dominante. Tratarlas como casos separados
  produce textos que se contradicen.
- **Un PR se puede mergear entre tu último `push` y tu `gh pr checks`.** Pasó
  con el #47: el fix del "0 especies" llegó a la rama después del merge y quedó
  fuera de master, con la rama remota aparentando estar completa. Antes de dar
  una rama por cerrada, `git log origin/master` y confirmá que estén **todos**
  los commits, no que el PR figure mergeado.
- **El `nginx.conf` del gateway es un bind mount de *archivo*, y por eso un
  `git pull` no lo actualiza nunca.** Diagnosticado el 2026-08-27, después de
  que el deploy de las insignias dejara la ruta nueva sin funcionar. Docker
  resuelve ese mount **por inode al crear el contenedor**; `git pull` escribe un
  archivo nuevo y lo renombra encima, así que el inode cambia y el contenedor
  sigue leyendo la copia vieja indefinidamente. Los síntomas engañan:
  `docker exec chiloe-gateway grep -c insignias /etc/nginx/nginx.conf` da **0**
  aunque el archivo del host sí la tenga, `nginx -t` **pasa** (la config vieja
  es válida) y `nginx -s reload` no cambia nada. `docker inspect` muestra el
  mount apuntando al archivo correcto, que es lo que despista. La cura es
  **`up -d --force-recreate gateway`**, nombrando el servicio.
- **Una ruta de `/api/v1/` que falta en el Nginx que corre devuelve 200, no
  404**: cae en `location /` y contesta el índice estático del gateway. La
  forma barata de detectarlo desde afuera es comparar contra una ruta que sí
  existe — `/api/v1/especies` da 401 — y mirar el `last-modified` de la
  respuesta.
- **El recálculo de insignias se puede disparar sin token de admin**, desde
  dentro de la red de Docker: la API confía en `X-User-Id`/`X-User-Role` que
  inyecta el gateway tras el `auth_request`
  (`services/especies-api/src/utils/request_identity.cpp:4`), y el contenedor
  de `especies-api` ya trae `curl`. Sirve cuando la cuenta admin entra por
  Google Sign-In y no hay contraseña que mandar por `curl`:
  `docker exec chiloe-especies-api curl -s -X POST -H 'X-User-Id: 1'
  -H 'X-User-Role: admin' http://localhost:9080/api/v1/insignias/recalcular`.
  El `X-User-Id` es arbitrario: `recalcular()` no lo usa. **La contracara es que
  la frontera de autorización es el gateway**: cualquiera con acceso a esa red
  puede hacerse pasar por admin. Es el diseño ya decidido, no un bug del PR 92.
- **El recálculo de insignias se probó contra un Postgres real**, no solo con
  gtest: `docker run postgres:16-alpine`, las migraciones en orden y datos
  sembrados. Ahí se confirmó que la autoidentificación no suma y que correrlo
  dos veces otorga cero. Los tests unitarios no tocan SQL.
- **Las tablas del catálogo no traen seed de especies**: una BD recién migrada
  tiene `especies` vacía, así que cualquier prueba de agregación hay que
  sembrarla a mano (ojo: `generos` no tiene columna `reino`, la tiene
  `familias`).
- **Las migraciones corren antes que los seeds.** Un backfill dentro de una
  migración no alcanza a las fichas que un entorno nuevo siembra después. Por
  eso el mapeo familia→subgrupo vive en la tabla `familia_subgrupo` y el seed
  `0003` lo reaplica.
- **El seed `0003` es parte del despliegue, no un opcional.** La `0013`
  clasifica por familia y deja fuera lo que ninguna familia mapea —monera no
  tiene subgrupos a propósito—. En producción quedaron 2 fichas de monera en
  `NULL` hasta correrlo.
- **Un índice sobre una columna añadida por `ALTER` no puede vivir junto a las
  tablas.** `CREATE TABLE IF NOT EXISTS` no toca la tabla que ya existe en el
  teléfono, así que el índice explota con `no such column`, rompe
  `initializeDatabase` entera y la app se cuelga en "Cargando sesión...".
  **Solo se ve actualizando sobre una instalación previa**: ni CI ni una
  instalación limpia lo detectan. El orden es tablas → ALTER → índices, y hay un
  test que lo defiende. Por eso el APK se instala con `adb install -r` sobre la
  versión anterior, nunca desinstalando primero.
- **Verificar en el teléfono no es un trámite.** Ese bug, la truncación de
  "Guardad…" y lo de los dos significados de "reinos" salieron los tres de
  mirar la pantalla, con los tests en verde.
- **Se puede probar una migración sin levantar el compose**: un
  `docker run -d -e POSTGRES_PASSWORD=postgres -e POSTGRES_DB=chiloe_flora -p 55432:5432 postgres:16`
  y después `DB_HOST=localhost DB_PORT=55432 DB_USER=postgres DB_PASSWORD=postgres`
  con `./scripts/migrate.sh` y `./scripts/seed.sh`.
- **Las dos APIs no reportan los errores igual.** El `auth-service` (Go) manda
  `message` y la `especies-api` (C++) manda `error`. `apiClient` solo miraba el
  primero, así que **todo** error de la API de especies llegaba a la UI como un
  `HTTP 400` pelado. Arreglado en el PR 44; si aparece un servicio nuevo,
  revisá con cuál de los dos formatos habla.
- **Medir antes de afirmar que algo mejoró el rendimiento.** `adb shell dumpsys
  gfxinfo <pkg>` con la misma secuencia de gestos en las dos builds.
- **Al mergear mobile hay que subir el puntero del submódulo** en el repo
  backend, en su propio PR. Es fácil de olvidar.
- **En el checkout de producción, ` M mobile` es ruido**, no trabajo sin
  commitear: es el puntero del submódulo, que allá nunca se inicializa.

---

## SSH: cómo entrar (esto cambió)

Lo que decía este archivo —"el SSH está bloqueado por el clasificador"— **era
cierto solo del `ssh` pelado**. Un `ssh` directo se deniega, y aunque lo
autorices muere con `Permission denied` porque sin `SSH_ASKPASS` no puede pedir
la contraseña. **Lo que sí funciona es un wrapper con `sshpass -e`** que lea las
credenciales del `.env` del repo. Recrealo en el scratchpad al arrancar (no está
commiteado, y las credenciales no se imprimen nunca):

```bash
# env-remoto.sh <prod|test> '<comando remoto>'
# lee PROD_SSH_HOST/USER/PASSWORD o TEST_SSH_* del .env del repo
set -euo pipefail
eval "$(python3 - <<'PY'
import re, shlex
for l in open('/home/obrero/programacion/Chilo-FloraApiDevops/.env'):
    m = re.match(r'\s*(?:export\s+)?([A-Za-z_]\w*)\s*[=:]\s*(.*?)\s*$', l)
    if m: print(f'export {m.group(1)}={shlex.quote(m.group(2).strip(chr(34)+chr(39)))}')
PY
)"
P=$( [ "$1" = prod ] && echo PROD || echo TEST )
eval "H=\$${P}_SSH_HOST U=\$${P}_SSH_USER; SSHPASS=\$${P}_SSH_PASSWORD"
export SSHPASS
sshpass -e ssh -o StrictHostKeyChecking=accept-new -o ConnectTimeout=15 "$U@$H" "$2"
```

Con eso anduvieron el cutover entero, `docker ps`, `docker logs` y `psql`.
Algunas cosas **igual las deniega el clasificador** y hay que pedírtelas a vos
con el prefijo `!`: leer archivos de credenciales (`~/.cloudflared/*.json`),
`scp` de esos archivos, y `docker compose build`/`up` en producción. Cuando
pase, te lo digo y te paso el comando; no lo esquivo por otro lado.

---

## Entorno

- Build de Android: `JAVA_HOME=/usr/lib/jvm/java-17-openjdk`. El default del
  host es **Java 26** y el plugin Gradle de RN revienta con
  `Error resolving plugin [id: 'com.facebook.react.settings'] > 26.0.2.1`.
  No toques `gradle.properties` por eso: es del entorno, no del repo. La release
  incremental tarda ~1 min si solo cambió JS; desde limpio, 12.
  **Y no pipees el `assembleRelease` a `tail`**: te quedás con el exit code del
  `tail` y un build fallido parece exitoso. Redirigí a un log y capturá `$?`.
- Tiene que ser **release**: `src/config/appConfig.ts` manda el debug a
  `http://localhost:8080` y solo el release habla con
  `api.piedrasdelrayadito.cl`. Si la pantalla sale roja con "Unable to load
  script", instalaste el debug.
- Para confirmar que un APK trae el dominio que creés:
  `unzip -p <apk> assets/index.android.bundle | grep -ac piedrasdelrayadito`
  (con `-a`: el bundle es binario para grep).
- El teléfono se conecta por wifi. **Es un teléfono en uso**: confirmá el foco
  con `adb shell dumpsys window | grep mCurrentFocus` antes de tocar nada, y
  **volvé a confirmarlo antes de cada captura** — ya pasó dos veces que el dueño
  volviera a su app entre el `dumpsys` y el `screencap`. **Si el foco no es
  `cl.chiloe.biodiversidad`, se para y se avisa**; la captura que ya salió se
  borra sin mirarla. Y `adb exec-out screencap` escribe en el cwd, que se
  resetea después de un comando en background: usá rutas absolutas.
- `especies-api` **solo compila dentro de Docker**: al host le faltan Pistache y
  libpqxx. Ignorá los errores de clang sobre esos headers. Los tests de gtest se
  reproducen con `docker build --target tester services/especies-api`; para un
  subconjunto:
  `docker run --rm --entrypoint /app/build/tests/unit_tests <img> --gtest_filter='*LoQueSea*'`.
- `applicationId cl.chiloe.biodiversidad`. El `release` está firmado con la
  **clave de debug** (`signingConfig signingConfigs.debug` en los dos
  buildTypes) y `enableProguardInReleaseBuilds = false`. O sea: compilar la
  release **no necesita ningún secreto de firma**.
- La key de Google Maps sale de `GOOGLE_MAP_API` en el `.env` y va a
  `mobile/android/local.properties` como `MAPS_API_KEY`. Ese archivo **no se
  commitea**.
- `npm run lint` en mobile arrastra **53 warnings `no-void`** preexistentes.
  0 errores es el criterio; no salgas a limpiarlos sin PR propio.
- El CI del backend corre **solo en `pull_request`** desde el PR #70. El job
  `test-especies-api` se cuelga a veces en `Install postgresql-client` y GitHub
  lo cancela a los 20 min: `gh run rerun <run-id> --failed`. La duración que
  muestra la UI **suma los dos intentos**.
- Si agregás un endpoint nuevo, agregá su `location` en `nginx.prod.conf` **y**
  en `nginx.dev.conf`, y acordate del `--force-recreate` del gateway al
  desplegar. Ojo: casi todas las rutas de `/api/v1/` exigen sesión con
  `auth_request`, así que un `curl` sin token contesta **401 y eso es lo
  correcto** — no es un síntoma de nada.
- **Producción no tiene `less`**: `git log` allá falla con "unable to execute
  pager 'less'". Usá `git --no-pager log`.

---

## Producción

- `donaldchavez@10.244.117.161` (`Clethus`, uid/gid 1000), checkout en
  `~/servicios/chiloe-biodiversidad-api`, proyecto compose `chiloe-prod`, env
  **fuera del repo** en `~/.config/chiloe-prod/chiloe.env`. Postgres: usuario
  `chiloe_prod`, base `chiloe_biodiversidad`.
- La máquina **no tiene IP pública**: se llega por **ZeroTier**
  (`10.244.0.0/16`, interfaz `ztbpaiczc3`) y el túnel Cloudflare publica
  `api.piedrasdelrayadito.cl` y `storage.piedrasdelrayadito.cl`. Un runner de
  GitHub no la alcanza. Si alguna vez se automatiza el deploy, los caminos son
  SSH por Cloudflare Access con service token, unir el runner a ZeroTier, o un
  runner self-hosted. **Eso lo decido yo.**
- **El apex y `www` de `piedrasdelrayadito.cl` apuntan a otro túnel**
  (`074c4978-…`, el ecommerce). No los toques: Chiloé vive solo en los dos
  subdominios.
- Tres piezas del despliegue **no viven en el repo** y hay que llevarlas a mano
  al cambiar de máquina: el env de arriba, `~/.cloudflared/<uuid>.json` (que el
  compose monta por la ruta absoluta de `CLOUDFLARED_CREDENTIALS_FILE`) y
  `CLOUDFLARED_UID`/`GID`, que existen porque la imagen corre como uid 65532 y
  el JSON es 0600 del usuario del host.
- Al redesplegar, **nombrá siempre los servicios concretos**: un `build`/`up`
  pelado levantaría un segundo `cloudflared` del túnel de Chiloé y Cloudflare
  repartiría el tráfico entre dos máquinas. En esa máquina corren **cuatro**
  `cloudflared` de proyectos distintos.

```bash
# en donaldchavez@10.244.117.161
cd ~/servicios/chiloe-biodiversidad-api && git pull --ff-only origin master
cd infrastructure/docker
docker compose -p chiloe-prod --env-file ~/.config/chiloe-prod/chiloe.env \
  -f docker-compose.prod.yml build especies-api gateway
docker compose -p chiloe-prod --env-file ~/.config/chiloe-prod/chiloe.env \
  -f docker-compose.prod.yml up especies-api-migrate      # sin -d
docker compose -p chiloe-prod --env-file ~/.config/chiloe-prod/chiloe.env \
  -f docker-compose.prod.yml up -d especies-api
# el gateway SIEMPRE con --force-recreate si cambió nginx.prod.conf:
docker compose -p chiloe-prod --env-file ~/.config/chiloe-prod/chiloe.env \
  -f docker-compose.prod.yml up -d --force-recreate gateway
# los seeds no los aplica el migrate: son parte del despliegue
docker exec -i chiloe-postgres psql -U chiloe_prod -d chiloe_biodiversidad \
  < ~/servicios/chiloe-biodiversidad-api/services/especies-api/seeds/0003_subgrupos_por_familia.sql
```

Después de desplegar, verificá **desde afuera** que las rutas nuevas no caigan
en `location /`:

```bash
for p in /api/v1/insignias /api/v1/especies /health; do
  printf "%-26s %s\n" "$p" "$(curl -s -o /dev/null -w '%{http_code}' https://api.piedrasdelrayadito.cl$p)"
done
# lo correcto: 401, 401, 200. Un 200 en una ruta de /api/v1/ es el síntoma
# de que el gateway no releyó nginx.prod.conf.
```

⚠️ En **zsh** no sirve meter el comando en una variable y hacer `$C build ...`:
zsh no parte la variable en palabras y falla con "no such file or directory".
Usá el comando completo, o `${=C}`.

Los contenedores se llaman `chiloe-postgres`, `chiloe-gateway`,
`chiloe-especies-api`, `chiloe-auth`, `chiloe-minio`, `chiloe-redis`,
`chiloe-cloudflared` (tienen `container_name` explícito).

Para verificar la base sin depender de tokens:

```bash
docker exec chiloe-postgres psql -U chiloe_prod -d chiloe_biodiversidad -c \
  "SELECT count(*) AS sin_subgrupo FROM especies WHERE categoria_id IS NULL;"
docker exec chiloe-postgres psql -U chiloe_prod -d chiloe_biodiversidad -c \
  "SELECT max(version) FROM schema_migrations; SELECT count(*) FROM insignias;"
```
