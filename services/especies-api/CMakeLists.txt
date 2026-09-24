cmake_minimum_required(VERSION 3.10)

project(ChiloeFloraAPI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuración de debug/release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra")
    message(STATUS "Configurando build de desarrollo")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
    message(STATUS "Configurando build de producción")
endif()

# Encontrar paquetes necesarios
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Configuración mejorada para Pistache
pkg_check_modules(Pistache REQUIRED libpistache)

# Verificar que Pistache se encontró correctamente
if(NOT Pistache_FOUND)
    message(FATAL_ERROR "Pistache no encontrado. Instálalo con: sudo apt-get install libpistache-dev")
endif()

# Mostrar información de debug de Pistache
message(STATUS "Pistache encontrado:")
message(STATUS "  - Version: ${Pistache_VERSION}")
message(STATUS "  - Include dirs: ${Pistache_INCLUDE_DIRS}")
message(STATUS "  - Libraries: ${Pistache_LIBRARIES}")
message(STATUS "  - Library dirs: ${Pistache_LIBRARY_DIRS}")
message(STATUS "  - Flags: ${Pistache_CFLAGS_OTHER}")

# Buscar librerías adicionales
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

# Verificar estructura de directorios
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src)
    message(FATAL_ERROR "Directorio src no encontrado: ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
    message(FATAL_ERROR "Directorio include no encontrado: ${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

# Include directories (rutas correctas para Docker)
include_directories(${Pistache_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(/usr/local/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Directorios de bibliotecas
link_directories(${Pistache_LIBRARY_DIRS})

# Verificar que los archivos fuente existen
set(REQUIRED_SOURCES
        src/main.cpp
        src/utils/database.cpp
        src/utils/dotenv.cpp
        src/controllers/familia_controller.cpp
        src/controllers/genero_controller.cpp
        src/controllers/especie_controller.cpp
        src/controllers/avistamiento_controller.cpp
        src/controllers/area_protegida_controller.cpp
        src/controllers/portada_controller.cpp
        src/controllers/categoria_controller.cpp
        src/controllers/moderador_controller.cpp
        src/controllers/curaduria_usuarios_controller.cpp
        src/controllers/identificacion_controller.cpp
        src/controllers/postulacion_controller.cpp
        src/controllers/insignia_controller.cpp
        src/controllers/schema_controller.cpp
        src/controllers/upload_controller.cpp
        src/models/familia.cpp
        src/models/genero.cpp
        src/models/especie.cpp
        src/models/avistamiento.cpp
        src/models/celda_mapa.cpp
        src/models/area_protegida.cpp
        src/models/portada.cpp
        src/models/categoria_moderacion.cpp
        src/models/identificacion.cpp
        src/models/postulacion_curador.cpp
        src/models/insignia.cpp
        src/models/especie_estado.cpp
        src/models/reino.cpp
        src/repository/postgres_familia_repository.cpp
        src/repository/postgres_genero_repository.cpp
        src/repository/postgresql_especie_repository.cpp
        src/repository/postgres_avistamiento_repository.cpp
        src/repository/postgres_area_protegida_repository.cpp
        src/repository/postgres_categoria_repository.cpp
        src/repository/postgres_moderador_categoria_repository.cpp
        src/repository/postgres_identificacion_repository.cpp
        src/repository/postgres_postulacion_repository.cpp
        src/repository/postgres_insignia_repository.cpp
        src/services/familia_service.cpp
        src/services/genero_service.cpp
        src/services/especie_service.cpp
        src/services/avistamiento_service.cpp
        src/services/area_protegida_service.cpp
        src/services/portada_service.cpp
        src/services/avistamiento_visibilidad.cpp
        src/services/categoria_service.cpp
        src/services/moderacion_service.cpp
        src/services/auth_usuarios_client.cpp
        src/services/curaduria_usuarios_service.cpp
        src/services/identificacion_service.cpp
        src/services/postulacion_service.cpp
        src/services/insignia_service.cpp
        src/services/upload_presign_service.cpp
        src/utils/config.cpp
        src/utils/atributos_schema_validator.cpp
        src/utils/request_identity.cpp
        src/utils/timestamps.cpp
        src/utils/fechas.cpp
        src/utils/conservacion.cpp
        src/models/imagen.cpp
)

# Verificar que los archivos existen
foreach(SOURCE_FILE ${REQUIRED_SOURCES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
        message(WARNING "Archivo fuente no encontrado: ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
    endif()
endforeach()

# Verificar archivos de cabecera (rutas corregidas)
set(REQUIRED_HEADERS
        include/utils/database.hpp
        include/utils/dotenv.hpp
        include/controllers/familia_controller.hpp
        include/controllers/genero_controller.hpp
        include/controllers/especie_controller.hpp
        include/controllers/avistamiento_controller.hpp
        include/controllers/categoria_controller.hpp
        include/controllers/moderador_controller.hpp
        include/controllers/identificacion_controller.hpp
        include/controllers/postulacion_controller.hpp
        include/controllers/insignia_controller.hpp
        include/controllers/portada_controller.hpp
        include/controllers/schema_controller.hpp
        include/controllers/upload_controller.hpp
        include/models/especie.hpp
        include/models/avistamiento.hpp
        include/models/portada.hpp
        include/models/categoria_moderacion.hpp
        include/models/identificacion.hpp
        include/models/postulacion_curador.hpp
        include/models/insignia.hpp
        include/models/familia.hpp
        include/models/genero.hpp
        include/models/especie_estado.hpp
        include/models/reino.hpp
        include/repository/postgres_familia_repository.hpp
        include/repository/postgres_genero_repository.hpp
        include/repository/postgresql_especie_repository.hpp
        include/repository/postgres_avistamiento_repository.hpp
        include/repository/avistamiento_repository.hpp
        include/repository/categoria_repository.hpp
        include/repository/postgres_categoria_repository.hpp
        include/repository/moderador_categoria_repository.hpp
        include/repository/postgres_moderador_categoria_repository.hpp
        include/repository/identificacion_repository.hpp
        include/repository/postgres_identificacion_repository.hpp
        include/repository/postulacion_repository.hpp
        include/repository/postgres_postulacion_repository.hpp
        include/repository/insignia_repository.hpp
        include/repository/postgres_insignia_repository.hpp
        include/repository/familia_repository.hpp
        include/repository/genero_repository.hpp
        include/repository/especie_repository.hpp
        include/services/familia_service.hpp
        include/services/genero_service.hpp
        include/services/especie_service.hpp
        include/services/avistamiento_service.hpp
        include/services/categoria_service.hpp
        include/services/moderacion_service.hpp
        include/services/identificacion_service.hpp
        include/services/portada_service.hpp
        include/services/postulacion_service.hpp
        include/services/insignia_service.hpp
        include/services/upload_presign_service.hpp
        include/utils/config.hpp
        include/utils/atributos_schema_validator.hpp
        include/utils/request_identity.hpp
        include/utils/timestamps.hpp
        include/models/imagen.hpp
        include/utils/constants.hpp
)

# Verificar que los archivos de cabecera existen
foreach(HEADER_FILE ${REQUIRED_HEADERS})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE})
        message(WARNING "Archivo de cabecera no encontrado: ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE}")
    endif()
endforeach()

# Fuentes (solo archivos .cpp, sin headers)
set(SOURCES
        src/main.cpp
        src/utils/database.cpp
        src/utils/dotenv.cpp
        src/controllers/familia_controller.cpp
        src/controllers/genero_controller.cpp
        src/controllers/especie_controller.cpp
        src/controllers/avistamiento_controller.cpp
        src/controllers/area_protegida_controller.cpp
        src/controllers/portada_controller.cpp
        src/controllers/categoria_controller.cpp
        src/controllers/moderador_controller.cpp
        src/controllers/curaduria_usuarios_controller.cpp
        src/controllers/identificacion_controller.cpp
        src/controllers/postulacion_controller.cpp
        src/controllers/insignia_controller.cpp
        src/controllers/schema_controller.cpp
        src/controllers/upload_controller.cpp
        src/models/familia.cpp
        src/models/genero.cpp
        src/models/especie.cpp
        src/models/avistamiento.cpp
        src/models/celda_mapa.cpp
        src/models/area_protegida.cpp
        src/models/portada.cpp
        src/models/categoria_moderacion.cpp
        src/models/identificacion.cpp
        src/models/postulacion_curador.cpp
        src/models/insignia.cpp
        src/models/especie_estado.cpp
        src/models/reino.cpp
        src/repository/postgres_familia_repository.cpp
        src/repository/postgres_genero_repository.cpp
        src/repository/postgresql_especie_repository.cpp
        src/repository/postgres_avistamiento_repository.cpp
        src/repository/postgres_area_protegida_repository.cpp
        src/repository/postgres_categoria_repository.cpp
        src/repository/postgres_moderador_categoria_repository.cpp
        src/repository/postgres_identificacion_repository.cpp
        src/repository/postgres_postulacion_repository.cpp
        src/repository/postgres_insignia_repository.cpp
        src/services/familia_service.cpp
        src/services/genero_service.cpp
        src/services/especie_service.cpp
        src/services/avistamiento_service.cpp
        src/services/area_protegida_service.cpp
        src/services/portada_service.cpp
        src/services/avistamiento_visibilidad.cpp
        src/services/categoria_service.cpp
        src/services/moderacion_service.cpp
        src/services/auth_usuarios_client.cpp
        src/services/curaduria_usuarios_service.cpp
        src/services/identificacion_service.cpp
        src/services/postulacion_service.cpp
        src/services/insignia_service.cpp
        src/services/upload_presign_service.cpp
        src/utils/config.cpp
        src/utils/atributos_schema_validator.cpp
        src/utils/request_identity.cpp
        src/utils/timestamps.cpp
        src/utils/fechas.cpp
        src/utils/conservacion.cpp
        src/utils/query_params.cpp
        src/models/imagen.cpp
)

# Filtrar solo archivos que existen
set(EXISTING_SOURCES)
foreach(SOURCE_FILE ${SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
        list(APPEND EXISTING_SOURCES ${SOURCE_FILE})
        message(STATUS "Archivo fuente encontrado: ${SOURCE_FILE}")
    else()
        message(WARNING "Archivo fuente no encontrado: ${SOURCE_FILE}")
    endif()
endforeach()

# Verificar que tenemos al menos main.cpp
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
    message(FATAL_ERROR "main.cpp no encontrado en src/")
endif()

# Si no hay fuentes, usar solo main.cpp
if(NOT EXISTING_SOURCES)
    set(EXISTING_SOURCES src/main.cpp)
    message(WARNING "Solo se encontró main.cpp, compilando con archivos mínimos")
endif()

# Ejecutable
add_executable(chiloe_especies_api ${EXISTING_SOURCES})

# Aplicar flags de compilación de Pistache
target_compile_options(chiloe_especies_api PRIVATE ${Pistache_CFLAGS_OTHER})

# Propiedades del target
set_target_properties(chiloe_especies_api PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Enlazar bibliotecas con orden específico.
# nlohmann_json_schema_validator viene de pboettch/json-schema-validator y
# valida atributos_especificos JSONB por reino. Está instalada en /usr/local
# por el Dockerfile (no hay paquete en apt para Ubuntu 22.04).
target_link_libraries(chiloe_especies_api
        ${Pistache_LIBRARIES}
        ${PQXX_LIB}
        ${PQ_LIB}
        nlohmann_json_schema_validator
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CURL_LIBRARIES}
        Threads::Threads
)

# Instalar
install(TARGETS chiloe_especies_api DESTINATION bin)

# Tests unitarios (gtest). Off por defecto para no arrastrar GoogleTest en la
# imagen de producción; el stage `test` del Dockerfile y el CI lo activan.
option(BUILD_TESTS "Compilar tests unitarios con GoogleTest" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Target para verificar estructura de proyecto
add_custom_target(check-structure
        COMMAND echo "🔍 Verificando estructura del proyecto..."
        COMMAND echo "Directorio actual: ${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ls -la ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND echo "Contenido de src/:"
        COMMAND ls -la ${CMAKE_CURRENT_SOURCE_DIR}/src/ || echo "Directorio src/ no existe"
        COMMAND echo "Contenido de include/:"
        COMMAND ls -la ${CMAKE_CURRENT_SOURCE_DIR}/include/ || echo "Directorio include/ no existe"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Verificando estructura del proyecto"
)

# Target para compilación rápida (solo con archivos existentes)
add_custom_target(quick-build
        COMMAND echo "⚡ Compilación rápida..."
        COMMAND mkdir -p build && cd build && cmake .. && make -j$(nproc)
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compilación rápida"
)

# Targets de análisis estático mejorados
add_custom_target(static-analysis
        COMMAND echo "🔍 Ejecutando análisis estático..."
        COMMAND cppcheck --enable=all --suppress=missingIncludeSystem --inline-suppr --std=c++17 --project=${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_SOURCE_DIR}/src/ --xml --output-file=${CMAKE_BINARY_DIR}/cppcheck-report.xml || echo "Cppcheck no disponible"
        COMMAND echo "📊 Reporte cppcheck generado en build/cppcheck-report.xml"
        COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src/ -name "*.cpp" -o -name "*.hpp" | xargs cpplint --filter=-whitespace,-build/include_subdir,-build/header_guard --output=vs7 || echo "Cpplint no disponible"
        COMMAND echo "✅ Análisis completado"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Ejecutando análisis estático completo"
)

# Target para análisis rápido (solo errores críticos)
add_custom_target(quick-analysis
        COMMAND echo "⚡ Análisis rápido..."
        COMMAND cppcheck --enable=error,warning --suppress=missingIncludeSystem --std=c++17 --quiet ${CMAKE_CURRENT_SOURCE_DIR}/src/ || echo "Cppcheck no disponible"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Análisis rápido (solo errores críticos)"
)

# Target para formateo de código
add_custom_target(format
        COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_SOURCE_DIR}/include/ -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i --style=Google || echo "Clang-format no disponible"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formateando código con clang-format"
)

# Generar compile_commands.json para herramientas de análisis
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Mostrar información de configuración
message(STATUS "=== Configuración del proyecto ===")
message(STATUS "Directorio fuente: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Directorio build: ${CMAKE_BINARY_DIR}")
message(STATUS "Tipo de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compilador: ${CMAKE_CXX_COMPILER}")
message(STATUS "Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Archivos fuente encontrados: ${EXISTING_SOURCES}")
message(STATUS "===================================")
