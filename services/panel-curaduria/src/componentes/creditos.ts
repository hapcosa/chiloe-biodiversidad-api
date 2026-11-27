import type { CreditoFoto } from '../api/tipos';

// Las únicas licencias que se aceptan (ADR #28). iNaturalist usa la versión 4.0.
const LICENCIAS: Record<string, { nombre: string; url: string }> = {
  cc0: { nombre: 'CC0 1.0', url: 'https://creativecommons.org/publicdomain/zero/1.0/' },
  'cc-by': { nombre: 'CC BY 4.0', url: 'https://creativecommons.org/licenses/by/4.0/' },
  'cc-by-sa': { nombre: 'CC BY-SA 4.0', url: 'https://creativecommons.org/licenses/by-sa/4.0/' },
};

// Una licencia desconocida se muestra tal cual y sin enlace: mejor que
// atribuirle una que nadie verificó.
export function licenciaDe(codigo: string): { nombre: string; url: string | null } {
  return LICENCIAS[codigo] ?? { nombre: codigo, url: null };
}

export function enlaceSeguro(url: string | null | undefined): string | null {
  return url && url.startsWith('https://') ? url : null;
}

export function autorDe(credito: CreditoFoto): string {
  return credito.autor || 'Autor no indicado';
}
