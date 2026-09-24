package services

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"errors"
	"fmt"
	"strings"
	"time"

	"auth-service/internal/config"
	"auth-service/internal/models"

	"github.com/golang-jwt/jwt/v5"
	"github.com/redis/go-redis/v9"
	"golang.org/x/crypto/bcrypt"
	"gorm.io/gorm"
)

type AuthService struct {
	db     *gorm.DB
	redis  *redis.Client
	config config.JWTConfig
}

type Claims struct {
	UserID uint            `json:"user_id"`
	Email  string          `json:"email"`
	Role   models.UserRole `json:"role"`
	jwt.RegisteredClaims
}

var (
	ErrUserNotFound       = errors.New("user not found")
	ErrInvalidPassword    = errors.New("invalid password")
	ErrInvalidToken       = errors.New("invalid token")
	ErrTokenExpired       = errors.New("token expired")
	ErrUserExists         = errors.New("user already exists")
	ErrInvalidEmail       = errors.New("invalid email")
	ErrWeakPassword       = errors.New("password too weak")
	ErrOAuthNotConfigured = errors.New("oauth provider not configured")
	ErrInvalidOAuthToken  = errors.New("invalid oauth token")
	ErrUnverifiedEmail    = errors.New("email is not verified")
	ErrUserNotActive      = errors.New("user account is not active")
	ErrInvalidRole        = errors.New("invalid role")
)

func NewAuthService(db *gorm.DB, redis *redis.Client, config config.JWTConfig) *AuthService {
	return &AuthService{
		db:     db,
		redis:  redis,
		config: config,
	}
}

// Register crea un nuevo usuario
func (s *AuthService) Register(req *models.RegisterRequest) (*models.AuthResponse, error) {
	// Verificar si el usuario ya existe
	var existingUser models.User
	if err := s.db.Where("email = ?", req.Email).First(&existingUser).Error; err == nil {
		return nil, ErrUserExists
	}

	// Hash de la contraseña
	hashedPassword, err := s.hashPassword(req.Password)
	if err != nil {
		return nil, fmt.Errorf("failed to hash password: %w", err)
	}

	// Crear nuevo usuario
	user := models.User{
		Email:    req.Email,
		Password: hashedPassword,
		Name:     req.Name,
		Role:     models.UserRoleUser,
		Status:   models.UserStatusActive,
		Provider: "local",
	}

	if err := s.db.Create(&user).Error; err != nil {
		return nil, fmt.Errorf("failed to create user: %w", err)
	}

	// Generar tokens
	return s.generateAuthResponse(&user)
}

// Login autentica un usuario
func (s *AuthService) Login(req *models.LoginRequest) (*models.AuthResponse, error) {
	// Buscar usuario por email
	var user models.User
	if err := s.db.Where("email = ? AND provider = ?", req.Email, "local").First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	// Verificar estado del usuario
	if user.Status != models.UserStatusActive {
		return nil, ErrUserNotActive
	}

	// Verificar contraseña
	if err := bcrypt.CompareHashAndPassword([]byte(user.Password), []byte(req.Password)); err != nil {
		return nil, ErrInvalidPassword
	}

	// Generar tokens
	return s.generateAuthResponse(&user)
}

// RefreshToken genera un nuevo access token usando el refresh token
func (s *AuthService) RefreshToken(refreshToken string) (*models.AuthResponse, error) {
	// Buscar refresh token en la base de datos
	var tokenRecord models.RefreshToken
	if err := s.db.Preload("User").Where("token = ? AND expires_at > ?",
		s.hashToken(refreshToken), time.Now()).First(&tokenRecord).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrInvalidToken
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	// Verificar estado del usuario
	if tokenRecord.User.Status != models.UserStatusActive {
		return nil, ErrUserNotActive
	}

	// Generar nuevo access token
	accessToken, err := s.generateAccessToken(&tokenRecord.User)
	if err != nil {
		return nil, fmt.Errorf("failed to generate access token: %w", err)
	}

	return &models.AuthResponse{
		User:         tokenRecord.User.ToPublic(),
		AccessToken:  accessToken,
		RefreshToken: refreshToken, // Mantener el mismo refresh token
		ExpiresIn:    int64(s.config.Expiry.Seconds()),
	}, nil
}

// VerifyToken verifica la validez de un access token y retorna el usuario
func (s *AuthService) VerifyToken(tokenString string) (*models.User, error) {
	// Verificar blacklist en Redis si está disponible
	if s.redis != nil {
		ctx := context.Background()
		if s.redis.Exists(ctx, fmt.Sprintf("blacklist:%s", tokenString)).Val() > 0 {
			return nil, ErrInvalidToken
		}
	}

	// Parsear y validar token
	token, err := jwt.ParseWithClaims(tokenString, &Claims{}, func(token *jwt.Token) (interface{}, error) {
		// Verificar algoritmo de firma
		if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("unexpected signing method: %v", token.Header["alg"])
		}
		return []byte(s.config.Secret), nil
	})

	if err != nil {
		return nil, ErrInvalidToken
	}

	claims, ok := token.Claims.(*Claims)
	if !ok || !token.Valid {
		return nil, ErrInvalidToken
	}

	// Verificar expiración
	if claims.ExpiresAt.Time.Before(time.Now()) {
		return nil, ErrTokenExpired
	}

	// Buscar usuario en la base de datos
	var user models.User
	if err := s.db.Where("id = ?", claims.UserID).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	// Verificar estado del usuario
	if user.Status != models.UserStatusActive {
		return nil, ErrUserNotActive
	}

	return &user, nil
}

