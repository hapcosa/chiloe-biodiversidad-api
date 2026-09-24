-- =============================================================================
-- 0004_especies_chiloe_fauna.sql — ampliación de fauna del archipiélago
-- =============================================================================
-- Tercera tanda del catálogo, solo animalia: 45 fichas que faltaban entre las
-- más visibles de Chiloé. Incluye:
--   * el rayadito de Chiloé (Aphrastura spinicauda fulva), la subespecie
--     descrita desde Ancud, como ficha propia además de la especie;
--   * los dos búhos que faltaban (tucúquere y nuco), que completan con el
--     concón, el chuncho y la lechuza de 0002 las rapaces nocturnas del sur;
--   * las primeras fichas del subgrupo "Peces", que la migración 0013 creó
--     vacío a la espera de ellas.
--
-- La subespecie va como fila aparte con nombre trinomial: el esquema no tiene
-- rango infraespecífico y `nombre_cientifico` es la clave natural, así que el
-- trinomio no choca con la ficha de la especie y se encuentra con la misma
-- búsqueda por "Aphrastura".
--
-- Idempotente igual que 0001 y 0002 (ON CONFLICT DO NOTHING sobre las claves
-- naturales). Los atributos_especificos cumplen config/schemas/animalia.json.
--
-- Las familias nuevas se registran en `familia_subgrupo` y al final se
-- clasifican las fichas sin subgrupo. Hace falta aquí y no basta con el seed
-- 0003: ese corre antes que éste por orden lexicográfico, así que en la primera
-- aplicación estas fichas quedarían en la categoría "general" del reino.
--
-- ⚠️ Igual que 0002, las fichas se redactaron a partir de conocimiento general
-- de la fauna chilota y no transcribiendo una fuente por especie. Tamaños,
-- pesos y épocas reproductivas son plausibles pero no están verificados uno a
-- uno. La presencia del tucúquere en la Isla Grande está poco documentada y
-- la ficha lo dice así.
-- =============================================================================

BEGIN;

INSERT INTO familias (nombre, reino, descripcion) VALUES
    ('Troglodytidae',  'animalia', 'Chercanes y cucaracheros, paseriformes pequeños de cola erguida.'),
    ('Passerellidae',  'animalia', 'Chingolos y gorriones del Nuevo Mundo.'),
    ('Fringillidae',   'animalia', 'Jilgueros y fringílidos granívoros.'),
    ('Hirundinidae',   'animalia', 'Golondrinas, insectívoras aéreas.'),
    ('Charadriidae',   'animalia', 'Chorlos y queltehues.'),
    ('Scolopacidae',   'animalia', 'Zarapitos, playeros y becasinas.'),
    ('Ardeidae',       'animalia', 'Garzas y huairavos.'),
    ('Accipitridae',   'animalia', 'Águilas, aguiluchos y gavilanes.'),
    ('Columbidae',     'animalia', 'Palomas y tórtolas.'),
    ('Alcedinidae',    'animalia', 'Martines pescadores.'),
    ('Caenolestidae',  'animalia', 'Marsupiales ratoniles andinos, un linaje relicto.'),
    ('Leptodactylidae','animalia', 'Ranas de pulgar sudamericanas.'),
    ('Galaxiidae',     'animalia', 'Peces galáxidos de aguas frías del hemisferio sur.'),
    ('Eleginopidae',   'animalia', 'Familia monotípica del róbalo, nototenioideo de estuario.'),
    ('Ophidiidae',     'animalia', 'Congrios y brótulas, peces anguiliformes de fondo.'),
    ('Clupeidae',      'animalia', 'Sardinas y arenques.'),
    ('Aeglidae',       'animalia', 'Pancoras, cangrejos de agua dulce exclusivos de Sudamérica.'),
    ('Cancridae',      'animalia', 'Jaibas de caparazón ovalado.'),
    ('Balanidae',      'animalia', 'Picorocos y cirrípedos sésiles.'),
    ('Pyuridae',       'animalia', 'Ascidias solitarias de túnica gruesa.'),
    ('Solecurtidae',   'animalia', 'Navajuelas, bivalvos excavadores alargados.')
ON CONFLICT (reino, nombre) DO NOTHING;

INSERT INTO generos (nombre, familia_id, descripcion)
SELECT v.nombre, f.id, v.descripcion
FROM (VALUES
    ('Bubo',             'animalia', 'Strigidae',        'Búhos grandes con penachos auriculares.'),
    ('Asio',             'animalia', 'Strigidae',        'Búhos de ala larga, de campo abierto.'),
    ('Scytalopus',       'animalia', 'Rhinocryptidae',   'Churrines, tapaculos diminutos y oscuros.'),
    ('Eugralla',         'animalia', 'Rhinocryptidae',   'Género monotípico del churrín de la Mocha.'),
    ('Pygarrhichas',     'animalia', 'Furnariidae',      'Género monotípico del comesebo grande.'),
    ('Cinclodes',        'animalia', 'Furnariidae',      'Churretes, furnáridos de borde de agua.'),
    ('Anairetes',        'animalia', 'Tyrannidae',       'Cachuditos, tiránidos de cresta eréctil.'),
    ('Colorhamphus',     'animalia', 'Tyrannidae',       'Género monotípico de la viudita.'),
    ('Troglodytes',      'animalia', 'Troglodytidae',    'Chercanes.'),
    ('Zonotrichia',      'animalia', 'Passerellidae',    'Chincoles.'),
    ('Spinus',           'animalia', 'Fringillidae',     'Jilgueros.'),
    ('Tachycineta',      'animalia', 'Hirundinidae',     'Golondrinas de dorso metálico.'),
    ('Leistes',          'animalia', 'Icteridae',        'Loicas de pecho rojo.'),
    ('Vanellus',         'animalia', 'Charadriidae',     'Queltehues y avefrías.'),
    ('Limosa',           'animalia', 'Scolopacidae',     'Zarapitos de pico recto.'),
    ('Numenius',         'animalia', 'Scolopacidae',     'Zarapitos de pico curvo.'),
    ('Chloephaga',       'animalia', 'Anatidae',         'Cauquenes y carancas.'),
    ('Tachyeres',        'animalia', 'Anatidae',         'Quetrus, patos vapor.'),
    ('Ardea',            'animalia', 'Ardeidae',         'Garzas grandes.'),
    ('Nycticorax',       'animalia', 'Ardeidae',         'Huairavos, garzas nocturnas.'),
    ('Geranoaetus',      'animalia', 'Accipitridae',     'Aguiluchos y águila mora.'),
    ('Accipiter',        'animalia', 'Accipitridae',     'Gavilanes de bosque.'),
    ('Caracara',         'animalia', 'Falconidae',       'Caranchos o traros.'),
    ('Patagioenas',      'animalia', 'Columbidae',       'Palomas del Nuevo Mundo.'),
    ('Megaceryle',       'animalia', 'Alcedinidae',      'Martines pescadores grandes.'),
    ('Rhyncholestes',    'animalia', 'Caenolestidae',    'Género monotípico de la comadrejita trompuda.'),
    ('Abrothrix',        'animalia', 'Cricetidae',       'Ratones de pelo largo y oliváceos.'),
    ('Geoxus',           'animalia', 'Cricetidae',       'Ratones topo del bosque austral.'),
    ('Histiotus',        'animalia', 'Vespertilionidae', 'Murciélagos orejones.'),
    ('Arctocephalus',    'animalia', 'Otariidae',        'Lobos finos u osos marinos.'),
    ('Megaptera',        'animalia', 'Balaenopteridae',  'Género monotípico de la ballena jorobada.'),
    ('Pleurodema',       'animalia', 'Leptodactylidae',  'Sapitos de cuatro ojos.'),
    ('Hylorina',         'animalia', 'Batrachylidae',    'Género monotípico de la rana esmeralda.'),
    ('Galaxias',         'animalia', 'Galaxiidae',       'Puyes y peladillas.'),
    ('Eleginops',        'animalia', 'Eleginopidae',     'Género monotípico del róbalo.'),
    ('Genypterus',       'animalia', 'Ophidiidae',       'Congrios.'),
    ('Sprattus',         'animalia', 'Clupeidae',        'Sardinas australes y espadines.'),
    ('Aegla',            'animalia', 'Aeglidae',         'Pancoras.'),
    ('Metacarcinus',     'animalia', 'Cancridae',        'Jaibas marmola.'),
    ('Austromegabalanus','animalia', 'Balanidae',        'Picorocos gigantes.'),
    ('Pyura',            'animalia', 'Pyuridae',         'Piures.'),
    ('Tagelus',          'animalia', 'Solecurtidae',     'Navajuelas.')
) AS v(nombre, reino, familia, descripcion)
JOIN familias f ON f.nombre = v.familia AND f.reino = v.reino::reino_enum
ON CONFLICT (familia_id, nombre) DO NOTHING;

