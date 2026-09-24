package services

import "testing"

func TestNormalizarPaginacion(t *testing.T) {
	casos := []struct {
		nombre            string
		pagina, porPagina int
		queremosPagina    int
		queremosPorPagina int
	}{
		{"valores usables pasan tal cual", 3, 50, 3, 50},
		{"página cero o negativa arranca en la primera", 0, 10, 1, 10},
		{"página negativa arranca en la primera", -7, 10, 1, 10},
		{"sin por_pagina se usa el default", 1, 0, 1, usuariosPorPaginaPorDefecto},
		{"por_pagina negativo se usa el default", 1, -5, 1, usuariosPorPaginaPorDefecto},
		{"por_pagina sin techo se acota", 1, 100000, 1, usuariosPorPaginaMaximo},
	}

	for _, caso := range casos {
		t.Run(caso.nombre, func(t *testing.T) {
			pagina, porPagina := normalizarPaginacion(caso.pagina, caso.porPagina)
			if pagina != caso.queremosPagina || porPagina != caso.queremosPorPagina {
				t.Fatalf("normalizarPaginacion(%d, %d) = (%d, %d); queríamos (%d, %d)",
					caso.pagina, caso.porPagina, pagina, porPagina,
					caso.queremosPagina, caso.queremosPorPagina)
			}
		})
	}
}

func TestNormalizarBusqueda(t *testing.T) {
	casos := []struct {
		nombre  string
		entrada string
		salida  string
	}{
		{"recorta los espacios", "  ana  ", "ana"},
		{"deja el texto normal intacto", "ana@correo.cl", "ana@correo.cl"},
		// Sin escapar, un '%' haría que la búsqueda calce con toda la tabla.
		{"escapa el comodín de porcentaje", "%", `\%`},
		{"escapa el comodín de guion bajo", "a_b", `a\_b`},
		{"escapa la barra invertida antes que los comodines", `a\%b`, `a\\\%b`},
	}

	for _, caso := range casos {
		t.Run(caso.nombre, func(t *testing.T) {
			if got := normalizarBusqueda(caso.entrada); got != caso.salida {
				t.Fatalf("normalizarBusqueda(%q) = %q; queríamos %q", caso.entrada, got, caso.salida)
			}
		})
	}
}

func TestNormalizarBusquedaAcotaElLargo(t *testing.T) {
	larga := make([]byte, largoMaximoDeBusqueda+50)
	for i := range larga {
		larga[i] = 'a'
	}
	if got := normalizarBusqueda(string(larga)); len(got) != largoMaximoDeBusqueda {
		t.Fatalf("largo = %d; queríamos %d", len(got), largoMaximoDeBusqueda)
	}
}

func TestRolValido(t *testing.T) {
	for _, rol := range []string{"admin", "moderator", "researcher", "user"} {
		if !rolValido(rol) {
			t.Fatalf("rolValido(%q) = false; es un rol del modelo", rol)
		}
	}
	for _, rol := range []string{"", "Admin", "curador", "'; DROP TABLE users; --"} {
		if rolValido(rol) {
			t.Fatalf("rolValido(%q) = true; no es un rol del modelo", rol)
		}
	}
}