// Logout invalida un token (lo agrega a blacklist)
func (s *AuthService) Logout(tokenString string) error {
	if s.redis == nil {
		return nil // Si no hay Redis, no podemos hacer blacklist
	}

	// Parsear token para obtener expiración
	token, err := jwt.ParseWithClaims(tokenString, &Claims{}, func(token *jwt.Token) (interface{}, error) {
		return []byte(s.config.Secret), nil
	})

	if err != nil {
		return nil // Token inválido, no necesita blacklist
	}

	claims, ok := token.Claims.(*Claims)
	if !ok {
		return nil
	}

	// Agregar a blacklist hasta su expiración
	ctx := context.Background()
	ttl := time.Until(claims.ExpiresAt.Time)
	if ttl > 0 {
		s.redis.Set(ctx, fmt.Sprintf("blacklist:%s", tokenString), "1", ttl)
	}

	return nil
}

// GetUserProfile obtiene el perfil de un usuario
func (s *AuthService) GetUserProfile(userID uint) (*models.UserPublic, error) {
	var user models.User
	if err := s.db.Where("id = ?", userID).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	profile := user.ToPublic()
	return &profile, nil
}

// GetPerfilPublico devuelve la vista que un tercero puede ver de una persona.
// Un perfil que no está publicado se responde igual que uno inexistente: que
// una cuenta exista tampoco es información de terceros.
func (s *AuthService) GetPerfilPublico(userID uint) (*models.UserPerfilPublico, error) {
	var user models.User
	if err := s.db.Where("id = ? AND perfil_publico = ? AND status = ?",
		userID, true, models.UserStatusActive).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	perfil := user.ToPerfilPublico()
	return &perfil, nil
}

// UpdateUserProfile actualiza el perfil de un usuario
func (s *AuthService) UpdateUserProfile(userID uint, req *models.UpdateUserRequest) (*models.UserPublic, error) {
	var user models.User
	if err := s.db.Where("id = ?", userID).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, fmt.Errorf("database error: %w", err)
	}

	// Actualizar campos
	if req.Name != "" {
		user.Name = req.Name
	}
	if req.Avatar != "" {
		user.Avatar = req.Avatar
	}
	// Punteros: nil es "no lo mandó", "" es "quiero borrarlo".
	if req.Bio != nil {
		user.Bio = strings.TrimSpace(*req.Bio)
	}
	if req.Profesion != nil {
		user.Profesion = strings.TrimSpace(*req.Profesion)
	}
	if req.PerfilPublico != nil {
		user.PerfilPublico = *req.PerfilPublico
	}

	if err := s.db.Save(&user).Error; err != nil {
		return nil, fmt.Errorf("failed to update user: %w", err)
	}

	profile := user.ToPublic()
	return &profile, nil
}

// ChangePassword cambia la contraseña de un usuario
func (s *AuthService) ChangePassword(userID uint, req *models.ChangePasswordRequest) error {
	var user models.User
	if err := s.db.Where("id = ?", userID).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return ErrUserNotFound
		}
		return fmt.Errorf("database error: %w", err)
	}

	// Verificar contraseña actual
	if err := bcrypt.CompareHashAndPassword([]byte(user.Password), []byte(req.CurrentPassword)); err != nil {
		return ErrInvalidPassword
	}

	// Hash nueva contraseña
	hashedPassword, err := s.hashPassword(req.NewPassword)
	if err != nil {
		return fmt.Errorf("failed to hash password: %w", err)
	}

	// Actualizar contraseña
	user.Password = hashedPassword
	if err := s.db.Save(&user).Error; err != nil {
		return fmt.Errorf("failed to update password: %w", err)
	}

	return nil
}

// generateAuthResponse genera tokens de autenticación
func (s *AuthService) generateAuthResponse(user *models.User) (*models.AuthResponse, error) {
	// Generar access token
	accessToken, err := s.generateAccessToken(user)
	if err != nil {
		return nil, fmt.Errorf("failed to generate access token: %w", err)
	}

	// Generar refresh token
	refreshToken, err := s.generateRefreshToken(user)
	if err != nil {
		return nil, fmt.Errorf("failed to generate refresh token: %w", err)
	}

	return &models.AuthResponse{
		User:         user.ToPublic(),
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		ExpiresIn:    int64(s.config.Expiry.Seconds()),
	}, nil
}

// generateAccessToken genera un JWT access token
func (s *AuthService) generateAccessToken(user *models.User) (string, error) {
	claims := Claims{
		UserID: user.ID,
		Email:  user.Email,
		Role:   user.Role,
		RegisteredClaims: jwt.RegisteredClaims{
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(s.config.Expiry)),
			IssuedAt:  jwt.NewNumericDate(time.Now()),
			NotBefore: jwt.NewNumericDate(time.Now()),
			Issuer:    "chiloe-flora-auth",
			Subject:   fmt.Sprintf("user:%d", user.ID),
		},
	}

	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	return token.SignedString([]byte(s.config.Secret))
}