INSERT INTO especies (
    nombre_cientifico, nombre_comun, autor_cientifico, descripcion, habitat,
    distribucion_chiloe, endemica, estado_conservacion, reino, genero_id,
    atributos_especificos, fuentes
)
SELECT
    v.nombre_cientifico, v.nombre_comun, v.autor_cientifico, v.descripcion,
    v.habitat, v.distribucion_chiloe, v.endemica, v.estado_conservacion,
    v.reino::reino_enum, g.id, v.atributos::jsonb, v.fuentes::jsonb
FROM (VALUES
    -- -------------------------------------------------------------------------
    -- Aves
    -- -------------------------------------------------------------------------
    (
        'Aphrastura spinicauda fulva', 'Rayadito de Chiloé', 'Angelini, 1905',
        'Subespecie del rayadito descrita desde Ancud, originalmente como especie propia (Aphrastura fulva). Se distingue de la forma continental por las partes inferiores de un ocre profundo en vez de blanquecinas —más pálidas solo en la garganta— y por un dorso más rojizo. Conserva el resto del patrón de la especie: ceja canela ancha, alas negras con barras ocráceas y la cola de raquis desnudos terminados en espinas, que usa de apoyo al trepar. Recorre el follaje en bandadas mixtas, colgando cabeza abajo de ramas y líquenes en busca de artrópodos. Estudios genéticos la muestran menos diferenciada que la subespecie de isla Mocha y comparten alelos con la población continental; pieles de color parecido en otras islas costeras sugieren que el vientre ocre podría aparecer de forma independiente en poblaciones insulares de clima muy húmedo.',
        'Bosque siempreverde, renovales y bordes soleados; nidifica en grietas y huecos de troncos, a menudo quemados.',
        'Es la forma del rayadito que vive en la Isla Grande; la subespecie nominal continental no se registra en la isla. Su límite sur frente a los archipiélagos de Guaitecas y Chonos no está bien resuelto.',
        true, 'No Evaluada', 'animalia', 'Aphrastura',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos, larvas y arañas que busca en corteza, musgos y líquenes epífitos.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":14,"peso_promedio_g":11,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica entre octubre y diciembre en cavidades de troncos, con tres a cuatro huevos."}',
        '["Angelini, G. 1905 — Bollettino della Società Zoologica Italiana (ser. 2) 6: 227","Birds of the World — Thorn-tailed Rayadito","IOC World Bird List — Furnariidae"]'
    ),
    (
        'Bubo magellanicus', 'Tucúquere', '(Lesson, 1828)',
        'El búho más grande de Chile: supera el medio metro y ronda el kilo y medio. Tiene dos penachos de plumas sobre la cabeza que parecen orejas, ojos amarillos grandes y un plumaje pardo grisáceo finamente barrado. Hasta hace poco se lo trataba como subespecie del búho cornudo norteamericano; la voz, la forma y análisis genéticos sostienen hoy su separación. Caza desde perchas al anochecer presas que otros búhos chilenos no alcanzan, como liebres y conejos. En Chiloé se lo conoce también como raiquén, y la tradición oral lo asocia a presagios y a brujos transformados.',
        'Ambientes abiertos y semiabiertos con roqueríos, quebradas, bordes de bosque y matorral.',
        'Presencia en la Isla Grande poco documentada: no hay colonias ni registros publicados que la confirmen como residente. Se incluye por su lugar en el folclore chilote y a la espera de registros de avistamientos.',
        false, 'Preocupación Menor', 'animalia', 'Bubo',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Mamíferos pequeños y medianos —roedores, conejos, liebres—, aves y ocasionalmente reptiles e insectos grandes.","comportamiento":{"actividad":"nocturno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":48,"peso_promedio_g":1300,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en repisas rocosas o nidos abandonados a fines del invierno, con dos a tres huevos."}',
        '["IUCN Red List — Bubo magellanicus","Museo de Historia Natural de Concepción — Tucúquere","Kusch, A. & Donoso, J. 2017 — Boletín Chileno de Ornitología"]'
    ),
    (
        'Asio flammeus', 'Nuco', '(Pontoppidan, 1763)',
        'Búho de campo abierto, mediano, de ojos amarillos rodeados de un antifaz oscuro y penachos tan cortos que casi no se ven. El plumaje es ocre con listas pardas y las alas son largas, con una mancha clara en la base de las primarias visible en vuelo. A diferencia de los búhos de bosque, caza volando bajo y con aleteos pausados sobre pastizales y turberas, a menudo a plena luz del crepúsculo. Anida en el suelo, entre pastos altos, lo que lo hace vulnerable al fuego, al pisoteo del ganado y a los perros.',
        'Praderas, vegas húmedas, turberas y pastizales costeros.',
        'Presente en praderas y humedales de la Isla Grande, más visible al atardecer sobre pastizales y bordes de turbera.',
        false, 'Preocupación Menor', 'animalia', 'Asio',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Principalmente roedores de pradera; complementa con aves pequeñas e insectos grandes.","comportamiento":{"actividad":"crepuscular","social":"solitario","migratorio":false},"tamano_promedio_cm":38,"peso_promedio_g":350,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en el suelo en primavera, con cuatro a siete huevos según la abundancia de roedores."}',
        '["IUCN Red List — Asio flammeus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Scytalopus magellanicus', 'Churrín del sur', '(Gmelin, 1789)',
        'Tapaculo diminuto, de apenas once centímetros, gris pizarra uniforme con los flancos pardos barrados y, en algunos individuos, una mancha blanca en la frente. Vive pegado al suelo en la maraña del sotobosque, donde se mueve como un ratón y rara vez vuela más de unos metros. Es mucho más fácil de oír que de ver: repite sin pausa un chirrido rítmico que llena los quilantos. Se encuentra desde bosque maduro hasta matorral y quebradas, siempre que haya cubierta densa.',
        'Sotobosque denso, quilantos, matorrales de quebrada y bordes de bosque húmedo.',
        'Común en la Isla Grande donde hay sotobosque cerrado, incluso en fragmentos pequeños.',
        false, 'Preocupación Menor', 'animalia', 'Scytalopus',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos y arañas pequeñas del mantillo y de la base de la vegetación.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":false},"tamano_promedio_cm":11,"peso_promedio_g":12,"reproduccion":"oviparo","epoca_reproductiva":"Nido esférico de musgo en taludes o raíces, con dos huevos blancos en primavera."}',
        '["IUCN Red List — Scytalopus magellanicus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Eugralla paradoxa', 'Churrín de la Mocha', '(Kittlitz, 1830)',
        'Tapaculo gris oscuro, más grande que el churrín del sur, con la rabadilla y los flancos castaño rojizo y patas amarillentas robustas. Su pico alto y comprimido es distinto al de sus parientes y le da el nombre de "paradoxa". Se desplaza saltando por el suelo y entre las cañas de quila, y delata su presencia con un llamado seco y repetido. Es una especie de distribución reducida, casi exclusiva del bosque húmedo del centro-sur de Chile.',
        'Quilantos y sotobosque denso de bosque templado húmedo.',
        'Presente en bosques con quila de la Isla Grande, más escuchado que visto.',
        false, 'Preocupación Menor', 'animalia', 'Eugralla',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos, larvas y otros invertebrados del suelo y de la base de las cañas.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":14,"peso_promedio_g":25,"reproduccion":"oviparo","epoca_reproductiva":"Nido esférico entre quilas en primavera, con dos huevos."}',
        '["IUCN Red List — Eugralla paradoxa","Aves de Chile — Guía de campo"]'
    ),
    (
        'Pygarrhichas albogularis', 'Comesebo grande', '(King, 1831)',
        'Furnárido trepador de dorso pardo, rabadilla y cola rufas, y garganta y pecho blancos que resaltan contra la corteza oscura. Usa la cola rígida como apoyo y un pico en forma de cincel para desprender corteza y excavar madera blanda, un nicho parecido al de un carpintero pequeño. Excava su propia cavidad de nidificación en troncos muertos, por lo que depende de bosques con madera en descomposición. Suele integrarse a bandadas mixtas con rayaditos.',
        'Bosque maduro de Nothofagus y siempreverde con troncos muertos en pie.',
        'Presente en los bosques mejor conservados de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Pygarrhichas',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Larvas e insectos bajo la corteza y en madera en descomposición.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":15,"peso_promedio_g":25,"reproduccion":"oviparo","epoca_reproductiva":"Excava su nido en troncos muertos en primavera, con dos a tres huevos."}',
        '["IUCN Red List — Pygarrhichas albogularis","Aves de Chile — Guía de campo"]'
    ),
    (
        'Cinclodes patagonicus', 'Churrete', '(Gmelin, 1789)',
        'Furnárido pardo oscuro con una ceja blanca marcada y la garganta moteada, siempre cerca del agua. Camina y corre por playas de piedras, esteros y roqueríos moviendo la cola, y se zambulle a medias para capturar invertebrados en la orilla. Es territorial y ruidoso, con un trino rápido que emite a menudo en vuelo. En Chiloé es una de las aves más confiadas de las caletas.',
        'Orillas de ríos, esteros, playas de piedra y roqueríos costeros.',
        'Común en todo el litoral y en los cursos de agua de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Cinclodes',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Larvas acuáticas, pequeños crustáceos y moluscos de la orilla e insectos.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":20,"peso_promedio_g":45,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en cavidades de barrancos o bajo piedras en primavera, con dos a tres huevos."}',
        '["IUCN Red List — Cinclodes patagonicus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Anairetes parulus', 'Cachudito', '(Kittlitz, 1830)',
        'Tiránido diminuto de pecho blanquecino estriado, ojos claros y un copete de plumas negras finas y curvadas hacia adelante que le da su nombre. Se mueve inquieto entre arbustos y matorrales, capturando insectos al vuelo o en las hojas, y emite un trino agudo y chisporroteante. Tolera bien ambientes intervenidos y es frecuente en cercos vivos y bordes de camino.',
        'Matorrales, bordes de bosque, cercos vivos y jardines.',
        'Frecuente en la Isla Grande en matorral y paisaje rural.',
        false, 'Preocupación Menor', 'animalia', 'Anairetes',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos pequeños capturados en el follaje o en vuelos cortos.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":11,"peso_promedio_g":6,"reproduccion":"oviparo","epoca_reproductiva":"Nido en taza de fibras y plumas en arbustos, con dos a tres huevos en primavera."}',
        '["IUCN Red List — Anairetes parulus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Colorhamphus parvirostris', 'Viudita', '(Gould & G.R. Gray, 1839)',
        'Tiránido pequeño y discreto, gris oliváceo, con una mancha oscura detrás del ojo y dos barras alares claras. Su canto es un silbido agudo, fino y descendente, fácil de pasar por alto en el ruido del bosque. Nidifica en el bosque austral en verano y parte de la población se desplaza al norte en invierno. Es un ave de interior de bosque, poco confiada y difícil de observar.',
        'Interior y borde de bosque templado, renovales densos.',
        'Presente en bosques de la Isla Grande, sobre todo en primavera y verano.',
        false, 'Preocupación Menor', 'animalia', 'Colorhamphus',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos capturados en el follaje medio y bajo del bosque.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":true},"tamano_promedio_cm":12,"peso_promedio_g":8,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en primavera y verano en el bosque austral."}',
        '["IUCN Red List — Colorhamphus parvirostris","Aves de Chile — Guía de campo"]'
    ),
    (
        'Troglodytes aedon', 'Chercán', 'Vieillot, 1809',
        'Pájaro pequeño, pardo, de cola corta que lleva levantada, y un canto sorprendentemente potente y variado para su tamaño. Recorre matorrales, leñeras, galpones y rincones de las casas en busca de insectos, y anida en cualquier hueco disponible, desde troncos hasta tejas. Es de los pájaros más cercanos a la vida rural chilota. Algunas listas separan a las poblaciones sudamericanas como Troglodytes musculus.',
        'Matorrales, bordes de bosque, jardines, galpones y construcciones rurales.',
        'Abundante en toda la isla, en el campo y en los pueblos.',
        false, 'Preocupación Menor', 'animalia', 'Troglodytes',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos, larvas y arañas que busca entre ramas, grietas y hojarasca.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":12,"peso_promedio_g":11,"reproduccion":"oviparo","epoca_reproductiva":"Anida en cavidades naturales o artificiales, con cuatro a seis huevos y hasta dos nidadas por temporada."}',
        '["IUCN Red List — Troglodytes aedon","Aves de Chile — Guía de campo"]'
    ),
    (
        'Zonotrichia capensis', 'Chincol', '(Statius Muller, 1776)',
        'Gorrión nativo de cabeza gris con líneas negras, un leve copete y un collar castaño en la nuca. Su canto silbado, de pocas notas y con dialectos locales, es uno de los sonidos más familiares del campo chileno. Se alimenta en el suelo de semillas e insectos y se adapta bien a jardines y praderas. Es un hospedero frecuente del mirlo parásito en otras regiones del país.',
        'Praderas, jardines, bordes de camino y matorrales.',
        'Abundante en toda la Isla Grande, en áreas rurales y urbanas.',
        false, 'Preocupación Menor', 'animalia', 'Zonotrichia',
        '{"clase":"Aves","alimentacion":"granivoro","dieta_detalle":"Semillas de pastos y hierbas; en temporada reproductiva, insectos para los pollos.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":14,"peso_promedio_g":21,"reproduccion":"oviparo","epoca_reproductiva":"Nido en taza en el suelo o arbustos bajos, con dos a tres huevos, de primavera a verano."}',
        '["IUCN Red List — Zonotrichia capensis","Aves de Chile — Guía de campo"]'
    ),
    (
        'Spinus barbatus', 'Jilguero', '(Molina, 1782)',
        'Fringílido pequeño de plumaje amarillo verdoso; el macho tiene la corona y la barbilla negras. Se mueve en bandadas que recorren bordes de bosque, cardales y matorrales, colgándose de las cabezuelas para extraer semillas. Su canto es un gorjeo largo y variado que lo hizo históricamente un ave de jaula. Es residente del bosque austral y frecuenta jardines con árboles.',
        'Bordes de bosque, matorrales, praderas con malezas y jardines.',
        'Común en toda la isla, en bandadas que aumentan en otoño e invierno.',
        false, 'Preocupación Menor', 'animalia', 'Spinus',
        '{"clase":"Aves","alimentacion":"granivoro","dieta_detalle":"Semillas de compuestas y árboles como alisos, además de brotes; insectos para los pollos.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":13,"peso_promedio_g":13,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en árboles y arbustos en primavera, con tres a cinco huevos."}',
        '["IUCN Red List — Spinus barbatus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Tachycineta leucopyga', 'Golondrina chilena', '(Meyen, 1834)',
        'Golondrina de dorso negro azulado metálico, vientre blanco y rabadilla blanca que se ve bien en vuelo. Llega al sur en primavera para reproducirse y se retira hacia el norte y Argentina en otoño. Caza insectos en vuelo sobre praderas, ríos y bahías, y anida en huecos de árboles, aleros y construcciones. En Chiloé su llegada marca el inicio de la temporada cálida.',
        'Espacios abiertos cerca del agua, praderas, pueblos y bordes de bosque.',
        'Visitante de primavera y verano en toda la isla, abundante en pueblos y caletas.',
        false, 'Preocupación Menor', 'animalia', 'Tachycineta',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos voladores pequeños capturados en vuelo.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":true},"tamano_promedio_cm":13,"peso_promedio_g":17,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en cavidades y aleros entre octubre y enero, con cuatro a seis huevos."}',
        '["IUCN Red List — Tachycineta leucopyga","Aves de Chile — Guía de campo"]'
    ),
    (
        'Leistes loyca', 'Loica', '(Molina, 1782)',
        'Ictérido de dorso pardo estriado cuyo macho luce un pecho rojo escarlata intenso, visible a gran distancia sobre el verde de las praderas. Camina por el suelo hurgando con el pico largo y puntiagudo en busca de larvas. Canta desde postes y cercos una frase áspera y descendente. Hasta hace poco se ubicaba en el género Sturnella.',
        'Praderas, pastizales, campos de cultivo y vegas.',
        'Común en las praderas ganaderas de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Leistes',
        '{"clase":"Aves","alimentacion":"omnivoro","dieta_detalle":"Larvas e insectos del suelo, lombrices y semillas.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":25,"peso_promedio_g":110,"reproduccion":"oviparo","epoca_reproductiva":"Nido en el suelo oculto entre pastos, con tres a cuatro huevos en primavera."}',
        '["IUCN Red List — Leistes loyca","IOC World Bird List — Icteridae"]'
    ),
    (
        'Vanellus chilensis', 'Queltehue', '(Molina, 1782)',
        'Ave zancuda gris con pecho negro, un fino penacho en la nuca, ojos y patas rojizas y un espolón óseo en el ala. Es famosa por su alarma estridente, que repite ante cualquier intruso, y por lanzarse en picada contra perros y personas que se acercan al nido. Se adapta muy bien a praderas, canchas y bordes de camino. En Chiloé el dicho popular dice que su grito anuncia lluvia.',
        'Praderas, vegas húmedas, orillas de lagunas y áreas abiertas.',
        'Abundante en praderas y humedales de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Vanellus',
        '{"clase":"Aves","alimentacion":"insectivoro","dieta_detalle":"Insectos, larvas y lombrices que captura caminando por la pradera.","comportamiento":{"actividad":"catemeral","social":"en_pareja","migratorio":false},"tamano_promedio_cm":35,"peso_promedio_g":280,"reproduccion":"oviparo","epoca_reproductiva":"Nido en el suelo desde fines del invierno, con tres a cuatro huevos crípticos."}',
        '["IUCN Red List — Vanellus chilensis","Aves de Chile — Guía de campo"]'
    ),
    (
        'Limosa haemastica', 'Zarapito de pico recto', '(Linnaeus, 1758)',
        'Ave playera migratoria de pico largo, recto y levemente curvado hacia arriba. Se reproduce en Alaska y Canadá y pasa el verano austral en unos pocos sitios del sur de Sudamérica; Chiloé alberga una de sus concentraciones más importantes. Realiza vuelos sin escalas de miles de kilómetros, lo que la vuelve dependiente de que sus sitios de invernada tengan barros ricos en alimento. Los humedales costeros chilotes son, por eso, prioritarios para su conservación a escala hemisférica.',
        'Planicies intermareales de barro, estuarios y bahías protegidas.',
        'Visitante estival en humedales del este de la isla, como los de Caulín, Putemún y Curaco de Vélez.',
        false, 'Preocupación Menor', 'animalia', 'Limosa',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Poliquetos, pequeños bivalvos y crustáceos que obtiene sondeando el barro.","comportamiento":{"actividad":"catemeral","social":"gregario","migratorio":true},"tamano_promedio_cm":38,"peso_promedio_g":300,"reproduccion":"oviparo","epoca_reproductiva":"Se reproduce en el Ártico norteamericano en el verano boreal; no nidifica en Chile."}',
        '["IUCN Red List — Limosa haemastica","Red Hemisférica de Reservas para Aves Playeras (WHSRN) — Chiloé"]'
    ),
    (
        'Numenius hudsonicus', 'Zarapito', 'Latham, 1790',
        'Ave playera grande, parda y moteada, con un pico largo curvado hacia abajo y la corona listada. Llega desde Norteamérica a pasar el verano austral en las costas de Chiloé, donde es una de las playeras más conspicuas de las bahías interiores. Con el pico extrae cangrejos y gusanos de las madrigueras del barro. Varias poblaciones han disminuido y la especie depende de la protección de sus humedales de invernada.',
        'Planicies de barro intermareales, playas y praderas costeras.',
        'Visitante estival en bahías y humedales del mar interior de Chiloé.',
        false, 'Preocupación Menor', 'animalia', 'Numenius',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Cangrejos, poliquetos y otros invertebrados del barro y la orilla.","comportamiento":{"actividad":"catemeral","social":"gregario","migratorio":true},"tamano_promedio_cm":42,"peso_promedio_g":400,"reproduccion":"oviparo","epoca_reproductiva":"Se reproduce en la tundra norteamericana; no nidifica en Chile."}',
        '["IUCN Red List — Numenius hudsonicus","Red Hemisférica de Reservas para Aves Playeras (WHSRN) — Chiloé"]'
    ),
    (
        'Chloephaga hybrida', 'Caranca', '(Molina, 1782)',
        'Ganso costero de marcado dimorfismo: el macho es completamente blanco y la hembra oscura, barrada en blanco y negro, con patas amarillas. Vive estrictamente en costas rocosas, donde se alimenta de algas verdes que raspa de las rocas en marea baja. Las parejas se ven juntas todo el año, a menudo en roqueríos batidos por el oleaje.',
        'Costas rocosas con algas verdes en el intermareal.',
        'Presente en roqueríos de la costa de la Isla Grande y de las islas del sur del archipiélago.',
        false, 'Preocupación Menor', 'animalia', 'Chloephaga',
        '{"clase":"Aves","alimentacion":"herbivoro","dieta_detalle":"Algas verdes del intermareal rocoso, especialmente lechuga de mar.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":60,"peso_promedio_g":2500,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica cerca de la costa en primavera, con cuatro a seis huevos."}',
        '["IUCN Red List — Chloephaga hybrida","Aves de Chile — Guía de campo"]'
    ),
    (
        'Tachyeres pteneres', 'Quetru no volador', '(J.R. Forster, 1844)',
        'Pato marino enorme y pesado, gris, con pico anaranjado, incapaz de volar en la adultez. Cuando huye "corre" sobre el agua batiendo alas y patas y levantando espuma, como un vapor de ruedas, de ahí el nombre de pato vapor. Es territorial y agresivo: las parejas defienden su tramo de costa. Bucea para capturar mariscos que tritura con un pico muy potente.',
        'Costas rocosas, canales y fiordos protegidos.',
        'Presente en costas y canales del sur del archipiélago; más escaso en el norte de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Tachyeres',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Mitílidos, caracoles, erizos y crustáceos obtenidos buceando.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":80,"peso_promedio_g":5500,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica entre la vegetación costera en primavera, con cinco a ocho huevos."}',
        '["IUCN Red List — Tachyeres pteneres","Aves de Chile — Guía de campo"]'
    ),
    (
        'Ardea cocoi', 'Garza cuca', 'Linnaeus, 1766',
        'La garza más grande de Chile, gris azulada con corona y flancos negros y un cuello largo blanco. Pesca inmóvil en aguas someras y lanza el pico como un arpón. Es solitaria y desconfiada; alza el vuelo con aleteos lentos y el cuello recogido en S. En Chiloé frecuenta estuarios y orillas de lagos.',
        'Estuarios, lagunas, orillas de ríos y humedales costeros.',
        'Presente pero escasa en estuarios y humedales de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Ardea',
        '{"clase":"Aves","alimentacion":"piscivoro","dieta_detalle":"Peces, anfibios y crustáceos capturados al acecho en aguas someras.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":false},"tamano_promedio_cm":110,"peso_promedio_g":2500,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en colonias en árboles o juncales en primavera, con dos a cuatro huevos."}',
        '["IUCN Red List — Ardea cocoi","Aves de Chile — Guía de campo"]'
    ),
    (
        'Nycticorax nycticorax', 'Huairavo', '(Linnaeus, 1758)',
        'Garza robusta y de cuello corto, con corona y dorso negros, alas grises, ojos rojos y dos plumas blancas largas en la nuca. Descansa en grupos durante el día en árboles cerca del agua y sale a pescar al atardecer. Su graznido ronco en la noche es característico. Los juveniles, pardos y moteados, parecen otra especie.',
        'Estuarios, orillas de ríos, lagos y bordes costeros con árboles para dormir.',
        'Frecuente en estuarios y bordes costeros de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Nycticorax',
        '{"clase":"Aves","alimentacion":"piscivoro","dieta_detalle":"Peces, crustáceos, anfibios e insectos acuáticos.","comportamiento":{"actividad":"nocturno","social":"colonial","migratorio":false},"tamano_promedio_cm":60,"peso_promedio_g":800,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en colonias en árboles en primavera, con dos a cuatro huevos."}',
        '["IUCN Red List — Nycticorax nycticorax","Aves de Chile — Guía de campo"]'
    ),
    (
        'Geranoaetus polyosoma', 'Aguilucho', '(Quoy & Gaimard, 1824)',
        'Rapaz diurna mediana de cola blanca con banda negra terminal y un plumaje muy variable: hay individuos grises, con dorso rojizo o completamente oscuros. Planea en círculos sobre praderas y laderas, y a menudo se cierne quieto contra el viento antes de lanzarse sobre una presa. Es una de las rapaces más comunes del paisaje rural del sur.',
        'Praderas, bordes de bosque y áreas abiertas con perchas.',
        'Común en praderas y bordes de bosque de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Geranoaetus',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Roedores, conejos, aves, reptiles e insectos grandes.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":52,"peso_promedio_g":950,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en árboles o riscos en primavera, con dos a tres huevos."}',
        '["IUCN Red List — Geranoaetus polyosoma","Aves de Chile — Guía de campo"]'
    ),
    (
        'Accipiter chilensis', 'Peuquito', 'Philippi & Landbeck, 1864',
        'Gavilán de bosque, de alas cortas y redondeadas y cola larga, que le permiten maniobrar entre troncos a gran velocidad. El dorso es gris oscuro y el pecho finamente barrado; la hembra es bastante mayor que el macho. Caza aves pequeñas por sorpresa dentro del bosque. Es discreto y rara vez se deja ver en campo abierto. Algunas autoridades lo tratan como subespecie de Accipiter bicolor.',
        'Interior de bosque templado maduro y renovales densos.',
        'Presente en bosques de la Isla Grande, poco observado.',
        false, 'Preocupación Menor', 'animalia', 'Accipiter',
        '{"clase":"Aves","alimentacion":"carnivoro","dieta_detalle":"Aves pequeñas y medianas del bosque capturadas en persecución.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":false},"tamano_promedio_cm":40,"peso_promedio_g":350,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en árboles altos del bosque en primavera, con dos a tres huevos."}',
        '["IUCN Red List — Accipiter chilensis","Aves de Chile — Guía de campo"]'
    ),
    (
        'Caracara plancus', 'Traro', '(Miller, 1777)',
        'Falcónido grande de gorro negro, cara desnuda rojo anaranjada y pecho barrado. Camina con soltura por el suelo y es tan carroñero como cazador: aprovecha animales muertos en caminos, restos de faenas y presas vivas pequeñas. Se ve en pareja o en grupos junto a tiuques y jotes.',
        'Praderas, bordes de camino, playas y áreas agrícolas.',
        'Frecuente en praderas y costas de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Caracara',
        '{"clase":"Aves","alimentacion":"carronero","dieta_detalle":"Carroña, roedores, insectos, huevos y pollos de otras aves.","comportamiento":{"actividad":"diurno","social":"en_pareja","migratorio":false},"tamano_promedio_cm":55,"peso_promedio_g":1200,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en árboles en primavera, con dos a tres huevos."}',
        '["IUCN Red List — Caracara plancus","Aves de Chile — Guía de campo"]'
    ),
    (
        'Patagioenas araucana', 'Torcaza', '(Lesson, 1827)',
        'Paloma nativa grande, de plumaje vinoso con un collar blanco fino y una zona escamada verde metálica en la nuca. Se alimenta en el dosel de frutos y semillas y es una dispersora importante de especies como el boldo y el peumo. Fue casi exterminada por una enfermedad en los años cincuenta y luego por la caza, y se ha recuperado de forma notable. Vuela en bandadas rápidas con un aplauso de alas al despegar.',
        'Bosque nativo, bordes de bosque y áreas agrícolas vecinas.',
        'Común en bosques y campos de toda la isla, en bandadas.',
        false, 'Preocupación Menor', 'animalia', 'Patagioenas',
        '{"clase":"Aves","alimentacion":"frugivoro","dieta_detalle":"Frutos y semillas de árboles nativos, bellotas y granos cultivados.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":37,"peso_promedio_g":300,"reproduccion":"oviparo","epoca_reproductiva":"Nidifica en árboles en primavera y verano, con uno a dos huevos."}',
        '["IUCN Red List — Patagioenas araucana","Aves de Chile — Guía de campo"]'
    ),
    (
        'Megaceryle torquata', 'Martín pescador', '(Linnaeus, 1766)',
        'Ave de cabeza grande con cresta desgreñada, pico largo y puñal, dorso gris azulado y pecho castaño en el macho. Vigila el agua desde ramas y postes y se zambulle de cabeza para atrapar peces, que golpea contra la percha antes de tragarlos. Su llamado es un traqueteo fuerte. Excava largos túneles en barrancos de tierra para anidar.',
        'Ríos, esteros, lagos y bordes costeros protegidos con perchas.',
        'Presente en ríos y bordes costeros de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Megaceryle',
        '{"clase":"Aves","alimentacion":"piscivoro","dieta_detalle":"Peces pequeños y, en menor medida, crustáceos y anfibios.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":false},"tamano_promedio_cm":40,"peso_promedio_g":300,"reproduccion":"oviparo","epoca_reproductiva":"Anida en túneles excavados en barrancos, con tres a seis huevos en primavera."}',
        '["IUCN Red List — Megaceryle torquata","Aves de Chile — Guía de campo"]'
    ),
    -- -------------------------------------------------------------------------
    -- Mamíferos
    -- -------------------------------------------------------------------------
    (
        'Rhyncholestes raphanurus', 'Comadrejita trompuda', 'Osgood, 1924',
        'Pequeño marsupial del tamaño de un ratón, de hocico alargado, ojos diminutos y pelaje pardo oscuro. Pertenece a un linaje antiguo, los ratones runchos, que hoy sobrevive solo en los Andes y el bosque templado. Se describió a partir de un ejemplar colectado en 1923 en la desembocadura del río Inío, en el extremo sur de Chiloé. Vive en el mantillo húmedo y acumula grasa en la cola para los períodos de escasez. Se conoce de menos de veinticinco localidades y depende de bosques intactos.',
        'Mantillo y sotobosque de bosque templado lluvioso maduro.',
        'La isla es la localidad tipo de la subespecie nominal (R. r. raphanurus); registrada en el sur de la Isla Grande.',
        false, 'Casi Amenazado', 'animalia', 'Rhyncholestes',
        '{"clase":"Mammalia","alimentacion":"omnivoro","dieta_detalle":"Invertebrados del suelo, lombrices, larvas y en menor medida hongos y material vegetal.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":20,"peso_promedio_g":25,"reproduccion":"viviparo","epoca_reproductiva":"Poco conocida; la hembra carece de marsupio y las crías se prenden a las mamas."}',
        '["Osgood, W.H. 1924 — Field Museum of Natural History, Zoological Series 14(2)","IUCN Red List — Rhyncholestes raphanurus","Reglamento de Clasificación de Especies (MMA)"]'
    ),
    (
        'Abrothrix olivacea', 'Ratón oliváceo', '(Waterhouse, 1837)',
        'Roedor pequeño de pelaje pardo oliváceo, orejas cortas y cola más corta que el cuerpo. Es uno de los micromamíferos más abundantes del sur de Chile y activo tanto de día como de noche. Es presa básica de concones, chunchos, güiñas y zorros, por lo que sostiene buena parte de la cadena trófica del bosque.',
        'Sotobosque, matorrales, bordes de bosque y praderas con cobertura.',
        'Abundante en toda la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Abrothrix',
        '{"clase":"Mammalia","alimentacion":"omnivoro","dieta_detalle":"Insectos, larvas, semillas, hongos y material vegetal.","comportamiento":{"actividad":"catemeral","social":"solitario","migratorio":false},"tamano_promedio_cm":20,"peso_promedio_g":30,"reproduccion":"viviparo","epoca_reproductiva":"Se reproduce de primavera a otoño con camadas de tres a seis crías."}',
        '["IUCN Red List — Abrothrix olivacea","Iriarte, A. — Mamíferos de Chile"]'
    ),
    (
        'Geoxus valdivianus', 'Ratón topo valdiviano', '(Philippi, 1858)',
        'Roedor de hábitos cavadores, de pelaje oscuro y aterciopelado, ojos y orejas pequeños y garras delanteras largas. Vive bajo el mantillo en galerías superficiales y se alimenta sobre todo de lombrices e invertebrados. Es difícil de observar y se conoce principalmente por trampeo.',
        'Suelos húmedos y profundos de bosque templado y praderas con mantillo.',
        'Presente en la Isla Grande en bosques húmedos.',
        false, 'Preocupación Menor', 'animalia', 'Geoxus',
        '{"clase":"Mammalia","alimentacion":"insectivoro","dieta_detalle":"Lombrices, larvas y otros invertebrados del suelo.","comportamiento":{"actividad":"catemeral","social":"solitario","migratorio":false},"tamano_promedio_cm":15,"peso_promedio_g":30,"reproduccion":"viviparo","epoca_reproductiva":"Camadas pequeñas en primavera y verano."}',
        '["IUCN Red List — Geoxus valdivianus","Iriarte, A. — Mamíferos de Chile"]'
    ),
    (
        'Histiotus magellanicus', 'Murciélago orejón del sur', '(Philippi, 1866)',
        'Murciélago de orejas enormes, casi tan largas como el cuerpo, que le permiten detectar insectos por ecolocalización en el interior del bosque. Se refugia en huecos de árboles y construcciones y sale a cazar al anochecer. Es uno de los murciélagos más australes del mundo y su biología está poco estudiada.',
        'Bosque templado, bordes de bosque y construcciones rurales.',
        'Presente en la Isla Grande; registros escasos.',
        false, 'Preocupación Menor', 'animalia', 'Histiotus',
        '{"clase":"Mammalia","alimentacion":"insectivoro","dieta_detalle":"Polillas y otros insectos nocturnos capturados en vuelo.","comportamiento":{"actividad":"nocturno","social":"gregario","migratorio":false},"tamano_promedio_cm":11,"peso_promedio_g":12,"reproduccion":"viviparo","epoca_reproductiva":"Una cría al año, nacida a comienzos del verano."}',
        '["IUCN Red List — Histiotus magellanicus","Iriarte, A. — Mamíferos de Chile"]'
    ),
    (
        'Arctocephalus australis', 'Lobo fino austral', '(Zimmermann, 1783)',
        'Otárido más pequeño y esbelto que el lobo marino común, de hocico puntiagudo, orejas visibles y un pelaje denso de dos capas que lo protege del frío. Esa piel lo hizo objeto de una caza intensa en los siglos XIX y XX. Forma colonias en roqueríos expuestos y se alimenta lejos de la costa. Es más ágil en tierra que otros pinnípedos.',
        'Roqueríos expuestos y cuevas marinas del litoral oceánico.',
        'Colonias en roqueríos de la costa occidental del archipiélago.',
        false, 'Preocupación Menor', 'animalia', 'Arctocephalus',
        '{"clase":"Mammalia","alimentacion":"piscivoro","dieta_detalle":"Peces pelágicos, calamares y crustáceos.","comportamiento":{"actividad":"catemeral","social":"colonial","migratorio":false},"tamano_promedio_cm":170,"peso_promedio_g":60000,"reproduccion":"viviparo","epoca_reproductiva":"Paren una cría entre noviembre y diciembre en la colonia."}',
        '["IUCN Red List — Arctocephalus australis","Iriarte, A. — Mamíferos de Chile"]'
    ),
    (
        'Megaptera novaeangliae', 'Ballena jorobada', '(Borowski, 1781)',
        'Rorcual de aletas pectorales larguísimas, casi un tercio del cuerpo, y cola con diseño blanco y negro único en cada individuo, que permite identificarlos por fotografía. Es acrobática: salta fuera del agua y golpea la superficie con aletas y cola. Los machos cantan en las zonas de reproducción tropicales. Migra hacia aguas del sur de Chile para alimentarse en verano.',
        'Aguas costeras y oceánicas; golfos y canales productivos en verano.',
        'Visitante estival en el golfo de Corcovado y aguas al sur y oeste de la isla.',
        false, 'Preocupación Menor', 'animalia', 'Megaptera',
        '{"clase":"Mammalia","alimentacion":"filtrador","dieta_detalle":"Krill y peces de cardumen filtrados con las barbas, a veces cercados con redes de burbujas.","comportamiento":{"actividad":"catemeral","social":"gregario","migratorio":true},"tamano_promedio_cm":1400,"peso_promedio_g":30000000,"reproduccion":"viviparo","epoca_reproductiva":"Se reproduce en aguas tropicales en invierno; gestación cercana a un año."}',
        '["IUCN Red List — Megaptera novaeangliae","Centro Ballena Azul — Chiloé"]'
    ),
    -- -------------------------------------------------------------------------
    -- Anfibios y reptiles
    -- -------------------------------------------------------------------------
    (
        'Pleurodema thaul', 'Sapito de cuatro ojos', '(Schneider, 1799)',
        'Sapito de color muy variable, generalmente verde o pardo con manchas, que tiene en la parte posterior del cuerpo dos glándulas lumbares negras con borde claro. Al sentirse amenazado levanta la parte trasera y las muestra como si fueran un par de ojos grandes, y las glándulas secretan sustancias irritantes. Es el anfibio de distribución más amplia en Chile y se reproduce en charcas temporales.',
        'Charcas, acequias, vegas y bordes de lagunas, incluso en ambientes intervenidos.',
        'Común en humedales y charcas de toda la isla.',
        false, 'Preocupación Menor', 'animalia', 'Pleurodema',
        '{"clase":"Amphibia","alimentacion":"insectivoro","dieta_detalle":"Insectos y otros artrópodos pequeños.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":4,"peso_promedio_g":5,"reproduccion":"oviparo","epoca_reproductiva":"Pone huevos en cordones gelatinosos en charcas, de fines de invierno a primavera."}',
        '["IUCN Red List — Pleurodema thaul","Rabanal, F. & Núñez, J. — Anfibios de los Bosques Templados de Chile"]'
    ),
    (
        'Hylorina sylvatica', 'Rana esmeralda', 'Bell, 1843',
        'Una de las ranas más llamativas del bosque austral: dorso verde esmeralda o dorado con bandas laterales oscuras y vientre negro. Tiene dedos largos y buena capacidad de salto. Vive en pozas y pantanos dentro del bosque, donde los machos cantan desde la vegetación flotante. Charles Darwin colectó ejemplares en Chiloé durante el viaje del Beagle.',
        'Pantanos, pozas y turberas dentro o al borde del bosque templado.',
        'Presente en humedales boscosos de la Isla Grande; localidad de colecta histórica de Darwin.',
        false, 'Preocupación Menor', 'animalia', 'Hylorina',
        '{"clase":"Amphibia","alimentacion":"insectivoro","dieta_detalle":"Insectos y artrópodos capturados en la vegetación de la orilla.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":6,"peso_promedio_g":12,"reproduccion":"oviparo","epoca_reproductiva":"Se reproduce en primavera; los huevos se depositan en masas en el agua."}',
        '["IUCN Red List — Hylorina sylvatica","Rabanal, F. & Núñez, J. — Anfibios de los Bosques Templados de Chile"]'
    ),
    (
        'Batrachyla leptopus', 'Sapito moteado', 'Bell, 1843',
        'Sapito pequeño de patas finas, pardo con manchas oscuras y una franja clara variable en el dorso. A diferencia de muchos anuros, pone los huevos fuera del agua, en el musgo o bajo troncos cerca de charcas; los renacuajos completan el desarrollo cuando las lluvias inundan la puesta. Su canto es un golpeteo suave y repetido.',
        'Suelo húmedo, musgo y hojarasca de bosque y matorral cerca de charcas.',
        'Presente en bosques y turberas de la Isla Grande.',
        false, 'Preocupación Menor', 'animalia', 'Batrachyla',
        '{"clase":"Amphibia","alimentacion":"insectivoro","dieta_detalle":"Insectos y artrópodos pequeños del suelo.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":4,"peso_promedio_g":3,"reproduccion":"oviparo","epoca_reproductiva":"Otoño e invierno, con puestas terrestres que eclosionan al inundarse."}',
        '["IUCN Red List — Batrachyla leptopus","Rabanal, F. & Núñez, J. — Anfibios de los Bosques Templados de Chile"]'
    ),
    (
        'Liolaemus cyanogaster', 'Lagartija de vientre azul', '(Duméril & Bibron, 1837)',
        'Lagartija de bosque de colores vivos: dorso verdoso o pardo con manchas y, en los machos, un vientre azul o verde azulado muy llamativo. Es una de las pocas lagartijas adaptadas al bosque templado húmedo, donde busca claros y troncos al sol para termorregular. Es endémica de Chile.',
        'Claros, bordes y troncos soleados de bosque templado.',
        'Presente en la Isla Grande en claros y bordes de bosque.',
        true, 'Preocupación Menor', 'animalia', 'Liolaemus',
        '{"clase":"Reptilia","alimentacion":"insectivoro","dieta_detalle":"Insectos y arañas capturados al acecho.","comportamiento":{"actividad":"diurno","social":"solitario","migratorio":false},"tamano_promedio_cm":17,"peso_promedio_g":10,"reproduccion":"viviparo","epoca_reproductiva":"Las crías nacen a fines del verano."}',
        '["IUCN Red List — Liolaemus cyanogaster","Demangel, D. — Reptiles en Chile"]'
    ),
    -- -------------------------------------------------------------------------
    -- Peces
    -- -------------------------------------------------------------------------
    (
        'Galaxias maculatus', 'Puye', '(Jenyns, 1842)',
        'Pez pequeño, alargado y sin escamas, casi transparente de juvenil y moteado de adulto. Tiene un ciclo que lo lleva entre el mar y los ríos: las larvas se desarrollan en el mar y los juveniles remontan los estuarios en cardúmenes, momento en que se pescan como "puyes". Es uno de los peces de agua dulce con distribución natural más amplia del hemisferio sur.',
        'Ríos, esteros y lagos costeros; estuarios en la fase de migración.',
        'Presente en ríos y estuarios de toda la isla, pescado artesanalmente en primavera.',
        false, 'Preocupación Menor', 'animalia', 'Galaxias',
        '{"clase":"Actinopterygii","alimentacion":"insectivoro","dieta_detalle":"Larvas de insectos acuáticos, zooplancton y pequeños crustáceos.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":true},"tamano_promedio_cm":9,"peso_promedio_g":5,"reproduccion":"oviparo","epoca_reproductiva":"Desova en la vegetación ribereña de estuarios durante mareas altas."}',
        '["IUCN Red List — Galaxias maculatus","Habit, E. et al. — Peces nativos de agua dulce de Chile"]'
    ),
    (
        'Eleginops maclovinus', 'Róbalo', '(Cuvier, 1830)',
        'Pez costero de cuerpo robusto, plateado con dorso oscuro, que tolera desde agua de mar hasta agua dulce y entra a estuarios y ríos. Es el único miembro de su familia y pariente de los nototenioideos antárticos. Cambia de sexo a lo largo de la vida, de macho a hembra. Es un recurso tradicional de la pesca artesanal y deportiva chilota.',
        'Bahías, estuarios, desembocaduras y fondos someros.',
        'Común en bahías y estuarios del mar interior de Chiloé.',
        false, 'No Evaluada', 'animalia', 'Eleginops',
        '{"clase":"Actinopterygii","alimentacion":"omnivoro","dieta_detalle":"Algas, crustáceos, poliquetos y peces pequeños.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":45,"peso_promedio_g":1500,"reproduccion":"oviparo","epoca_reproductiva":"Desova en invierno y primavera en aguas costeras."}',
        '["FishBase — Eleginops maclovinus","SUBPESCA — Fichas de especies"]'
    ),
    (
        'Genypterus blacodes', 'Congrio dorado', '(Forster, 1801)',
        'Pez anguiliforme de cuerpo alargado, rosado anaranjado con manchas pardas, que vive cerca del fondo en aguas frías de la plataforma y el talud. Pese a su forma no es una anguila. Es un recurso pesquero importante del sur de Chile y base de platos tradicionales como el caldillo de congrio.',
        'Fondos blandos y rocosos de plataforma y talud continental.',
        'Presente en aguas exteriores y en el mar interior del archipiélago.',
        false, 'No Evaluada', 'animalia', 'Genypterus',
        '{"clase":"Actinopterygii","alimentacion":"carnivoro","dieta_detalle":"Crustáceos, peces y cefalópodos del fondo.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":100,"peso_promedio_g":3000,"reproduccion":"oviparo","epoca_reproductiva":"Desova en invierno y primavera."}',
        '["FishBase — Genypterus blacodes","SUBPESCA — Fichas de especies"]'
    ),
    (
        'Sprattus fuegensis', 'Sardina austral', '(Jenyns, 1842)',
        'Pequeño pez plateado de cardumen, pariente de las sardinas y arenques. Vive en los canales y fiordos del sur y en el mar interior de Chiloé, donde forma cardúmenes densos. Es una pieza clave de la trama alimentaria: sostiene a pingüinos, lobos marinos, delfines y aves pescadoras. Desde los años 2000 sostiene una pesquería de cerco en la zona.',
        'Aguas costeras, canales y fiordos, en la columna de agua.',
        'Abundante en el mar interior de Chiloé y el golfo de Ancud.',
        false, 'No Evaluada', 'animalia', 'Sprattus',
        '{"clase":"Actinopterygii","alimentacion":"filtrador","dieta_detalle":"Zooplancton, principalmente copépodos y larvas de crustáceos.","comportamiento":{"actividad":"diurno","social":"gregario","migratorio":false},"tamano_promedio_cm":14,"peso_promedio_g":25,"reproduccion":"oviparo","epoca_reproductiva":"Desova en primavera en aguas interiores."}',
        '["FishBase — Sprattus fuegensis","IFOP — Programa de seguimiento de pesquerías pelágicas"]'
    ),
    -- -------------------------------------------------------------------------
    -- Invertebrados
    -- -------------------------------------------------------------------------
    (
        'Aegla abtao', 'Pancora de Abtao', 'Schmitt, 1942',
        'Crustáceo de agua dulce parecido a un cangrejo pequeño, con caparazón aplanado y pinzas robustas. Las pancoras pertenecen a una familia que solo existe en Sudamérica y cuyo origen es marino. Esta especie lleva el nombre de la isla Abtao, en el archipiélago. Vive bajo piedras en esteros de aguas limpias y oxigenadas, por lo que es sensible a la contaminación y a la alteración de los cauces.',
        'Esteros y ríos de aguas frías y limpias, bajo piedras y troncos.',
        'Registrada en esteros del archipiélago, que incluye su localidad tipo.',
        true, 'No Evaluada', 'animalia', 'Aegla',
        '{"clase":"Malacostraca","alimentacion":"omnivoro","dieta_detalle":"Detritos, restos vegetales, larvas de insectos y pequeños invertebrados.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":3,"peso_promedio_g":5,"reproduccion":"oviparo","epoca_reproductiva":"Las hembras cargan los huevos bajo el abdomen hasta que eclosionan juveniles."}',
        '["Schmitt, W.L. 1942 — The species of Aegla","Bahamonde, N. et al. — Crustáceos decápodos de aguas continentales de Chile"]'
    ),
    (
        'Metacarcinus edwardsii', 'Jaiba marmola', '(Bell, 1835)',
        'Jaiba grande de caparazón ovalado ancho, rojizo con manchas claras, y pinzas de puntas negras. Vive en fondos de arena y fango de bahías y es el cangrejo de mayor importancia comercial en Chiloé. Tiene un rol de depredador y carroñero en los fondos del mar interior.',
        'Fondos arenosos y fangosos de bahías, desde el intermareal bajo hasta decenas de metros.',
        'Común en el mar interior del archipiélago; se desembarca en caletas de toda la isla.',
        false, 'No Evaluada', 'animalia', 'Metacarcinus',
        '{"clase":"Malacostraca","alimentacion":"carnivoro","dieta_detalle":"Bivalvos, caracoles, poliquetos y carroña.","comportamiento":{"actividad":"nocturno","social":"solitario","migratorio":false},"tamano_promedio_cm":14,"peso_promedio_g":400,"reproduccion":"oviparo","epoca_reproductiva":"Las hembras portan los huevos en invierno."}',
        '["SUBPESCA — Fichas de especies","Retamal, M. — Catálogo de crustáceos decápodos de Chile"]'
    ),
    (
        'Austromegabalanus psittacus', 'Picoroco', '(Molina, 1782)',
        'Uno de los cirrípedos más grandes del mundo. Aunque parece un molusco es un crustáceo que vive fijo a la roca dentro de un armazón de placas calcáreas, y saca unas patas plumosas para filtrar el agua. El nombre viene del pico curvo de las placas internas. En Chiloé es un ingrediente del curanto y de caldos tradicionales.',
        'Roqueríos del intermareal bajo y submareal somero con corrientes.',
        'Presente en roqueríos y canales del archipiélago; también se cultiva de forma experimental.',
        false, 'No Evaluada', 'animalia', 'Austromegabalanus',
        '{"clase":"Thecostraca","alimentacion":"filtrador","dieta_detalle":"Plancton y partículas en suspensión capturadas con los cirros.","comportamiento":{"actividad":"catemeral","social":"colonial","migratorio":false},"tamano_promedio_cm":15,"peso_promedio_g":200,"reproduccion":"oviparo","epoca_reproductiva":"Incuba huevos dentro del caparazón; las larvas son planctónicas."}',
        '["SUBPESCA — Fichas de especies","López, D. et al. — Estudios sobre el cultivo del picoroco"]'
    ),
    (
        'Pyura chilensis', 'Piure', 'Molina, 1782',
        'Ascidia solitaria con una túnica gruesa y rugosa por fuera y un cuerpo rojo intenso por dentro, que es lo que se come. Aunque parece una roca con dos sifones, es un cordado: sus larvas tienen notocordio, lo que la emparienta más con los vertebrados que con los moluscos. Filtra el agua y acumula vanadio en su sangre. Forma agregaciones densas en roqueríos.',
        'Roqueríos y fondos duros del submareal somero.',
        'Presente en roqueríos del litoral chilote; recurso de la pesca artesanal.',
        true, 'No Evaluada', 'animalia', 'Pyura',
        '{"clase":"Ascidiacea","alimentacion":"filtrador","dieta_detalle":"Fitoplancton y partículas orgánicas filtradas del agua.","comportamiento":{"actividad":"catemeral","social":"colonial","migratorio":false},"tamano_promedio_cm":12,"peso_promedio_g":200,"reproduccion":"oviparo","epoca_reproductiva":"Hermafrodita; libera gametos al agua y la larva nada brevemente antes de fijarse."}',
        '["SUBPESCA — Fichas de especies","Castilla, J.C. — Estudios ecológicos sobre Pyura chilensis"]'
    ),
    (
        'Tagelus dombeii', 'Navajuela', '(Lamarck, 1818)',
        'Bivalvo alargado de concha delgada y bordes paralelos que vive enterrado verticalmente en fondos de arena y fango. Asoma solo los sifones para filtrar el agua y se entierra rápidamente ante cualquier perturbación. Es extraído por recolectores de orilla en las playas del mar interior.',
        'Planicies arenosas y fangosas del intermareal y submareal somero.',
        'Presente en playas y bahías del mar interior de Chiloé.',
        false, 'No Evaluada', 'animalia', 'Tagelus',
        '{"clase":"Bivalvia","alimentacion":"filtrador","dieta_detalle":"Fitoplancton y materia orgánica en suspensión.","comportamiento":{"actividad":"catemeral","social":"gregario","migratorio":false},"tamano_promedio_cm":8,"peso_promedio_g":30,"reproduccion":"oviparo","epoca_reproductiva":"Desove en primavera y verano con larvas planctónicas."}',
        '["SUBPESCA — Fichas de especies","Osorio, C. — Moluscos marinos de Chile"]'
    )
) AS v(
    nombre_cientifico, nombre_comun, autor_cientifico, descripcion, habitat,
    distribucion_chiloe, endemica, estado_conservacion, reino, genero,
    atributos, fuentes
)
JOIN generos g ON g.nombre = v.genero
JOIN familias f ON f.id = g.familia_id AND f.reino = v.reino::reino_enum
ON CONFLICT (nombre_cientifico) DO NOTHING;

