import type { CreditoFoto } from '../api/tipos';
import { autorDe, enlaceSeguro, licenciaDe } from './creditos';

export function CreditoDeFoto({ credito }: { credito: CreditoFoto }) {
  const licencia = licenciaDe(credito.licencia);
  const origen = enlaceSeguro(credito.url_origen);
  const fuente = credito.fuente || 'Origen';

  return (
    <p className="credito">
      {autorDe(credito)} ·{' '}
      {licencia.url ? (
        <a href={licencia.url} target="_blank" rel="noreferrer">
          {licencia.nombre}
        </a>
      ) : (
        licencia.nombre
      )}
      {origen && (
        <>
          {' · '}
          <a href={origen} target="_blank" rel="noreferrer">
            {fuente}
          </a>
        </>
      )}
    </p>
  );
}
