// Espejo de los contratos JSON de especies-api y auth-service. Solo los campos
// que el panel usa: ampliar aquí cuando se necesite alguno más.

export const REINOS = ['animalia', 'plantae', 'fungi', 'protista', 'monera'] as const;
export type Reino = (typeof REINOS)[number];

export type EspecieEstado = 'borrador' | 'publicada';

export const ROLES = ['admin', 'moderator', 'researcher', 'user'] as const;
export type RolUsuario = (typeof ROLES)[number];

export interface Usuario {
  id: number;
  email: string;
  name: string;
  role: RolUsuario;
}

export interface RespuestaLogin {
  user: Usuario;
  access_token: string;
  refresh_token: string;
  expires_in: number;
}

export interface Categoria {
  id: number;
  slug: string;
  nombre: string;
  reino: Reino;
  descripcion: string | null;
}

export interface Especie {
  id: number;
  reino: Reino;
  genero_id: number;
  genero_nombre?: string;
  nombre_cientifico: string;
  nombre_comun: string;
  autor_cientifico: string;
  descripcion: string;
  habitat: string;
  distribucion_chiloe: string;
  endemica: boolean;
  estado_conservacion: string;
  fuentes: unknown[];
  geo_lat: number | null;
  geo_lng: number | null;
  atributos_especificos: Record<string, unknown>;
  foto_portada_key: string | null;
  fotos_keys: string[];
  categoria_id: number | null;
  estado: EspecieEstado;
  publicado_por: number | null;
  fecha_publicacion: string | null;
  created_at: string | null;
  updated_at: string | null;
}

export interface Familia {
  id: number;
  nombre: string;
  reino: Reino;
}

export interface Genero {
  id: number;
  nombre: string;
  familia_id: number;
}

export type PostulacionEstado = 'pendiente' | 'aprobada' | 'rechazada';

export interface Postulacion {
  id: number;
  usuario_id: number;
  categoria_id: number;
  texto: string;
  estado: PostulacionEstado;
  revisado_por: number | null;
  revisado_en: string | null;
  motivo: string | null;
  created_at: string | null;
}

export type AvistamientoEstado = 'pendiente' | 'aprobado' | 'rechazado';

export interface Avistamiento {
  id: number;
  especie_id: number | null;
  reino: Reino;
  nombre_sugerido: string | null;
  descripcion: string | null;
  foto_key: string;
  geo_lat: number;
  geo_lng: number;
  observado_en: string | null;
  creado_por: number | null;
  estado: AvistamientoEstado;
  motivo_rechazo: string | null;
  created_at: string | null;
}

export interface PresignedUpload {
  success: boolean;
  method: string;
  bucket: string;
  key: string;
  url: string;
  headers: Record<string, string>;
  expires_in: number;
}

// Subconjunto de JSON Schema que el formulario sabe renderizar. Lo que no
// encaje aquí se muestra como aviso en vez de silenciarse.
export interface JsonSchema {
  type?: string;
  title?: string;
  description?: string;
  enum?: (string | number)[];
  required?: string[];
  properties?: Record<string, JsonSchema>;
  items?: JsonSchema;
  minimum?: number;
  maximum?: number;
  maxLength?: number;
  uniqueItems?: boolean;
  additionalProperties?: boolean;
}

// ----- usuarios de curaduría -----
//
// El listado lo arma especies-api cruzando los usuarios del auth-service con
// las asignaciones de curaduría de su propia base (ADR #27). Los nombres de
// campo son los del backend (`rol`, no `role`): es otra respuesta, no la del
// login.

export interface CategoriaCurada {
  id: number;
  slug: string;
  nombre: string;
  reino: Reino;
  total_especies: number;
  descripcion?: string | null;
}

export interface UsuarioDeCuraduria {
  id: number;
  // Cuando el auth-service no respondió estos campos llegan vacíos: la fila
  // trae solo el id, que es lo único que especies-api sabe por sí misma.
  email: string;
  nombre: string;
  avatar: string;
  rol: RolUsuario | '';
  estado: string;
  profesion: string;
  perfil_publico: boolean;
  creado_en: string;
  categorias_curadas: CategoriaCurada[];
}

export interface PaginaDeUsuarios {
  success: boolean;
  usuarios: UsuarioDeCuraduria[];
  total: number;
  pagina: number;
  por_pagina: number;
  // false = el auth-service no contestó y la página viene degradada: solo los
  // ids que curan algo, sin nombre ni email. La pantalla lo avisa.
  auth_disponible: boolean;
}

// ----- insignias -----

export type TipoInsignia = 'automatica' | 'rol';

export interface Insignia {
  id: number;
  codigo: string;
  nombre: string;
  descripcion: string;
  criterio: string;
  // Las `automatica` las otorga el recálculo por métrica y umbral; solo las
  // `rol` se dan y se quitan a mano.
  tipo: TipoInsignia;
  metrica: string | null;
  umbral: number | null;
}

export interface InsigniaOtorgada extends Insignia {
  otorgada_en: string;
  otorgada_por: number | null;
  motivo: string | null;
}