-- =============================================================================
-- Subgrupos de navegación de las familias nuevas
-- =============================================================================
INSERT INTO familia_subgrupo (familia, categoria_id)
SELECT m.familia, c.id
FROM (VALUES
    ('Accipitridae',    'animalia-aves'),
    ('Alcedinidae',     'animalia-aves'),
    ('Ardeidae',        'animalia-aves'),
    ('Charadriidae',    'animalia-aves'),
    ('Columbidae',      'animalia-aves'),
    ('Fringillidae',    'animalia-aves'),
    ('Hirundinidae',    'animalia-aves'),
    ('Passerellidae',   'animalia-aves'),
    ('Scolopacidae',    'animalia-aves'),
    ('Troglodytidae',   'animalia-aves'),
    ('Caenolestidae',   'animalia-mamiferos'),
    ('Leptodactylidae', 'animalia-anfibios'),
    ('Clupeidae',       'animalia-peces'),
    ('Eleginopidae',    'animalia-peces'),
    ('Galaxiidae',      'animalia-peces'),
    ('Ophidiidae',      'animalia-peces'),
    ('Aeglidae',        'animalia-invertebrados'),
    ('Balanidae',       'animalia-invertebrados'),
    ('Cancridae',       'animalia-invertebrados'),
    ('Pyuridae',        'animalia-invertebrados'),
    ('Solecurtidae',    'animalia-invertebrados')
) AS m(familia, slug)
JOIN categorias_moderacion c ON c.slug = m.slug
ON CONFLICT (familia) DO NOTHING;

-- Mismo criterio que el seed 0003: solo fichas sin categoría, para no pisar
-- una reclasificación hecha a mano.
UPDATE especies e
SET categoria_id = fs.categoria_id
FROM generos g
JOIN familias f          ON f.id = g.familia_id
JOIN familia_subgrupo fs ON fs.familia = f.nombre
JOIN categorias_moderacion c ON c.id = fs.categoria_id
WHERE e.genero_id = g.id
  AND e.categoria_id IS NULL
  AND c.reino = e.reino;

COMMIT;
