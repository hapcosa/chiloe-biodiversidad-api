import { describe, expect, it } from 'vitest';
import { autorDe, enlaceSeguro, licenciaDe } from './creditos';

describe('créditos de fotos', () => {
  it('nombra y enlaza las licencias aceptadas', () => {
    expect(licenciaDe('cc-by')).toEqual({
      nombre: 'CC BY 4.0',
      url: 'https://creativecommons.org/licenses/by/4.0/',
    });
    expect(licenciaDe('cc-by-sa').nombre).toBe('CC BY-SA 4.0');
    expect(licenciaDe('cc0').nombre).toBe('CC0 1.0');
  });

  it('no inventa un enlace para una licencia desconocida', () => {
    expect(licenciaDe('otra')).toEqual({ nombre: 'otra', url: null });
  });

  it('solo enlaza URLs https', () => {
    expect(enlaceSeguro('https://www.inaturalist.org/photos/1')).not.toBeNull();
    expect(enlaceSeguro('javascript:alert(1)')).toBeNull();
    expect(enlaceSeguro(undefined)).toBeNull();
  });

  it('avisa cuando no hay autor', () => {
    expect(autorDe({ autor: null, licencia: 'cc0' })).toBe('Autor no indicado');
    expect(autorDe({ autor: 'Ana', licencia: 'cc-by' })).toBe('Ana');
  });
});
