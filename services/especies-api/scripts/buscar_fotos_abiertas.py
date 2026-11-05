#!/usr/bin/env python3
"""Busca en iNaturalist una foto con licencia abierta por especie (ADR #28).

Entrada: líneas `id|nombre_cientifico|tiene_foto(t/f)` (psql -AtF'|'). Salida: el
reporte JSON que consume `preparar_fotos_abiertas.py`. Solo lectura: no descarga
fotos ni toca la BD.

Una especie solo se busca si el taxón de iNaturalist coincide EXACTO con el
nombre; los sinónimos quedan con una nota y los resuelve curaduría, porque un
fallback automático puede dar otra especie (guigna -> geoffroyi).
Se prefiere una observación de grado investigación en Chile (place_id 7182); si no
hay, cae a cualquier país y lo anota en `nota`.

Uso:  python3 -I buscar_fotos_abiertas.py especies.txt reporte.json
"""
import json
import sys
import time
import urllib.parse
import urllib.request

OK = {"cc0", "cc-by", "cc-by-sa"}  # sin NC/ND: la divulgación puede acabar en uso comercial
UA = {"User-Agent": "chiloe-biodiversidad/1.0 (curaduria)"}
def get(u):
    with urllib.request.urlopen(urllib.request.Request(u, headers=UA), timeout=30) as r:
        return json.load(r)
out = []
for line in open(sys.argv[1]):
    id_, nombre, tiene = line.strip().split("|")
    row = {"id": int(id_), "nombre": nombre, "tiene_foto": tiene == "t", "foto": None, "nota": ""}
    try:
        t = get("https://api.inaturalist.org/v1/taxa?rank=species,subspecies&per_page=1&q=" + urllib.parse.quote(nombre))["results"]
        if not t or t[0]["name"].lower() != nombre.lower():
            row["nota"] = "taxón no coincide/no encontrado" + (f" ({t[0]['name']})" if t else "")
        else:
            tid = t[0]["id"]
            ps = get(f"https://api.inaturalist.org/v1/observations?taxon_id={tid}&photo_license=cc0,cc-by,cc-by-sa&quality_grade=research&place_id=7182&order_by=votes&per_page=1&photos=true")
            if not ps["results"]:  # fallback: sin acotar a Chile
                ps = get(f"https://api.inaturalist.org/v1/observations?taxon_id={tid}&photo_license=cc0,cc-by,cc-by-sa&quality_grade=research&order_by=votes&per_page=1&photos=true")
                row["nota"] = "fuera de Chile"
            if ps["results"]:
                o = ps["results"][0]; p = o["photos"][0]
                if p["license_code"] in OK:
                    row["foto"] = {"obs": o["id"], "licencia": p["license_code"], "atribucion": p["attribution"], "url": p["url"].replace("square", "large")}
            if not row["foto"]: row["nota"] = "sin foto abierta"
    except Exception as e:
        row["nota"] = f"error: {e}"
    out.append(row); time.sleep(1.1)  # respeta ~60 req/min de iNaturalist
json.dump(out, open(sys.argv[2], "w"), ensure_ascii=False, indent=1)
