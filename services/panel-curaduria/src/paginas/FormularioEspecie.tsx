import { useEffect, useMemo, useState } from "react";
import { useNavigate, useParams } from "react-router-dom";
import { useSesion } from "../auth/sesion";
import { Aviso } from "../componentes/Aviso";
import { SubidaFotos } from "../componentes/SubidaFotos";
import { FormularioAtributos } from "../schema/FormularioAtributos";
import {
  atributosParaEnviar,
  camposDeSchema,
  faltantesRequeridos,
} from "../schema/campos";
import {
  REINOS,
  type Especie,
  type Familia,
  type Genero,
  type JsonSchema,
  type Reino,
} from "../api/tipos";

interface Borrador {
  reino: Reino;
  genero_id: number | "";
  categoria_id: number | "";
  nombre_cientifico: string;
  nombre_comun: string;
  autor_cientifico: string;
  descripcion: string;
  habitat: string;
  distribucion_chiloe: string;
  endemica: boolean;
  estado_conservacion: string;
  atributos_especificos: Record<string, unknown>;
  fotos_keys: string[];
  foto_portada_key: string | null;
}

const BORRADOR_VACIO: Borrador = {
  reino: "plantae",
  genero_id: "",
  categoria_id: "",
  nombre_cientifico: "",
  nombre_comun: "",
  autor_cientifico: "",
  descripcion: "",
  habitat: "",
  distribucion_chiloe: "",
  endemica: false,
  estado_conservacion: "",
  atributos_especificos: {},
  fotos_keys: [],
  foto_portada_key: null,
};

function desdeEspecie(especie: Especie): Borrador {
  return {
    reino: especie.reino,
    genero_id: especie.genero_id,
    categoria_id: especie.categoria_id ?? "",
    nombre_cientifico: especie.nombre_cientifico,
    nombre_comun: especie.nombre_comun,
    autor_cientifico: especie.autor_cientifico,
    descripcion: especie.descripcion,
    habitat: especie.habitat,
    distribucion_chiloe: especie.distribucion_chiloe,
    endemica: especie.endemica,
    estado_conservacion: especie.estado_conservacion,
    atributos_especificos: especie.atributos_especificos ?? {},
    fotos_keys: especie.fotos_keys ?? [],
    foto_portada_key: especie.foto_portada_key,
  };
}

