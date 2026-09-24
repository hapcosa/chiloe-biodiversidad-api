package services

import (
	"fmt"
	"strings"

	"auth-service/internal/models"
)

const (
	usuariosPorPaginaPorDefecto = 25
	usuariosPorPaginaMaximo     = 100
	largoMaximoDeBusqueda       = 120
)

// normalizarPaginacion acota lo que llegue por query string. Un `per_page`
// sin techo deja que una sola petición se traiga la tabla entera.
func normalizarPaginacion(pagina, porPagina int) (int, int) {
	if pagina < 1 {
		pagina = 1
	}
	if porPagina < 1 {
		porPagina = usuariosPorPaginaPorDefecto
	}
	if porPagina > usuariosPorPaginaMaximo {
		porPagina = usuariosPorPaginaMaximo
	}
	return pagina, porPagina
}

// normalizarBusqueda deja el texto listo para un LIKE. Escapa los comodines
// de SQL: un `%` escrito por quien busca haría que la búsqueda calce con todo
// en vez de con lo que se pidió.
func normalizarBusqueda(texto string) string {
	limpio := strings.TrimSpace(texto)
	if len(limpio) > largoMaximoDeBusqueda {
		limpio = limpio[:largoMaximoDeBusqueda]
	}
	reemplazos := strings.NewReplacer(`\`, `\\`, `%`, `\%`, `_`, `\_`)
	return reemplazos.Replace(limpio)
}

// rolValido descarta un filtro de rol inventado antes de llegar a la consulta.
func rolValido(rol string) bool {
	switch models.UserRole(rol) {
	case models.UserRoleAdmin, models.UserRoleModerator, models.UserRoleResearcher, models.UserRoleUser:
		return true
	default:
		return false
	}
}

// ListUsersFilter son los filtros de la pantalla de usuarios del panel.
type ListUsersFilter struct {
	Pagina    int
	PorPagina int
	Buscar    string
	Rol       string
}

// UsersPage es una página del listado, con lo necesario para paginar en el
// panel sin pedir el total aparte.
type UsersPage struct {
	Usuarios  []models.UserPublic `json:"usuarios"`
	Total     int64               `json:"total"`
	Pagina    int                 `json:"pagina"`
	PorPagina int                 `json:"por_pagina"`
}

// ListUsers devuelve una página de usuarios para el panel de curaduría. El
// orden es por id ascendente: estable, y hace que paginar no repita ni saltee
// filas cuando alguien se registra a mitad del recorrido.
func (s *AuthService) ListUsers(filtro ListUsersFilter) (*UsersPage, error) {
	pagina, porPagina := normalizarPaginacion(filtro.Pagina, filtro.PorPagina)

	consulta := s.db.Model(&models.User{})

	if buscar := normalizarBusqueda(filtro.Buscar); buscar != "" {
		patron := "%" + strings.ToLower(buscar) + "%"
		consulta = consulta.Where(
			`(LOWER(name) LIKE ? ESCAPE '\' OR LOWER(email) LIKE ? ESCAPE '\')`,
			patron, patron,
		)
	}

	if filtro.Rol != "" {
		if !rolValido(filtro.Rol) {
			return nil, fmt.Errorf("%w: %s", ErrInvalidRole, filtro.Rol)
		}
		consulta = consulta.Where("role = ?", filtro.Rol)
	}

	var total int64
	if err := consulta.Count(&total).Error; err != nil {
		return nil, fmt.Errorf("database error: %w", err)
	}

	var usuarios []models.User
	if err := consulta.
		Order("id ASC").
		Limit(porPagina).
		Offset((pagina - 1) * porPagina).
		Find(&usuarios).Error; err != nil {
		return nil, fmt.Errorf("database error: %w", err)
	}

	publicos := make([]models.UserPublic, 0, len(usuarios))
	for indice := range usuarios {
		publicos = append(publicos, usuarios[indice].ToPublic())
	}

	return &UsersPage{
		Usuarios:  publicos,
		Total:     total,
		Pagina:    pagina,
		PorPagina: porPagina,
	}, nil
}

// GetUser devuelve un usuario por id para el panel. A diferencia del perfil
// público, acá sí van el email y el estado de la cuenta: quien mira es admin.
func (s *AuthService) GetUser(userID uint) (*models.UserPublic, error) {
	return s.GetUserProfile(userID)
}
