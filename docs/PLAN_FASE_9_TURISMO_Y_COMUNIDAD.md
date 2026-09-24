# Fase 9 — Turismo responsable, mapa y comunidad

Plan de trabajo derivado de la sesión del 2026-08-18. Complementa
[PLAN_MAESTRO.md](PLAN_MAESTRO.md); las decisiones de arquitectura que salgan de
aquí se anotan en su §10 en el mismo PR que las implemente.

**Hilo conductor**: la app deja de ser un catálogo con extras y pasa a ser una
guía de campo para el visitante estival de Chiloé, con un sesgo explícito hacia
la observación sin molestar.

---

## 0. Lo que ya está y lo que no

Verificado en el repo y contra producción el 2026-08-18.

| Pregunta | Respuesta |
|---|---|
| ¿El panel de curaduría/moderación está listo? | **Sí, y está arriba.** `https://api.piedrasdelrayadito.cl/curaduria/` responde 200. Login, listado y edición de fichas con formulario generado desde el JSON Schema, publicar/despublicar, bandeja de postulaciones a curador (solo `admin`) y bandeja de avistamientos (`admin`/`moderator`). Se compila dentro de la imagen del gateway; no es un servicio con puerto propio. |
| ¿Qué son las "24 descubiertas"? | **Fichas que abriste**, no encuentros. `BibliotecaScreen` llama `markSpeciesViewed(id)` al tocar un ítem, y el perfil cuenta esas filas. Registrar un encuentro también marca la especie como vista, pero el número está dominado por la simple navegación. |
| ¿"Actualizar perfil" hace algo? | **Casi nada.** Llama `authApi.whoami()` y refresca los datos que el servidor ya tiene. No edita nada. El endpoint de edición (`PUT /api/v1/auth/me`) existe y solo acepta `name` y `avatar`. |
| ¿Hay mapa? | No. No hay ninguna dependencia de mapas en `mobile/package.json`. |
| ¿Hay insignias, bio de usuario, profesión de moderador, parques? | No, nada de eso existe todavía. |
| ¿Cómo postula un usuario a curar una categoría? | **No puede.** El backend tiene `POST /api/v1/postulaciones` y el panel tiene la bandeja para que un `admin` apruebe o rechace, pero **no existe la interfaz para postular**: `mobile/src/` no menciona postulaciones en ningún archivo. La bandeja del panel solo puede llenarse a mano contra la API. |
| ¿Puede un `admin` asignar una categoría a alguien directamente? | Solo por API. `POST /api/v1/categorias/:id/moderadores/:usuarioId` y su `DELETE` existen y son admin-only, pero el panel no tiene pantalla que los llame ni listado de usuarios. La única vía por interfaz es aprobar una postulación… que nadie puede crear. |

---

## 1. Decisiones tomadas

| # | Decisión | Alternativas descartadas | Por qué |
|---|---|---|---|
| A | **`react-native-maps` con el proveedor Google**, en modo satelital | MapLibre + Esri World Imagery; MapLibre + MapTiler | La imagen satelital de Google sobre Chiloé es la mejor de las tres, y el proyecto de Google Cloud ya existe por Google Sign-In: habilitar *Maps SDK for Android* y emitir una key restringida por firma de app es trámite, no infraestructura nueva. El crédito mensual gratuito de Google cubre de sobra el volumen de esta app. Trae clustering y heatmap sin librerías extra. |
| B | **Se ataca por PRs chicos en orden de valor**, no backend-primero | Backend completo y después la app | Lo barato y visible (emojis, contador de encuentros, advertencia de fauna) se puede mergear en días; el mapa y las insignias necesitan migraciones y tardan. Cada PR entra solo. |
| C | El emoji **⚠️ de comestibilidad en Fungi se conserva** | Quitar todos los emojis sin excepción | Los demás emojis son decorativos; ese marca riesgo sanitario real y la regla del proyecto pide que la comestibilidad sea imposible de pasar por alto. Si querés que se vaya también, se cambia por un badge de color y texto. |

---

## 2. Fases

Cada ítem es un PR contra `master` del repo que corresponda. El merge lo hace
el humano.

### Fase 9.0 — Lo barato y visible

**PR 1 — `fix(ficha): quitar los emojis decorativos de las secciones`**
*(mobile)*
Quita el campo `emoji` de `FichaSection`/`Grupo` en `src/utils/ficha.ts` y de su
render en `EspecieDetailScreen`. Las secciones pasan a distinguirse por
tipografía y separadores. Excepción: la sección de comestibilidad de Fungi
mantiene su marca de advertencia (decisión C).
Toca: `src/utils/ficha.ts`, `src/screens/EspecieDetailScreen.tsx`,
`src/utils/__tests__/ficha.test.ts`.

