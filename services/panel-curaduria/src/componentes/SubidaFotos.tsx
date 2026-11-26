import { useState } from 'react';
import { subirArchivo } from '../api/api';
import type { CreditoFoto } from '../api/tipos';
import { useSesion } from '../auth/sesion';
import { Aviso } from './Aviso';
import { CreditoDeFoto } from './CreditoDeFoto';

const BUCKET = 'especies-fotos';

// El bucket de fotos de especies queda con descarga pública, así que basta con
// componer la URL. La base se fija en tiempo de build (VITE_S3_PUBLIC_BASE).
export function urlPublica(key: string): string {
  const base = import.meta.env.VITE_S3_PUBLIC_BASE ?? '';
  return `${base.replace(/\/$/, '')}/${BUCKET}/${key}`;
}

interface Props {
  fotosKeys: string[];
  portada: string | null;
  creditos?: Record<string, CreditoFoto>;
  onChange: (fotos: { fotos_keys: string[]; foto_portada_key: string | null }) => void;
}

export function SubidaFotos({ fotosKeys, portada, creditos, onChange }: Props) {
  const { api } = useSesion();
  const [subiendo, setSubiendo] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function subir(archivos: FileList | null) {
    if (!archivos || archivos.length === 0) return;
    setError(null);
    setSubiendo(true);
    try {
      const nuevasKeys: string[] = [];
      for (const archivo of Array.from(archivos)) {
        // Presigned URL + PUT directo al object storage. Nunca multipart contra
        // la API (CLAUDE.md § Fotos).
        const presign = await api.presign(BUCKET, archivo.name, archivo.type);
        await subirArchivo(presign, archivo);
        nuevasKeys.push(presign.key);
      }
      const keys = [...fotosKeys, ...nuevasKeys];
      onChange({ fotos_keys: keys, foto_portada_key: portada ?? keys[0] ?? null });
    } catch (fallo) {
      setError(fallo instanceof Error ? fallo.message : 'No se pudo subir la foto');
    } finally {
      setSubiendo(false);
    }
  }

  function quitar(key: string) {
    const keys = fotosKeys.filter((item) => item !== key);
    onChange({
      fotos_keys: keys,
      foto_portada_key: portada === key ? (keys[0] ?? null) : portada,
    });
  }

  return (
    <div className="fotos">
      <label>
        Fotos
        <input
          type="file"
          accept="image/jpeg,image/png,image/webp"
          multiple
          disabled={subiendo}
          onChange={(evento) => {
            void subir(evento.target.files);
            evento.target.value = '';
          }}
        />
      </label>
      <p className="ayuda">
        Se suben directo al almacenamiento; la ficha solo guarda la clave. Quita el GPS de las
        fotos antes de subirlas si no quieres publicar la ubicación exacta.
      </p>

      {subiendo && <p>Subiendo…</p>}
      {error && <Aviso tono="error">{error}</Aviso>}

      <ul className="galeria">
        {fotosKeys.map((key) => (
          <li key={key} className={key === portada ? 'es-portada' : undefined}>
            <img src={urlPublica(key)} alt="" loading="lazy" />
            {creditos?.[key] && <CreditoDeFoto credito={creditos[key]} />}
            <div>
              <button
                type="button"
                disabled={key === portada}
                onClick={() => onChange({ fotos_keys: fotosKeys, foto_portada_key: key })}
              >
                {key === portada ? 'Portada' : 'Usar de portada'}
              </button>
              <button type="button" onClick={() => quitar(key)}>
                Quitar
              </button>
            </div>
          </li>
        ))}
      </ul>
    </div>
  );
}
