// Lógica de la pantalla de usuarios que no necesita el DOM. Vive aparte para
// poder probarla: los tests del panel corren en `node`, sin renderizar.

import type {
  Categoria,
  CategoriaCurada,
  Insignia,
  InsigniaOtorgada,
  UsuarioDeCuraduria,
} from '../api/tipos';

/**
 * Con qué nombre se muestra a alguien. Cuando el auth-service no respondió la
 * fila llega solo con el id: mostrar "#7" es honesto, inventar un nombre no.
 */
export function nombreDeUsuario(usuario: UsuarioDeCuraduria): string {
  if (usuario.nombre.trim() !== '') return usuario.nombre;
  if (usuario.email.trim() !== '') return usuario.email;
  return `#${usuario.id}`;
}

/** Páginas que hay en total. Nunca menos de una: la vacía también se muestra. */
export function totalDePaginas(total: number, porPagina: number): number {
  if (porPagina <= 0 || total <= 0) return 1;
  return Math.ceil(total / porPagina);
}

/** "21–40 de 137" para el pie de la tabla. */
export function rangoMostrado(pagina: number, porPagina: number, total: number): string {
  if (total <= 0) return '0 de 0';
  const desde = (Math.max(1, pagina) - 1) * porPagina + 1;
  return `${desde}–${Math.min(desde + porPagina - 1, total)} de ${total}`;
}

/**
 * Categorías que todavía se le pueden asignar. Ofrecer una que ya cura es
 * ofrecer un botón que responde "ya existía".
 */
export function categoriasAsignables(
  todas: Categoria[],
  yaCuradas: CategoriaCurada[],
): Categoria[] {
  const asignadas = new Set(yaCuradas.map((categoria) => categoria.id));
  return todas.filter((categoria) => !asignadas.has(categoria.id));
}

/**
 * Insignias que un admin puede dar a mano: solo las de tipo `rol`. Las
 * `automatica` las decide el recálculo a partir de una métrica, así que
 * otorgarlas a dedo las dejaría en contradicción con su propio criterio.
 */
export function insigniasOtorgablesAMano(catalogo: Insignia[]): Insignia[] {
  return catalogo.filter((insignia) => insignia.tipo === 'rol');
}

/** Las de ese tipo que la persona aún no tiene. */
export function insigniasDisponibles(
  catalogo: Insignia[],
  yaOtorgadas: InsigniaOtorgada[],
): Insignia[] {
  const tiene = new Set(yaOtorgadas.map((insignia) => insignia.codigo));
  return insigniasOtorgablesAMano(catalogo).filter((insignia) => !tiene.has(insignia.codigo));
}

/**
 * La API devuelve las insignias de varios usuarios como objeto indexado por id
 * en texto (`{"7": [...]}`). Acá se pasa a un mapa por número, que es como la
 * tabla las busca.
 */
export function insigniasPorUsuario(
  respuesta: Record<string, InsigniaOtorgada[]>,
): Map<number, InsigniaOtorgada[]> {
  const mapa = new Map<number, InsigniaOtorgada[]>();
  for (const [clave, otorgadas] of Object.entries(respuesta)) {
    const id = Number(clave);
    if (!Number.isInteger(id)) continue;
    mapa.set(id, otorgadas);
  }
  return mapa;
}
