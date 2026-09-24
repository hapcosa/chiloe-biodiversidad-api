import { describe, expect, it } from 'vitest';
import {
  categoriasAsignables,
  insigniasDisponibles,
  insigniasOtorgablesAMano,
  insigniasPorUsuario,
  nombreDeUsuario,
  rangoMostrado,
  totalDePaginas,
} from './usuarios';
import type {
  Categoria,
  CategoriaCurada,
  Insignia,
  InsigniaOtorgada,
  UsuarioDeCuraduria,
} from '../api/tipos';

const usuario = (parcial: Partial<UsuarioDeCuraduria>): UsuarioDeCuraduria => ({
  id: 1,
  email: '',
  nombre: '',
  avatar: '',
  rol: '',
  estado: '',
  profesion: '',
  perfil_publico: false,
  creado_en: '',
  categorias_curadas: [],
  ...parcial,
});

const categoria = (id: number, nombre: string): Categoria => ({
  id,
  slug: nombre.toLowerCase(),
  nombre,
  reino: 'animalia',
  descripcion: null,
});

const curada = (id: number, nombre: string): CategoriaCurada => ({
  id,
  slug: nombre.toLowerCase(),
  nombre,
  reino: 'animalia',
  total_especies: 0,
});

const insignia = (codigo: string, tipo: Insignia['tipo']): Insignia => ({
  id: 1,
  codigo,
  nombre: codigo,
  descripcion: '',
  criterio: '',
  tipo,
  metrica: null,
  umbral: null,
});

const otorgada = (codigo: string, tipo: Insignia['tipo']): InsigniaOtorgada => ({
  ...insignia(codigo, tipo),
  otorgada_en: '2026-01-01T00:00:00Z',
  otorgada_por: 1,
  motivo: null,
});

describe('nombreDeUsuario', () => {
  it('prefiere el nombre', () => {
    expect(nombreDeUsuario(usuario({ nombre: 'Ada', email: 'ada@x.cl' }))).toBe('Ada');
  });

  it('cae al email cuando no hay nombre', () => {
    expect(nombreDeUsuario(usuario({ email: 'ada@x.cl' }))).toBe('ada@x.cl');
  });

  // Es el caso degradado: el auth-service no respondió y la fila trae solo el
  // id. Mostrar "#7" avisa de que falta un dato; un nombre inventado, no.
  it('cae al id cuando la fila viene sin nombre ni email', () => {
    expect(nombreDeUsuario(usuario({ id: 7 }))).toBe('#7');
  });

  it('no toma por nombre una cadena de espacios', () => {
    expect(nombreDeUsuario(usuario({ nombre: '   ', email: 'ada@x.cl' }))).toBe('ada@x.cl');
  });
});

describe('totalDePaginas', () => {
  it('redondea hacia arriba', () => {
    expect(totalDePaginas(41, 20)).toBe(3);
    expect(totalDePaginas(40, 20)).toBe(2);
  });

  it('devuelve una página cuando no hay nada o el tamaño es inválido', () => {
    expect(totalDePaginas(0, 20)).toBe(1);
    expect(totalDePaginas(10, 0)).toBe(1);
  });
});

describe('rangoMostrado', () => {
  it('describe la página del medio', () => {
    expect(rangoMostrado(2, 20, 137)).toBe('21–40 de 137');
  });

  it('recorta la última página al total', () => {
    expect(rangoMostrado(7, 20, 137)).toBe('121–137 de 137');
  });

  it('no inventa un rango cuando no hay usuarios', () => {
    expect(rangoMostrado(1, 20, 0)).toBe('0 de 0');
  });
});

describe('categoriasAsignables', () => {
  it('quita las que ya cura', () => {
    const asignables = categoriasAsignables(
      [categoria(1, 'Aves'), categoria(2, 'Hongos'), categoria(3, 'Musgos')],
      [curada(2, 'Hongos')],
    );
    expect(asignables.map((c) => c.id)).toEqual([1, 3]);
  });

  it('devuelve todas cuando no cura nada', () => {
    expect(categoriasAsignables([categoria(1, 'Aves')], [])).toHaveLength(1);
  });
});

describe('insigniasOtorgablesAMano', () => {
  // Una insignia automática vive de su métrica y su umbral: dársela a dedo la
  // deja contradiciendo su propio criterio hasta el siguiente recálculo.
  it('deja fuera las automáticas', () => {
    const otorgables = insigniasOtorgablesAMano([
      insignia('curador', 'rol'),
      insignia('diez_fichas', 'automatica'),
    ]);
    expect(otorgables.map((i) => i.codigo)).toEqual(['curador']);
  });
});

describe('insigniasDisponibles', () => {
  it('quita las que ya tiene y las automáticas', () => {
    const disponibles = insigniasDisponibles(
      [insignia('curador', 'rol'), insignia('experto', 'rol'), insignia('diez', 'automatica')],
      [otorgada('curador', 'rol')],
    );
    expect(disponibles.map((i) => i.codigo)).toEqual(['experto']);
  });
});

describe('insigniasPorUsuario', () => {
  it('convierte las claves de texto a números', () => {
    const mapa = insigniasPorUsuario({ '7': [otorgada('curador', 'rol')] });
    expect(mapa.get(7)?.[0]?.codigo).toBe('curador');
  });

  it('ignora claves que no son enteros', () => {
    expect(insigniasPorUsuario({ total: [] }).size).toBe(0);
  });
});