export function FormularioEspecie() {
  const { id } = useParams();
  const navegar = useNavigate();
  const { api, categoriasCuradas } = useSesion();
  const esNueva = id === undefined;

  const [borrador, setBorrador] = useState<Borrador>(BORRADOR_VACIO);
  const [especie, setEspecie] = useState<Especie | null>(null);
  const [generos, setGeneros] = useState<Genero[]>([]);
  const [familias, setFamilias] = useState<Familia[]>([]);
  const [schema, setSchema] = useState<JsonSchema | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [aviso, setAviso] = useState<string | null>(null);
  const [cargando, setCargando] = useState(!esNueva);
  const [guardando, setGuardando] = useState(false);

  useEffect(() => {
    Promise.all([api.generos(), api.familias()])
      .then(([generosCargados, familiasCargadas]) => {
        setGeneros(generosCargados);
        setFamilias(familiasCargadas);
      })
      .catch(() => {
        setGeneros([]);
        setFamilias([]);
      });
  }, [api]);

  useEffect(() => {
    if (esNueva) return;
    setCargando(true);
    api
      .especie(Number(id))
      .then((cargada) => {
        setEspecie(cargada);
        setBorrador(desdeEspecie(cargada));
      })
      .catch((fallo) =>
        setError(
          fallo instanceof Error ? fallo.message : "No se pudo cargar la ficha",
        ),
      )
      .finally(() => setCargando(false));
  }, [api, id, esNueva]);

  // El schema se recarga al cambiar de reino: los atributos de una planta no
  // valen para un hongo, y el servidor los rechaza si sobran.
  useEffect(() => {
    let vigente = true;
    api
      .schema(borrador.reino)
      .then((cargado) => {
        if (vigente) setSchema(cargado);
      })
      .catch(() => {
        if (vigente) setSchema(null);
      });
    return () => {
      vigente = false;
    };
  }, [api, borrador.reino]);

  const campos = useMemo(
    () => (schema ? camposDeSchema(schema) : []),
    [schema],
  );

  // El género no lleva reino: lo hereda de su familia. Se filtra para que no se
  // pueda colgar un canelo de un género de aves.
  const generosDelReino = useMemo(() => {
    const reinoDeFamilia = new Map(
      familias.map((familia) => [familia.id, familia.reino]),
    );
    return generos.filter(
      (genero) => reinoDeFamilia.get(genero.familia_id) === borrador.reino,
    );
  }, [generos, familias, borrador.reino]);

  function actualizar<K extends keyof Borrador>(clave: K, valor: Borrador[K]) {
    setBorrador((actual) => ({ ...actual, [clave]: valor }));
  }

  async function guardar(evento: React.FormEvent) {
    evento.preventDefault();
    setError(null);
    setAviso(null);

    const faltantes = faltantesRequeridos(
      campos,
      borrador.atributos_especificos,
    );
    if (faltantes.length > 0) {
      setError(
        `Faltan campos obligatorios del reino ${borrador.reino}: ${faltantes.join(", ")}`,
      );
      return;
    }
    if (borrador.genero_id === "") {
      setError("Elige un género.");
      return;
    }

    // `fotos_keys` y `foto_portada_key` viajan siempre en el cuerpo: el PUT
    // reescribe esas columnas, así que omitirlas borraría las fotos guardadas.
    const cuerpo = {
      ...borrador,
      genero_id: Number(borrador.genero_id),
      categoria_id:
        borrador.categoria_id === "" ? null : Number(borrador.categoria_id),
      atributos_especificos: atributosParaEnviar(
        borrador.atributos_especificos,
      ),
    };

    setGuardando(true);
    try {
      if (esNueva) {
        const creada = await api.crearEspecie(cuerpo);
        navegar(`/especies/${creada.id}`, { replace: true });
        setAviso("Ficha creada como borrador. Publícala cuando esté lista.");
      } else {
        const actualizada = await api.actualizarEspecie(Number(id), cuerpo);
        setEspecie(actualizada);
        setBorrador(desdeEspecie(actualizada));
        setAviso("Cambios guardados.");
      }
    } catch (fallo) {
      setError(fallo instanceof Error ? fallo.message : "No se pudo guardar");
    } finally {
      setGuardando(false);
    }
  }

  async function cambiarEstado() {
    if (!especie) return;
    setError(null);
    setAviso(null);
    try {
      const actualizada =
        especie.estado === "borrador"
          ? await api.publicar(especie.id)
          : await api.despublicar(especie.id);
      setEspecie(actualizada);
      setAviso(
        actualizada.estado === "publicada"
          ? "Ficha publicada."
          : "Ficha devuelta a borrador.",
      );
    } catch (fallo) {
      setError(
        fallo instanceof Error ? fallo.message : "No se pudo cambiar el estado",
      );
    }
  }

  if (cargando) return <p>Cargando ficha…</p>;

  return (
    <section>
      <header className="cabecera-seccion">
        <h1>
          {esNueva ? "Nueva ficha" : borrador.nombre_cientifico || "Ficha"}
        </h1>
        {especie && (
          <div className="acciones">
            <span className={`pastilla pastilla--${especie.estado}`}>
              {especie.estado}
            </span>
            <button type="button" onClick={cambiarEstado}>
              {especie.estado === "borrador" ? "Publicar" : "Despublicar"}
            </button>
          </div>
        )}
      </header>

      {esNueva && (
        <Aviso tono="atencion">
          La ficha nace como <strong>borrador</strong>: no la ve nadie fuera de
          la curaduría hasta que la publiques.
        </Aviso>
      )}

      {borrador.reino === "fungi" && (
        <Aviso tono="atencion">
          <strong>Hongos:</strong> la comestibilidad es obligatoria y la ficha
          se publica con una advertencia sanitaria. Ninguna ficha de este panel
          sirve para decidir si un hongo se puede comer: ante la duda, marca{" "}
          <em>desconocido</em> y explica las confusiones peligrosas en el campo
          de advertencia.
        </Aviso>
      )}

      {error && (
        <Aviso tono="error" desplazar>
          {error}
        </Aviso>
      )}
      {aviso && (
        <Aviso tono="exito" desplazar>
          {aviso}
        </Aviso>
      )}

      <form className="formulario" onSubmit={guardar}>
        <fieldset className="grupo">
          <legend>Identificación</legend>

          <label>
            Reino
            <select
              value={borrador.reino}
              onChange={(e) => {
                // Cambiar de reino invalida los atributos anteriores.
                setBorrador((actual) => ({
                  ...actual,
                  reino: e.target.value as Reino,
                  atributos_especificos: {},
                }));
              }}
            >
              {REINOS.map((reino) => (
                <option key={reino} value={reino}>
                  {reino}
                </option>
              ))}
            </select>
          </label>

          <label>
            Nombre científico <span className="requerido">*</span>
            <input
              type="text"
              required
              maxLength={200}
              value={borrador.nombre_cientifico}
              onChange={(e) => actualizar("nombre_cientifico", e.target.value)}
            />
          </label>

          <label>
            Nombre común
            <input
              type="text"
              maxLength={200}
              value={borrador.nombre_comun}
              onChange={(e) => actualizar("nombre_comun", e.target.value)}
            />
          </label>

          <label>
            Autor científico
            <input
              type="text"
              maxLength={200}
              value={borrador.autor_cientifico}
              onChange={(e) => actualizar("autor_cientifico", e.target.value)}
            />
          </label>

          <label>
            Género <span className="requerido">*</span>
            <select
              value={borrador.genero_id}
              onChange={(e) =>
                actualizar(
                  "genero_id",
                  e.target.value === "" ? "" : Number(e.target.value),
                )
              }
              required
            >
              <option value="">— elige —</option>
              {generosDelReino.map((genero) => (
                <option key={genero.id} value={genero.id}>
                  {genero.nombre}
                </option>
              ))}
            </select>
          </label>

          <label>
            Categoría de curaduría <span className="requerido">*</span>
            <select
              value={borrador.categoria_id}
              onChange={(e) =>
                actualizar(
                  "categoria_id",
                  e.target.value === "" ? "" : Number(e.target.value),
                )
              }
              required
            >
              <option value="">— elige —</option>
              {categoriasCuradas.map((categoria) => (
                <option key={categoria.id} value={categoria.id}>
                  {categoria.nombre} ({categoria.reino})
                </option>
              ))}
            </select>
            <span className="ayuda">
              Solo aparecen las categorías sobre las que tienes curaduría: son
              las únicas en las que puedes guardar.
            </span>
          </label>
        </fieldset>

        <fieldset className="grupo">
          <legend>Divulgación</legend>

          <label>
            Descripción
            <textarea
              rows={5}
              value={borrador.descripcion}
              onChange={(e) => actualizar("descripcion", e.target.value)}
            />
          </label>

          <label>
            Hábitat
            <textarea
              rows={3}
              value={borrador.habitat}
              onChange={(e) => actualizar("habitat", e.target.value)}
            />
          </label>

          <label>
            Distribución en Chiloé
            <textarea
              rows={3}
              value={borrador.distribucion_chiloe}
              onChange={(e) =>
                actualizar("distribucion_chiloe", e.target.value)
              }
            />
          </label>

          <label className="casilla">
            <input
              type="checkbox"
              checked={borrador.endemica}
              onChange={(e) => actualizar("endemica", e.target.checked)}
            />
            Endémica de Chiloé
          </label>

          <label>
            Estado de conservación
            <input
              type="text"
              maxLength={60}
              placeholder="LC, NT, VU, EN, CR…"
              value={borrador.estado_conservacion}
              onChange={(e) =>
                actualizar("estado_conservacion", e.target.value)
              }
            />
          </label>
        </fieldset>

        <fieldset className="grupo">
          <legend>Atributos de {borrador.reino}</legend>
          <p className="ayuda">
            Estos campos vienen del JSON Schema que valida el servidor, así que
            lo que aquí se acepta es exactamente lo que se acepta al guardar.
          </p>
          <FormularioAtributos
            campos={campos}
            atributos={borrador.atributos_especificos}
            onChange={(atributos) =>
              actualizar("atributos_especificos", atributos)
            }
          />
        </fieldset>

        <fieldset className="grupo">
          <legend>Fotos</legend>
          <SubidaFotos
            fotosKeys={borrador.fotos_keys}
            portada={borrador.foto_portada_key}
            creditos={especie?.fotos_creditos}
            onChange={({ fotos_keys, foto_portada_key }) =>
              setBorrador((actual) => ({
                ...actual,
                fotos_keys,
                foto_portada_key,
              }))
            }
          />
        </fieldset>

        <div className="acciones">
          <button type="submit" disabled={guardando}>
            {guardando ? "Guardando…" : "Guardar"}
          </button>
          <button type="button" onClick={() => navegar("/especies")}>
            Volver
          </button>
        </div>
      </form>
    </section>
  );
}