**PR 2 — `feat(camara): ofrecer crear un encuentro al terminar la captura`**
*(mobile)*
Hoy la pestaña "Capturar" deja la foto en caché sin destino. Tras el disparo se
muestra una revisión con la foto y tres salidas: **Crear encuentro** (lleva a
elegir especie y de ahí al formulario ya existente), **Repetir** y **Descartar**.
El selector de especie reusa la búsqueda de `BibliotecaScreen` y admite
"todavía no sé cuál es" (encuentro sin especie, que la comunidad identifica —
la tabla `avistamiento_identificaciones` de la migración `0007` ya existe para
eso).
Toca: `src/screens/CameraScreen.tsx`, pantalla nueva de revisión y de selección
de especie, `src/navigation/AppNavigator.tsx`,
`src/screens/MiEncuentroFormScreen.tsx` (aceptar una foto ya tomada).

**PR 3 — `feat(perfil): contar encuentros en vez de fichas abiertas`**
*(mobile)*
"Descubiertas" pasa a **"Encuentros"** y cuenta los avistamientos del usuario
(locales + sincronizados), no las fichas que abrió. "Progreso de campo" pasa a
medir **especies distintas con encuentro propio** sobre el total del catálogo,
que es lo que de verdad describe un recorrido de campo. Se conserva un contador
secundario "fichas consultadas" con ese nombre, que es lo que siempre midió.
Toca: `src/screens/PerfilScreen.tsx`, `src/db/mutationQueue.ts` (consulta de
conteo), test.

**PR 4 — `feat(app): advertencia sobre no molestar a la fauna`**
*(mobile)*
Tres puntos de aparición, de menor a mayor fricción:
1. **Ficha de especie de reino `animalia`**: aviso permanente bajo el
   encabezado — no acercarse, no alimentar, no usar reclamos ni grabaciones
   para atraer, no perseguir para fotografiar, mantener distancia y perros con
   correa.
2. **Formulario de encuentro de un `animalia`**: recordatorio corto antes de
   guardar.
3. **Primer arranque**: pantalla de bienvenida con el criterio general — usar la
   app en **parques, senderos y miradores habilitados**, donde el recorrido
   humano ya está definido, en vez de salir a batir hábitat.
El texto vive en un solo módulo (`src/content/avisos.ts`) para que curaduría lo
pueda revisar sin buscar por las pantallas.
Toca: `src/screens/EspecieDetailScreen.tsx`,
`src/screens/MiEncuentroFormScreen.tsx`, pantalla de bienvenida, `src/content/`.

### Fase 9.1 — Perfil de verdad

**PR 5 — `feat(auth): bio, profesión y visibilidad en el perfil`**
*(backend, `auth-service`)*
Migración nueva sobre `users`: `bio TEXT` (límite ~500 caracteres),
`profesion TEXT` y `perfil_publico BOOLEAN NOT NULL DEFAULT false`.
`PUT /api/v1/auth/me` acepta los dos primeros; `profesion` **solo se muestra**
cuando el rol es `moderator`/`admin`/curador, que es el caso de uso que
justifica el campo (dar respaldo a quien modera). Endpoint nuevo
`GET /api/v1/auth/usuarios/:id/publico` con el subconjunto visible por
terceros: nombre, avatar, bio, rol, profesión si corresponde, insignias y
conteo de encuentros públicos. Nunca el email.

**PR 6 — `feat(perfil): editar perfil de verdad`**
*(mobile)*
El botón "Actualizar perfil" pasa a llamarse **"Editar perfil"** y abre un
formulario con nombre, bio y —si el rol lo permite— profesión. El refresco
contra el servidor queda como acción secundaria con su nombre real
("Recargar datos"). Se añade la vista pública de perfil de otro usuario, a la
que ya lleva el feed comunitario.

**PR 7 — `feat(encuentros): registrar encuentros anteriores a la app`**
*(mobile + backend)*
El formulario de encuentro deja de asumir "aquí y ahora":
- **Fecha**: selector, con tope en hoy y piso razonable, en vez de
  `new Date()` fijo.
- **Ubicación**: además del GPS, elegir el punto **tocando el mapa** (depende
  de la Fase 9.2) y un campo de precisión declarada — "exacto", "aproximado",
  "solo la zona" — porque un recuerdo de hace tres años no tiene 5 m de
  precisión y guardarlo como si los tuviera contamina el mapa.
