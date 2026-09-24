package handlers

import (
	"crypto/rand"
	"encoding/hex"
	"errors"
	"net/http"
	"strconv"
	"strings"

	"auth-service/internal/middleware"
	"auth-service/internal/models"
	"auth-service/internal/services"

	"github.com/gin-gonic/gin"
)

type AuthHandler struct {
	authService  *services.AuthService
	oauthService *services.OAuthService
}

// WhoAmI devuelve información básica del usuario actual basado en el token
func (h *AuthHandler) WhoAmI(c *gin.Context) {
	// Obtener token del header Authorization
	authHeader := c.GetHeader("Authorization")
	if authHeader == "" {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Missing authorization header",
			"code":    401,
		})
		return
	}

	// Verificar formato Bearer
	tokenParts := strings.SplitN(authHeader, " ", 2)
	if len(tokenParts) != 2 || tokenParts[0] != "Bearer" {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Invalid authorization header format",
			"code":    401,
		})
		return
	}

	// Verificar token y obtener usuario
	user, err := h.authService.VerifyToken(tokenParts[1])
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Invalid or expired token",
			"code":    401,
		})
		return
	}

	// Devolver información pública del usuario
	c.JSON(http.StatusOK, gin.H{
		"user":          user.ToPublic(),
		"authenticated": true,
		"token_valid":   true,
	})
}

func NewAuthHandler(authService *services.AuthService, oauthService *services.OAuthService) *AuthHandler {
	return &AuthHandler{
		authService:  authService,
		oauthService: oauthService,
	}
}

