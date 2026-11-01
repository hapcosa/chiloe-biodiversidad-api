#!/usr/bin/env python3
"""Prepara la carga de fotos de iNaturalist con licencia abierta (ADR #28).

Recibe el reporte de `buscar_fotos_abiertas` (id, nombre, foto{url,licencia,
atribucion}) y deja en --salida todo lo necesario para cargarlas, SIN tocar
producción:

  fotos/<key>      el JPEG ya sin EXIF (jpegtran -copy none), con la key final
  carga.sql        un UPDATE por especie, con foto_portada_key/fotos_keys/fotos_creditos
  plan.json        qué se prepara y qué se descarta, y por qué

Subir los archivos a MinIO y aplicar carga.sql es un paso aparte y manual.

Uso:  python3 -I preparar_fotos_abiertas.py reporte.json --salida DIR_NUEVO
"""
import argparse
import datetime
import hashlib
import json
import os
import re
import secrets
import subprocess
import sys
import time
import urllib.request

LICENCIAS_ABIERTAS = {"cc0", "cc-by", "cc-by-sa"}
UA = {"User-Agent": "chiloe-biodiversidad/1.0 (curaduria)"}
MAX_BYTES = 8 * 1024 * 1024


def slug(nombre):
    return re.sub(r"[^a-z0-9]+", "-", nombre.lower()).strip("-")


def autor_de(atribucion):
    # "(c) Nombre, some rights reserved (CC BY), uploaded by Nombre"
    m = re.match(r"\(c\)\s*(.+?),\s*(?:some|no) rights reserved", atribucion or "")
    if m:
        return m.group(1).strip()
    m = re.search(r"uploaded by (.+)$", atribucion or "")
    return m.group(1).strip() if m else None


def sql_str(valor):
    return "'" + valor.replace("'", "''") + "'"


def descargar(url):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        datos = r.read(MAX_BYTES + 1)
    if len(datos) > MAX_BYTES:
        raise ValueError("más de 8 MiB")
    return datos


def sin_exif(datos):
    # -copy none descarta EXIF/XMP/ICC sin recomprimir. Falla si no es un JPEG.
    p = subprocess.run(["jpegtran", "-copy", "none"], input=datos,
                       capture_output=True, check=True)
    return p.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reporte")
    ap.add_argument("--salida", required=True)
    ap.add_argument("--incluir-con-foto", action="store_true",
                    help="también las especies que ya tienen foto de portada")
    args = ap.parse_args()

    if os.path.exists(args.salida) and os.listdir(args.salida):
        sys.exit(f"--salida debe ser un directorio nuevo o vacío: {args.salida}")
    os.makedirs(os.path.join(args.salida, "fotos"), exist_ok=True)

    hoy = datetime.datetime.now(datetime.timezone.utc).strftime("%Y/%m/%d")
    plan, sql = [], ["BEGIN;"]
    for fila in json.load(open(args.reporte)):
        foto = fila.get("foto")
        base = {"id": fila["id"], "nombre": fila["nombre"]}
        if not foto:
            plan.append({**base, "estado": "omitida", "motivo": fila.get("nota") or "sin foto"})
            continue
        if fila["tiene_foto"] and not args.incluir_con_foto:
            plan.append({**base, "estado": "omitida", "motivo": "ya tiene foto"})
            continue
        if foto["licencia"] not in LICENCIAS_ABIERTAS:
            plan.append({**base, "estado": "omitida", "motivo": f"licencia {foto['licencia']}"})
            continue
        try:
            jpeg = sin_exif(descargar(foto["url"]))
            time.sleep(0.5)
        except Exception as e:  # una foto rota no debe parar el resto
            plan.append({**base, "estado": "omitida", "motivo": f"descarga/jpeg: {e}"})
            continue

        # Mismo formato que upload_presign_service: especies/AAAA/MM/DD/<hex32>-<nombre>
        key = f"especies/{hoy}/{secrets.token_hex(16)}-{slug(fila['nombre'])}.jpg"
        ruta = os.path.join(args.salida, "fotos", key)
        os.makedirs(os.path.dirname(ruta), exist_ok=True)
        open(ruta, "wb").write(jpeg)

        foto_id = re.search(r"/photos/(\d+)/", foto["url"]).group(1)
        credito = {
            "autor": autor_de(foto["atribucion"]),
            "licencia": foto["licencia"],
            "url_origen": f"https://www.inaturalist.org/photos/{foto_id}",
            "fuente": "iNaturalist",
        }
        k = sql_str(key)
        creditos = "jsonb_build_object({}, {}::jsonb)".format(
            k, sql_str(json.dumps(credito, ensure_ascii=False)))
        # Los guards hacen que re-aplicar el SQL no pise una portada ya puesta.
        sql.append(
            f"UPDATE especies SET foto_portada_key = {k}, "
            f"fotos_keys = jsonb_build_array({k}), fotos_creditos = {creditos} "
            f"WHERE id = {fila['id']} AND foto_portada_key IS NULL "
            "AND fotos_keys = '[]'::jsonb;")
        plan.append({**base, "estado": "lista", "key": key, "bytes": len(jpeg),
                     "sha256": hashlib.sha256(jpeg).hexdigest(), "credito": credito})

    sql.append("COMMIT;")
    open(os.path.join(args.salida, "carga.sql"), "w").write("\n".join(sql) + "\n")
    json.dump(plan, open(os.path.join(args.salida, "plan.json"), "w"),
              ensure_ascii=False, indent=1)
    listas = sum(1 for p in plan if p["estado"] == "lista")
    print(f"listas: {listas} | omitidas: {len(plan) - listas}")


if __name__ == "__main__":
    main()