- **Foto**: desde la galería, que ya funciona (PR #31 mergeado).
En el backend hay que aflojar la validación de `observado_en` si asume fechas
recientes, y añadir `precision_declarada` al modelo.

### Fase 9.2 — El mapa

**PR 8 — `feat(mapa): mapa satelital de encuentros`**
*(mobile)*
Dependencia nueva: `react-native-maps` (justificada en el PR). Key de Google
Maps por `MAPS_API_KEY` en `local.properties`/CI, **nunca** commiteada, con
restricción por huella de firma. Pantalla de mapa con:
- Capa base **satelital** por defecto, alternable a híbrida.
- **Mis encuentros** y **encuentros públicos de la comunidad** como capas que
  se prenden y apagan por separado.
- **Clustering** por zoom. Un clúster que supere un umbral en un radio chico se
  rotula como **punto caliente** de esa especie ("aquí se ha visto mucho X").
- Filtro por reino y por especie.

**PR 9 — `feat(avistamientos): endpoint agregado para el mapa`**
*(backend, `especies-api`)*
`GET /api/v1/avistamientos/mapa?bbox=&zoom=&reino=&especie_id=` que devuelve
**celdas agregadas**, no filas: la app no necesita 10.000 puntos para dibujar
un clúster, y mandarlos sería lento y filtraría ubicaciones exactas de golpe.
Reglas de privacidad, que son la parte delicada:
- Solo entran avistamientos `visibilidad='publico'` **y** `estado='aprobado'`.
- Los propios del usuario se piden aparte, sin agregar, porque son suyos.
- **Ofuscación por sensibilidad**: para especies en categoría de conservación
  de riesgo (`VU`, `EN`, `CR`, `EW`) la coordenada pública se redondea a una
  celda de ~1 km. Publicar el punto exacto de una especie amenazada es una
  invitación al tráfico y a la presión de observadores; es el mismo criterio
  que usan GBIF e iNaturalist.
Índice espacial sobre `(geo_lat, geo_lng)` en la migración.

**PR 10 — `feat(mapa): parques y áreas protegidas de Chiloé`**
*(backend + mobile)*
Tabla nueva `areas_protegidas` (nombre, tipo — parque nacional, reserva,
parque privado, sitio Ramsar, humedal urbano —, administrador, geometría,
accesos, sitio web) con seed de las áreas de Chiloé: PN Chiloé, PN Tantauco,
Parque Tepuhueico, Reserva Forestal Piedra Blanca, humedales de Putemún y
Caulín, entre otras — la lista final la valida curaduría contra fuentes CONAF
y del administrador de cada parque, con su licencia anotada.
En el mapa se dibujan como capa propia, y la ficha de cada área lista qué
especies se han registrado dentro. Este es el gancho para el turista: no "dónde
está el bicho" sino "a qué parque voy y qué puedo ver ahí".

### Fase 9.3 — Comunidad

**PR 11 — `feat(insignias): sistema de insignias`**
*(backend + mobile)*
Tablas `insignias` (catálogo: código, nombre, descripción, criterio) y
`usuario_insignias` (otorgadas, con fecha y motivo). Dos familias:
- **Automáticas por actividad**: cantidad de encuentros, especies distintas,
  reinos cubiertos, encuentros identificados por otros. Se recalculan en un
  job, no al vuelo.
- **Manuales por rol**: moderador, curador de una categoría, administrador.
  Las otorga un `admin` desde el panel de curaduría.
Se muestran en el perfil propio, en el perfil público y junto al nombre en el
feed. Deliberadamente **no** hay ranking público de "quién tiene más
encuentros": premiar el volumen empuja exactamente la conducta que la Fase 9.0
intenta desalentar. Las insignias reconocen constancia y variedad, sin tabla de
posiciones.

**PR 12 — `feat(curaduria): pantalla de usuarios`**
*(panel de curaduría)*
El panel ya existe y está en producción; le falta la pantalla de usuarios, que
concentra tres cosas que hoy solo se pueden hacer por API o directo en la base:
- **Listar usuarios** con su rol y sus categorías asignadas. Hoy un `admin` no
  tiene forma de ver quién es quién.
- **Asignar y quitar categorías** a un curador, llamando a los
  `POST`/`DELETE /api/v1/categorias/:id/moderadores/:usuarioId` que ya existen.
  Hoy la única vía por interfaz es aprobar una postulación, y las postulaciones
  no se pueden crear (ver PR 13). Es el agujero que dejó a la cuenta admin sin
  poder repartir permisos.
- **Otorgar insignias manuales** y verificar la profesión declarada por un
  moderador. Sin verificación, "profesión" es texto libre que cualquiera se
  atribuye — y el punto del campo es dar respaldo.
Resuelto: el listado lo sirve `GET /api/v1/curaduria/usuarios` en
`especies-api`, que llama al `auth-service` para hidratar nombre y email sobre
las asignaciones de `moderador_categorias` (ADR #27). El cruce no queda en el
navegador. Si el `auth-service` no responde, la respuesta trae los ids que curan
algo con `auth_disponible: false` en vez de un 500, para que el panel avise en
vez de quedarse en blanco.
Hecho: el listado del lado `auth-service` (`GET /api/v1/auth/admin/users`,
paginado, con búsqueda y filtro por rol) y el endpoint combinado. Pendiente: la
pantalla del panel que los consume.

**PR 13 — `feat(curaduria): postular a curar una categoría`**
*(mobile + panel)*
Cierra el circuito que hoy está partido: el endpoint `POST /api/v1/postulaciones`
y la bandeja de revisión existen desde la migración `0005`, pero **nadie puede
postular** porque no hay interfaz en ningún cliente.
- En la app, desde el perfil: elegir categoría y escribir el `texto` de la
  postulación (por qué querría curar esa categoría: formación, experiencia de
  campo, trabajo). Ver el estado de la propia postulación —pendiente, aprobada,
  rechazada con motivo— y poder volver a postular si la rechazaron.
- Una sola postulación pendiente por categoría y usuario; conviene comprobar si
  la migración `0005` ya lo restringe con un índice único antes de resolverlo en
  código.
- Se enlaza con la profesión declarada del PR 11: postular es justamente el
  momento en que tiene sentido pedirla.

### Fase 9.4 — Que la app se vea viva

Los tres salen de mirar la app funcionando en el teléfono el 2026-08-18, no de
la planificación original.

**PR 14 — `feat(home): portada con lo último, no con una especie al azar`**
*(backend + mobile)*
Hoy la portada muestra **una especie destacada elegida arbitrariamente** y un
círculo por reino. No cambia entre visitas, no refleja que alguien esté usando
la app y no da ninguna razón para volver a abrirla. Una portada de una
biblioteca viva tiene que mostrar movimiento:

- **Últimas especies publicadas** — fichas nuevas, por `creado_en`.
- **Últimas ediciones** — fichas que se actualizaron, por `actualizado_en`, con
  qué cambió si es barato saberlo. Es lo que hace visible el trabajo de
  curaduría, que hoy no se ve por ningún lado.
- **Últimos encuentros de la comunidad** — los avistamientos aprobados y
  públicos más recientes, con foto. Reusa la visibilidad ya resuelta en el PR 8
  y la ofuscación de especies amenazadas del PR 9: la portada **no** puede ser
  la puerta de atrás que publique ubicaciones que el mapa protege.

Conviene un solo endpoint agregado (`GET /api/v1/portada`) y no tres llamadas:
la portada es lo primero que se abre y a veces con red de isla. Cachearlo
offline como el resto (`src/db/`), para que abrir la app sin señal siga
mostrando lo último que se vio en vez de tres bloques vacíos.

**PR 15 — `feat(explorar): filtros por grupo dentro de cada reino`**
*(backend + mobile)*
Filtrar solo por reino es demasiado grueso: `animalia` mete en la misma bolsa al
chungungo, al pudú, al chucao y a una rana. Hay que poder filtrar por grupo —
aves, mamíferos, peces, reptiles, anfibios e invertebrados en animalia, y lo que
corresponda en los otros cuatro reinos.

**El eje ya existe y conviene mirarlo antes de crear otro.** La migración `0004`
creó `categorias_moderacion` — literalmente "subgrupo curable dentro de un reino
(p. ej. 'Aves' dentro de animalia)", sin jerarquía — y la columna
`especies.categoria_id`, ya backfilleada con cinco categorías "general". Es la
misma partición que pide este filtro, escrita para otro propósito.

**Decisión pendiente, no la tomo solo.** Tres caminos:

1. **Reusar `categorias_moderacion` como eje de navegación.** No hay migración
   estructural: se crean las subcategorías reales ("Aves", "Mamíferos", "Peces",
   "Reptiles", "Anfibios") por la API, que es como el `0004` dice que se hacen, y
   se reclasifican las 103 fichas. El filtro es un `WHERE categoria_id = …`
   sobre una columna que ya está indexada. **Es la que recomiendo**, con una
   advertencia: acopla el eje de *permisos de curaduría* al de *navegación*, así
   que a futuro un curador de "Aves" y la pestaña "Aves" quedan atados al mismo
   registro. Hoy eso es una ventaja —quien cura aves es quien sabe de aves— pero
   conviene decidirlo a ojos abiertos.
2. **Tabla `clases` entre reino y familia**, con `familias.clase_id`. Es la
   taxonomía real (reino → filo → clase → orden → familia) y separa navegación
   de permisos. Cuesta una migración más y deja dos particiones parecidas
   conviviendo, que es la receta para que se contradigan.
3. **Columna `grupo` en `especies`** o propiedad en `atributos_especificos`.
   Las descarto: la primera es un dato denormalizado que puede contradecir a la
   familia; la segunda mete en el JSONB algo que aplica a todos los reinos y que
   se va a filtrar en la pantalla principal de navegación.

Cualquiera exige decidir los grupos de protista y monera, donde la clase no es
un concepto tan usado ni tan útil para alguien que pasea por Chiloé. Ojo con
inventar taxonomía: los grupos tienen que salir de una fuente, no de la
intuición.

**PR 16 — `fix(app): íconos de navegación en vez de emojis`**
*(mobile)*
La barra usa **emojis literales** (`AppNavigator.tsx:31-38`: 🏠 🔎 📷 🗺️ 👥 🔖
🙋). Se ven como emoticones de teléfono, cambian de dibujo en cada versión de
Android y no respetan el color activo/inactivo del tema, porque un emoji trae
su propio color. Hay que reemplazarlos por íconos de trazo, minimalistas, que
tomen el `color` que ya les pasa `tabBarIcon`.

Es la misma corrección que el PR 1 hizo en las fichas, pero en la navegación.
Requiere **una dependencia nueva** (`react-native-svg` o un set de íconos), que
hay que justificar en el PR; la alternativa sin dependencias es incluir los
trazos como componentes propios. Se decide junto con si el Mapa sigue siendo la
séptima pestaña: con siete, los títulos ya se truncan en pantalla ("Comun…",
"Guarda…").

---

## 3. Riesgos y cosas que hay que mirar

- **La ubicación es el dato más sensible de la app.** El mapa comunitario, los
  encuentros retroactivos y las insignias por volumen empujan todos hacia
  publicar más ubicaciones y más precisas. La ofuscación del PR 9 y la ausencia
  de ranking del PR 11 son contrapesos deliberados, no adornos.
- **Key de Google Maps**: restringida por huella de firma y nombre de paquete
  desde el primer commit. Una key sin restricción en un APK es una key
  regalada.
- **Licencias de los datos de parques**: las geometrías oficiales tienen
  condiciones de uso. Se anota la fuente y la licencia de cada área en el seed,
  igual que se hizo con las fotos de especies.
- **El JDK del host pasó a 26** y el plugin Gradle de React Native no lo parsea
  (`IllegalArgumentException: 26.0.2`). Hay que compilar con
  `JAVA_HOME=/usr/lib/jvm/java-17-openjdk`. Vale la pena resolverlo de raíz
  antes de que se lo coma CI.

---

## 4. Ideas que no pediste y creo que valen

1. **Ficha de parque como destino, no como capa.** Si el turista abre "PN
   Chiloé" y ve qué esperar en enero, con senderos y época, la app deja de
   competir con las guías impresas y las reemplaza.
2. **Modo sin conexión para el mapa de un parque.** En Tantauco y buena parte
   de la costa oeste no hay señal. Bajar los tiles de un área antes de salir es
   la diferencia entre una app útil en terreno y una que solo sirve en el hotel.
3. **Ventana estacional en la ficha.** "Se ve de noviembre a marzo" resuelve la
   pregunta real del visitante estival mejor que cualquier mapa.
4. **Encuentros sin especie identificada.** Bajar la barrera de registrar algo
   que no sabés qué es, y dejar que la comunidad lo identifique, es lo que hace
   crecer el dataset. La tabla de identificaciones ya existe desde la migración
   `0007`.
5. **Un botón de "reportar conducta" en el feed.** Si alguien sube una foto que
   evidencia acoso a un animal, tiene que haber por dónde avisar. El aviso de
   la Fase 9.0 sin un canal de reporte es una declaración de intenciones.
