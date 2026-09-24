import { useCallback, useEffect, useState } from 'react';
import { useSesion } from '../auth/sesion';
import { Aviso } from '../componentes/Aviso';
import {
  ROLES,
  type Categoria,
  type Insignia,
  type InsigniaOtorgada,
  type RolUsuario,
  type UsuarioDeCuraduria,
} from '../api/tipos';
import {
  categoriasAsignables,
  insigniasDisponibles,
  insigniasPorUsuario,
  nombreDeUsuario,
  rangoMostrado,
  totalDePaginas,
} from './usuarios';

const POR_PAGINA = 20;

export function Usuarios() {
  const { api, usuario: sesion } = useSesion();
  const esAdmin = sesion?.role === 'admin';

  const [usuarios, setUsuarios] = useState<UsuarioDeCuraduria[]>([]);
  const [total, setTotal] = useState(0);
  const [authDisponible, setAuthDisponible] = useState(true);
  const [pagina, setPagina] = useState(1);
  // Dos estados para la búsqueda: lo que se escribe y lo que se pidió. Si
  // fueran uno, cada tecla dispararía una consulta al auth-service.
  const [textoBusqueda, setTextoBusqueda] = useState('');
  const [buscar, setBuscar] = useState('');
  const [rol, setRol] = useState<RolUsuario | ''>('');

  const [categorias, setCategorias] = useState<Categoria[]>([]);
  const [catalogo, setCatalogo] = useState<Insignia[]>([]);
  const [insignias, setInsignias] = useState<Map<number, InsigniaOtorgada[]>>(new Map());

  // Qué eligió cada fila en sus dos desplegables, indexado por id de usuario.
  const [categoriaElegida, setCategoriaElegida] = useState<Record<number, string>>({});
  const [insigniaElegida, setInsigniaElegida] = useState<Record<number, string>>({});

  const [error, setError] = useState<string | null>(null);
  const [aviso, setAviso] = useState<string | null>(null);
  const [cargando, setCargando] = useState(true);
  const [ocupado, setOcupado] = useState(false);

  useEffect(() => {
    api
      .categorias()
      .then(setCategorias)
      .catch(() => setCategorias([]));
    api
      .insignias()
      .then(setCatalogo)
      .catch(() => setCatalogo([]));
  }, [api]);

  const cargar = useCallback(() => {
    setCargando(true);
    setError(null);
    api
      .usuariosDeCuraduria({ pagina, por_pagina: POR_PAGINA, buscar, rol })
      .then(async (respuesta) => {
        setUsuarios(respuesta.usuarios);
        setTotal(respuesta.total);
        setAuthDisponible(respuesta.auth_disponible);
        // Las insignias son una llamada aparte y no deben tumbar el listado:
        // sin ellas la tabla sigue sirviendo para asignar categorías.
        try {
          const porUsuario = await api.insigniasDeVarios(respuesta.usuarios.map((u) => u.id));
          setInsignias(insigniasPorUsuario(porUsuario));
        } catch {
          setInsignias(new Map());
        }
      })
      .catch((fallo) =>
        setError(
          fallo instanceof Error ? fallo.message : 'No se pudieron cargar los usuarios',
        ),
      )
      .finally(() => setCargando(false));
  }, [api, buscar, pagina, rol]);

  useEffect(cargar, [cargar]);

  async function conAccion(descripcion: string, accion: () => Promise<void>) {
    setError(null);
    setAviso(null);
    setOcupado(true);
    try {
      await accion();
      setAviso(descripcion);
      cargar();
    } catch (fallo) {
      setError(fallo instanceof Error ? fallo.message : 'No se pudo completar la acción');
    } finally {
      setOcupado(false);
    }
  }

  function asignar(usuario: UsuarioDeCuraduria) {
    const elegida = categoriaElegida[usuario.id];
    if (!elegida) return;
    const categoria = categorias.find((c) => String(c.id) === elegida);
    void conAccion(
      `${nombreDeUsuario(usuario)} ahora cura ${categoria?.nombre ?? `#${elegida}`}.`,
      async () => {
        await api.asignarCategoria(Number(elegida), usuario.id);
        setCategoriaElegida((previo) => ({ ...previo, [usuario.id]: '' }));
      },
    );
  }

  function quitar(usuario: UsuarioDeCuraduria, categoriaId: number, nombre: string) {
    if (!window.confirm(`¿Quitarle ${nombre} a ${nombreDeUsuario(usuario)}?`)) return;
    void conAccion(`${nombreDeUsuario(usuario)} ya no cura ${nombre}.`, () =>
      api.quitarCategoria(categoriaId, usuario.id).then(() => undefined),
    );
  }

  function otorgar(usuario: UsuarioDeCuraduria) {
    const codigo = insigniaElegida[usuario.id];
    if (!codigo) return;
    // El motivo queda guardado con la insignia: es la única explicación de por
    // qué esta se dio a mano y no la calculó el recálculo.
    const escrito = window.prompt('Motivo (opcional, queda registrado):');
    if (escrito === null) return;
    const motivo = escrito.trim() === '' ? undefined : escrito.trim();
    void conAccion(`Insignia ${codigo} otorgada a ${nombreDeUsuario(usuario)}.`, async () => {
      await api.otorgarInsignia(usuario.id, codigo, motivo);
      setInsigniaElegida((previo) => ({ ...previo, [usuario.id]: '' }));
    });
  }

  function revocar(usuario: UsuarioDeCuraduria, codigo: string) {
    if (!window.confirm(`¿Revocar la insignia ${codigo} a ${nombreDeUsuario(usuario)}?`)) return;
    void conAccion(`Insignia ${codigo} revocada.`, () =>
      api.revocarInsignia(usuario.id, codigo).then(() => undefined),
    );
  }

  function recalcular() {
    void conAccion('Insignias recalculadas.', async () => {
      const resultado = await api.recalcularInsignias();
      setAviso(
        resultado.otorgadas === 0
          ? 'Recálculo hecho: nadie cumplió un criterio nuevo.'
          : `Recálculo hecho: ${resultado.otorgadas} insignia(s) otorgadas.`,
      );
    });
  }

  const paginas = totalDePaginas(total, POR_PAGINA);

  return (
    <section>
      <header className="cabecera-seccion">
        <h1>Usuarios</h1>
        <form
          className="filtros"
          onSubmit={(e) => {
            e.preventDefault();
            setBuscar(textoBusqueda.trim());
            setPagina(1);
          }}
        >
          <label>
            Buscar
            <input
              type="search"
              value={textoBusqueda}
              placeholder="nombre o correo"
              onChange={(e) => setTextoBusqueda(e.target.value)}
            />
          </label>
          <label>
            Rol
            <select
              value={rol}
              onChange={(e) => {
                setRol(e.target.value as RolUsuario | '');
                setPagina(1);
              }}
            >
              <option value="">Todos</option>
              {ROLES.map((valor) => (
                <option key={valor} value={valor}>
                  {valor}
                </option>
              ))}
            </select>
          </label>
          <button type="submit">Buscar</button>
        </form>
        <button type="button" disabled={ocupado || !esAdmin} onClick={recalcular}>
          Recalcular insignias
        </button>
      </header>

      <p className="ayuda">
        Asignar una categoría da curaduría sobre ella sin cambiar el rol global de la cuenta. Las
        insignias que se pueden dar a mano son solo las de tipo <code>rol</code>: las automáticas
        las decide el recálculo a partir de su métrica.
      </p>

      {!esAdmin && (
        <Aviso tono="atencion">
          Esta pantalla requiere rol admin: el listado de usuarios y los cambios de curaduría los
          responde la API solo a un admin.
        </Aviso>
      )}

      {!authDisponible && (
        <Aviso tono="atencion">
          El auth-service no respondió. La tabla muestra solo los ids que ya curan alguna
          categoría, sin nombre, correo ni rol, y ni la búsqueda ni el filtro de rol se aplican.
          Lo que se vea aquí está incompleto.
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

      {cargando ? (
        <p>Cargando…</p>
      ) : usuarios.length === 0 ? (
        <p>No hay usuarios que coincidan con el filtro.</p>
      ) : (
        <table className="tabla">
          <thead>
            <tr>
              <th>Usuario</th>
              <th>Rol</th>
              <th>Categorías que cura</th>
              <th>Insignias</th>
            </tr>
          </thead>
          <tbody>
            {usuarios.map((usuario) => {
              const asignables = categoriasAsignables(categorias, usuario.categorias_curadas);
              const otorgadas = insignias.get(usuario.id) ?? [];
              const disponibles = insigniasDisponibles(catalogo, otorgadas);

              return (
                <tr key={usuario.id}>
                  <td>
                    {nombreDeUsuario(usuario)}
                    {usuario.email && <span className="ayuda">{usuario.email}</span>}
                    {usuario.profesion && (
                      // Declarada por la persona, sin verificar: el auth-service
                      // no tiene hoy dónde guardar esa verificación.
                      <span className="ayuda">
                        Profesión declarada: {usuario.profesion} (sin verificar)
                      </span>
                    )}
                  </td>
                  <td>{usuario.rol === '' ? '—' : usuario.rol}</td>
                  <td>
                    {usuario.categorias_curadas.length === 0 ? (
                      <span className="ayuda">Ninguna</span>
                    ) : (
                      <ul className="lista-chips">
                        {usuario.categorias_curadas.map((categoria) => (
                          <li key={categoria.id}>
                            <span className="pastilla">{categoria.nombre}</span>
                            <button
                              type="button"
                              disabled={ocupado || !esAdmin}
                              onClick={() => quitar(usuario, categoria.id, categoria.nombre)}
                            >
                              Quitar
                            </button>
                          </li>
                        ))}
                      </ul>
                    )}
                    {asignables.length > 0 && (
                      <div className="acciones-celda">
                        <select
                          aria-label={`Categoría para ${nombreDeUsuario(usuario)}`}
                          value={categoriaElegida[usuario.id] ?? ''}
                          onChange={(e) =>
                            setCategoriaElegida((previo) => ({
                              ...previo,
                              [usuario.id]: e.target.value,
                            }))
                          }
                        >
                          <option value="">Añadir categoría…</option>
                          {asignables.map((categoria) => (
                            <option key={categoria.id} value={categoria.id}>
                              {categoria.nombre}
                            </option>
                          ))}
                        </select>
                        <button
                          type="button"
                          disabled={ocupado || !esAdmin || !categoriaElegida[usuario.id]}
                          onClick={() => asignar(usuario)}
                        >
                          Asignar
                        </button>
                      </div>
                    )}
                  </td>
                  <td>
                    {otorgadas.length === 0 ? (
                      <span className="ayuda">Ninguna</span>
                    ) : (
                      <ul className="lista-chips">
                        {otorgadas.map((insignia) => (
                          <li key={insignia.codigo}>
                            <span className="pastilla" title={insignia.criterio}>
                              {insignia.nombre}
                            </span>
                            {insignia.tipo === 'rol' && (
                              <button
                                type="button"
                                disabled={ocupado || !esAdmin}
                                onClick={() => revocar(usuario, insignia.codigo)}
                              >
                                Revocar
                              </button>
                            )}
                          </li>
                        ))}
                      </ul>
                    )}
                    {disponibles.length > 0 && (
                      <div className="acciones-celda">
                        <select
                          aria-label={`Insignia para ${nombreDeUsuario(usuario)}`}
                          value={insigniaElegida[usuario.id] ?? ''}
                          onChange={(e) =>
                            setInsigniaElegida((previo) => ({
                              ...previo,
                              [usuario.id]: e.target.value,
                            }))
                          }
                        >
                          <option value="">Otorgar insignia…</option>
                          {disponibles.map((insignia) => (
                            <option key={insignia.codigo} value={insignia.codigo}>
                              {insignia.nombre}
                            </option>
                          ))}
                        </select>
                        <button
                          type="button"
                          disabled={ocupado || !esAdmin || !insigniaElegida[usuario.id]}
                          onClick={() => otorgar(usuario)}
                        >
                          Otorgar
                        </button>
                      </div>
                    )}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      )}

      <nav className="paginacion">
        <button type="button" disabled={pagina <= 1} onClick={() => setPagina(pagina - 1)}>
          Anterior
        </button>
        <span>{rangoMostrado(pagina, POR_PAGINA, total)}</span>
        <button type="button" disabled={pagina >= paginas} onClick={() => setPagina(pagina + 1)}>
          Siguiente
        </button>
      </nav>
    </section>
  );
}