// generateRefreshToken genera y almacena un refresh token
func (s *AuthService) generateRefreshToken(user *models.User) (string, error) {
	// Generar token aleatorio
	tokenBytes := make([]byte, 32)
	if _, err := rand.Read(tokenBytes); err != nil {
		return "", fmt.Errorf("failed to generate random token: %w", err)
	}
	token := hex.EncodeToString(tokenBytes)

	// Crear registro de refresh token
	refreshToken := models.RefreshToken{
		UserID:    user.ID,
		Token:     s.hashToken(token),
		ExpiresAt: time.Now().Add(s.config.RefreshExpiry),
	}

	// Limpiar tokens vencidos del usuario
	s.db.Where("user_id = ? AND expires_at < ?", user.ID, time.Now()).Delete(&models.RefreshToken{})

	// Guardar nuevo token
	if err := s.db.Create(&refreshToken).Error; err != nil {
		return "", fmt.Errorf("failed to save refresh token: %w", err)
	}

	return token, nil
}

// ProcessOAuthUser procesa un usuario de OAuth (crear o actualizar)
func (s *AuthService) ProcessOAuthUser(provider, providerID, email, name, avatar string) (*models.AuthResponse, error) {
	var user models.User
	err := s.db.Where("email = ? OR (provider = ? AND provider_id = ?)",
		email, provider, providerID).First(&user).Error

	if err != nil && !errors.Is(err, gorm.ErrRecordNotFound) {
		return nil, fmt.Errorf("database error: %w", err)
	}

	// Si el usuario no existe, crearlo
	if errors.Is(err, gorm.ErrRecordNotFound) {
		user = models.User{
			Email:         email,
			Password:      "", // No password for OAuth users
			Name:          name,
			Avatar:        avatar,
			Role:          models.UserRoleUser,
			Status:        models.UserStatusActive,
			Provider:      provider,
			ProviderID:    providerID,
			EmailVerified: true, // OAuth emails are considered verified
		}

		if err := s.db.Create(&user).Error; err != nil {
			return nil, fmt.Errorf("failed to create OAuth user: %w", err)
		}
	} else {
		// Actualizar información si es necesario
		user.Name = name
		user.Avatar = avatar
		user.EmailVerified = true

		if err := s.db.Save(&user).Error; err != nil {
			return nil, fmt.Errorf("failed to update OAuth user: %w", err)
		}
	}

	// Generar tokens
	return s.generateAuthResponse(&user)
}

func (s *AuthService) ProcessGoogleIDTokenUser(info *GoogleIDTokenInfo) (*models.AuthResponse, error) {
	if info == nil || info.Subject == "" || info.Email == "" {
		return nil, ErrInvalidOAuthToken
	}

	googleSub := info.Subject
	var user models.User
	err := s.db.Where("google_sub = ?", googleSub).First(&user).Error
	if errors.Is(err, gorm.ErrRecordNotFound) {
		err = s.db.Where("email = ?", info.Email).First(&user).Error
	}

	if err != nil && !errors.Is(err, gorm.ErrRecordNotFound) {
		return nil, fmt.Errorf("database error: %w", err)
	}

	name := info.Name
	if name == "" {
		name = info.Email
	}

	if errors.Is(err, gorm.ErrRecordNotFound) {
		user = models.User{
			Email:         info.Email,
			Password:      "",
			Name:          name,
			Avatar:        info.Picture,
			Role:          models.UserRoleUser,
			Status:        models.UserStatusActive,
			Provider:      "google",
			ProviderID:    googleSub,
			GoogleSub:     &googleSub,
			EmailVerified: info.EmailVerified,
		}

		if err := s.db.Create(&user).Error; err != nil {
			return nil, fmt.Errorf("failed to create Google user: %w", err)
		}
	} else {
		if user.GoogleSub != nil && *user.GoogleSub != googleSub {
			return nil, ErrInvalidOAuthToken
		}
		user.GoogleSub = &googleSub
		user.EmailVerified = info.EmailVerified
		user.Name = name
		user.Avatar = info.Picture
		if user.Provider == "" {
			user.Provider = "local"
		}
		if user.Provider == "google" {
			user.ProviderID = googleSub
		}
		if user.Status != models.UserStatusActive {
			return nil, ErrUserNotActive
		}

		if err := s.db.Save(&user).Error; err != nil {
			return nil, fmt.Errorf("failed to update Google user: %w", err)
		}
	}

	return s.generateAuthResponse(&user)
}

// hashPassword genera hash de contraseña
func (s *AuthService) hashPassword(password string) (string, error) {
	bytes, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	return string(bytes), err
}

// hashToken genera hash de token para almacenamiento seguro
func (s *AuthService) hashToken(token string) string {
	// En producción, usar un hash más seguro
	return fmt.Sprintf("sha256:%x", token)
}