// Register maneja el registro de nuevos usuarios
func (h *AuthHandler) Register(c *gin.Context) {
	var req models.RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	response, err := h.authService.Register(&req)
	if err != nil {
		if errors.Is(err, services.ErrUserExists) {
			c.JSON(http.StatusConflict, gin.H{
				"error":   "Conflict",
				"message": "User already exists with this email",
				"code":    409,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to register user",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusCreated, response)
}

// Login maneja la autenticación de usuarios
func (h *AuthHandler) Login(c *gin.Context) {
	var req models.LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	response, err := h.authService.Login(&req)
	if err != nil {
		// ErrUserNotActive comparte respuesta con las credenciales inválidas a
		// propósito: distinguirlas revelaría qué cuentas existen.
		if errors.Is(err, services.ErrUserNotFound) ||
			errors.Is(err, services.ErrInvalidPassword) ||
			errors.Is(err, services.ErrUserNotActive) {
			c.JSON(http.StatusUnauthorized, gin.H{
				"error":   "Unauthorized",
				"message": "Invalid email or password",
				"code":    401,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to authenticate user",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, response)
}

// RefreshToken maneja la renovación de tokens
func (h *AuthHandler) RefreshToken(c *gin.Context) {
	var req struct {
		RefreshToken string `json:"refresh_token" binding:"required"`
	}

	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	response, err := h.authService.RefreshToken(req.RefreshToken)
	if err != nil {
		if errors.Is(err, services.ErrInvalidToken) || errors.Is(err, services.ErrUserNotActive) {
			c.JSON(http.StatusUnauthorized, gin.H{
				"error":   "Unauthorized",
				"message": "Invalid or expired refresh token",
				"code":    401,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to refresh token",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, response)
}

// VerifyToken verifica la validez de un access token (usado por nginx)
func (h *AuthHandler) VerifyToken(c *gin.Context) {
	// Obtener token del header Authorization
	authHeader := c.GetHeader("Authorization")
	if authHeader == "" {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Missing authorization header",
			"code":    401,
		})
		return
	}

	// Verificar formato Bearer
	tokenParts := strings.SplitN(authHeader, " ", 2)
	if len(tokenParts) != 2 || tokenParts[0] != "Bearer" {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Invalid authorization header format",
			"code":    401,
		})
		return
	}

	// Verificar token
	user, err := h.authService.VerifyToken(tokenParts[1])
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "Invalid or expired token",
			"code":    401,
		})
		return
	}

	// Headers para nginx (auth_request los reenvía a especies-api vía auth_request_set)
	c.Header("X-User-ID", strconv.Itoa(int(user.ID)))
	c.Header("X-User", user.Email)
	c.Header("X-User-Role", string(user.Role))

	c.JSON(http.StatusOK, gin.H{
		"valid":  true,
		"user":   user.ToPublic(),
		"status": "authenticated",
	})
}

// Logout invalida el token actual
func (h *AuthHandler) Logout(c *gin.Context) {
	// Obtener token del header
	authHeader := c.GetHeader("Authorization")
	if authHeader != "" {
		tokenParts := strings.SplitN(authHeader, " ", 2)
		if len(tokenParts) == 2 && tokenParts[0] == "Bearer" {
			h.authService.Logout(tokenParts[1])
		}
	}

	c.JSON(http.StatusOK, gin.H{
		"message": "Successfully logged out",
		"status":  "success",
	})
}

// GetProfile obtiene el perfil del usuario actual
func (h *AuthHandler) GetProfile(c *gin.Context) {
	user, exists := middleware.GetCurrentUser(c)
	if !exists {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "User not found in context",
			"code":    401,
		})
		return
	}

	profile := user.ToPublic()
	c.JSON(http.StatusOK, profile)
}

// UpdateProfile actualiza el perfil del usuario actual
func (h *AuthHandler) UpdateProfile(c *gin.Context) {
	userID, exists := middleware.GetCurrentUserID(c)
	if !exists {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "User not found in context",
			"code":    401,
		})
		return
	}

	var req models.UpdateUserRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	profile, err := h.authService.UpdateUserProfile(userID, &req)
	if err != nil {
		if errors.Is(err, services.ErrUserNotFound) {
			c.JSON(http.StatusNotFound, gin.H{
				"error":   "Not Found",
				"message": "User not found",
				"code":    404,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to update profile",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, profile)
}

// GetPerfilPublico devuelve la vista pública de otra persona.
func (h *AuthHandler) GetPerfilPublico(c *gin.Context) {
	id, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid user id",
			"code":    400,
		})
		return
	}

	perfil, err := h.authService.GetPerfilPublico(uint(id))
	if err != nil {
		if errors.Is(err, services.ErrUserNotFound) {
			c.JSON(http.StatusNotFound, gin.H{
				"error":   "Not Found",
				"message": "Perfil no disponible",
				"code":    404,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to load profile",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, perfil)
}

// ChangePassword cambia la contraseña del usuario actual
func (h *AuthHandler) ChangePassword(c *gin.Context) {
	userID, exists := middleware.GetCurrentUserID(c)
	if !exists {
		c.JSON(http.StatusUnauthorized, gin.H{
			"error":   "Unauthorized",
			"message": "User not found in context",
			"code":    401,
		})
		return
	}

	var req models.ChangePasswordRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	err := h.authService.ChangePassword(userID, &req)
	if err != nil {
		if errors.Is(err, services.ErrUserNotFound) {
			c.JSON(http.StatusNotFound, gin.H{
				"error":   "Not Found",
				"message": "User not found",
				"code":    404,
			})
			return
		}

		if errors.Is(err, services.ErrInvalidPassword) {
			c.JSON(http.StatusBadRequest, gin.H{
				"error":   "Bad Request",
				"message": "Current password is incorrect",
				"code":    400,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to change password",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"message": "Password changed successfully",
		"status":  "success",
	})
}

// GoogleAuth inicia el flujo de autenticación con Google
func (h *AuthHandler) GoogleAuth(c *gin.Context) {
	// Generar state para CSRF protection
	state := h.generateState()
	// En una implementación real, guardarías el state en cache/sesión

	authURL := h.oauthService.GetGoogleAuthURL(state)
	if authURL == "" {
		c.JSON(http.StatusServiceUnavailable, gin.H{
			"error":   "Service Unavailable",
			"message": "Google OAuth is not configured",
			"code":    503,
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"auth_url": authURL,
		"state":    state,
	})
}

func (h *AuthHandler) GoogleIDTokenLogin(c *gin.Context) {
	var req struct {
		IDToken    string `json:"id_token"`
		Credential string `json:"credential"`
	}

	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Invalid request data",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	idToken := strings.TrimSpace(req.IDToken)
	if idToken == "" {
		idToken = strings.TrimSpace(req.Credential)
	}
	if idToken == "" {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "id_token is required",
			"code":    400,
		})
		return
	}

	tokenInfo, err := h.oauthService.VerifyGoogleIDToken(c.Request.Context(), idToken)
	if err != nil {
		status := http.StatusUnauthorized
		message := "Invalid Google ID token"
		if errors.Is(err, services.ErrOAuthNotConfigured) {
			status = http.StatusServiceUnavailable
			message = "Google OAuth is not configured"
		} else if errors.Is(err, services.ErrUnverifiedEmail) {
			message = "Google email is not verified"
		}
		c.JSON(status, gin.H{
			"error":   http.StatusText(status),
			"message": message,
			"code":    status,
		})
		return
	}

	response, err := h.authService.ProcessGoogleIDTokenUser(tokenInfo)
	if err != nil {
		// Aquí no hay riesgo de enumeración: Google ya verificó que quien llama
		// es dueño de esa cuenta, así que decirle que está deshabilitada es más
		// útil que un error genérico.
		if errors.Is(err, services.ErrUserNotActive) {
			c.JSON(http.StatusForbidden, gin.H{
				"error":   "Forbidden",
				"message": "User account is not active",
				"code":    403,
			})
			return
		}

		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to process Google user",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, response)
}

// GitHubAuth inicia el flujo de autenticación con GitHub
func (h *AuthHandler) GitHubAuth(c *gin.Context) {
	// Generar state para CSRF protection
	state := h.generateState()
	// En una implementación real, guardarías el state en cache/sesión

	authURL := h.oauthService.GetGitHubAuthURL(state)
	if authURL == "" {
		c.JSON(http.StatusServiceUnavailable, gin.H{
			"error":   "Service Unavailable",
			"message": "GitHub OAuth is not configured",
			"code":    503,
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"auth_url": authURL,
		"state":    state,
	})
}

// GoogleCallback maneja el callback de Google OAuth
func (h *AuthHandler) GoogleCallback(c *gin.Context) {
	code := c.Query("code")
	state := c.Query("state")

	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Authorization code is required",
			"code":    400,
			"state":   state,
		})
		return
	}

	// En una implementación real, verificarías el state aquí

	// Intercambiar código por información del usuario
	userInfo, err := h.oauthService.ExchangeGoogleCode(code)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Failed to exchange authorization code",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	// Procesar usuario OAuth
	response, err := h.processOAuthUser("google", userInfo.ID, userInfo.Email, userInfo.Name, userInfo.Picture)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to process OAuth user",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, response)
}

// GitHubCallback maneja el callback de GitHub OAuth
func (h *AuthHandler) GitHubCallback(c *gin.Context) {
	code := c.Query("code")
	state := c.Query("state")

	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Authorization code is required",
			"state":   state,
			"code":    400,
		})
		return
	}

	// En una implementación real, verificarías el state aquí

	// Intercambiar código por información del usuario
	userInfo, err := h.oauthService.ExchangeGitHubCode(code)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "Failed to exchange authorization code",
			"details": err.Error(),
			"code":    400,
		})
		return
	}

	// Procesar usuario OAuth
	response, err := h.processOAuthUser("github", strconv.Itoa(userInfo.ID), userInfo.Email, userInfo.Name, userInfo.AvatarURL)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "Failed to process OAuth user",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, response)
}

// processOAuthUser procesa un usuario de OAuth (crear o actualizar)
func (h *AuthHandler) processOAuthUser(provider, providerID, email, name, avatar string) (*models.AuthResponse, error) {
	return h.authService.ProcessOAuthUser(provider, providerID, email, name, avatar)
}

// generateState genera un state aleatorio para OAuth
func (h *AuthHandler) generateState() string {
	bytes := make([]byte, 32)
	rand.Read(bytes)
	return hex.EncodeToString(bytes)
}

// Métodos de administración

// ListUsers lista usuarios para la pantalla de usuarios del panel de
// curaduría, con paginación, búsqueda por nombre o email y filtro por rol.
func (h *AuthHandler) ListUsers(c *gin.Context) {
	pagina, err := strconv.Atoi(c.DefaultQuery("pagina", "1"))
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "El parámetro 'pagina' debe ser un número",
			"code":    400,
		})
		return
	}

	porPagina, err := strconv.Atoi(c.DefaultQuery("por_pagina", "0"))
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "El parámetro 'por_pagina' debe ser un número",
			"code":    400,
		})
		return
	}

	resultado, err := h.authService.ListUsers(services.ListUsersFilter{
		Pagina:    pagina,
		PorPagina: porPagina,
		Buscar:    c.Query("buscar"),
		Rol:       c.Query("rol"),
	})
	if err != nil {
		if errors.Is(err, services.ErrInvalidRole) {
			c.JSON(http.StatusBadRequest, gin.H{
				"error":   "Bad Request",
				"message": "Rol desconocido",
				"code":    400,
			})
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "No se pudo listar los usuarios",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, resultado)
}

func (h *AuthHandler) GetUser(c *gin.Context) {
	userID, err := strconv.ParseUint(c.Param("id"), 10, 32)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error":   "Bad Request",
			"message": "El id del usuario debe ser un número",
			"code":    400,
		})
		return
	}

	usuario, err := h.authService.GetUser(uint(userID))
	if err != nil {
		if errors.Is(err, services.ErrUserNotFound) {
			c.JSON(http.StatusNotFound, gin.H{
				"error":   "Not Found",
				"message": "Usuario no encontrado",
				"code":    404,
			})
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{
			"error":   "Internal Server Error",
			"message": "No se pudo obtener el usuario",
			"code":    500,
		})
		return
	}

	c.JSON(http.StatusOK, usuario)
}

func (h *AuthHandler) UpdateUser(c *gin.Context) {
	// TODO: Implementar actualización de usuario por admin
	c.JSON(http.StatusNotImplemented, gin.H{
		"error":   "Not Implemented",
		"message": "Feature not implemented yet",
		"code":    501,
	})
}

func (h *AuthHandler) DeleteUser(c *gin.Context) {
	// TODO: Implementar eliminación de usuario
	c.JSON(http.StatusNotImplemented, gin.H{
		"error":   "Not Implemented",
		"message": "Feature not implemented yet",
		"code":    501,
	})
}

func (h *AuthHandler) ActivateUser(c *gin.Context) {
	// TODO: Implementar activación de usuario
	c.JSON(http.StatusNotImplemented, gin.H{
		"error":   "Not Implemented",
		"message": "Feature not implemented yet",
		"code":    501,
	})
}

func (h *AuthHandler) DeactivateUser(c *gin.Context) {
	// TODO: Implementar desactivación de usuario
	c.JSON(http.StatusNotImplemented, gin.H{
		"error":   "Not Implemented",
		"message": "Feature not implemented yet",
		"code":    501,
	})
}
