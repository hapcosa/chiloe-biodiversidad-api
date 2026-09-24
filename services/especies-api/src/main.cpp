// Copyright 2025 <Obrero/Obrero>
#include <pistache/endpoint.h>
#include <pistache/router.h>

#include <iostream>
#include <memory>

#include "../include/controllers/especie_controller.hpp"
#include "../include/controllers/area_protegida_controller.hpp"
#include "../include/controllers/avistamiento_controller.hpp"
#include "../include/controllers/portada_controller.hpp"
#include "../include/controllers/categoria_controller.hpp"
#include "../include/controllers/curaduria_usuarios_controller.hpp"
#include "../include/controllers/familia_controller.hpp"
#include "../include/controllers/identificacion_controller.hpp"
#include "../include/controllers/insignia_controller.hpp"
#include "../include/controllers/genero_controller.hpp"
#include "../include/controllers/moderador_controller.hpp"
#include "../include/controllers/postulacion_controller.hpp"
#include "../include/controllers/schema_controller.hpp"
#include "../include/controllers/upload_controller.hpp"
#include "../include/repository/postgres_familia_repository.hpp"
#include "../include/repository/postgres_genero_repository.hpp"
#include "../include/repository/postgresql_especie_repository.hpp"
#include "../include/repository/postgres_area_protegida_repository.hpp"
#include "../include/repository/postgres_avistamiento_repository.hpp"
#include "../include/repository/postgres_categoria_repository.hpp"
#include "../include/repository/postgres_moderador_categoria_repository.hpp"
#include "../include/repository/postgres_identificacion_repository.hpp"
#include "../include/repository/postgres_insignia_repository.hpp"
#include "../include/repository/postgres_postulacion_repository.hpp"
#include "../include/services/area_protegida_service.hpp"
#include "../include/services/avistamiento_service.hpp"
#include "../include/services/portada_service.hpp"
#include "../include/services/auth_usuarios_client.hpp"
#include "../include/services/categoria_service.hpp"
#include "../include/services/curaduria_usuarios_service.hpp"
#include "../include/services/especie_service.hpp"
#include "../include/services/familia_service.hpp"
#include "../include/services/genero_service.hpp"
#include "../include/services/identificacion_service.hpp"
#include "../include/services/insignia_service.hpp"
#include "../include/services/moderacion_service.hpp"
#include "../include/services/postulacion_service.hpp"
#include "../include/services/upload_presign_service.hpp"
#include "../include/utils/atributos_schema_validator.hpp"
#include "../include/utils/database.hpp"
#include "../include/utils/config.hpp"

int main(int argc, char** argv) {
  // Get port from environment or use default
	Config config;
	int port = config.getApiPort();
	std::string host = config.getApiHost();

	Pistache::Address addr(host, Pistache::Port(port));


  // Setup connection string para PostgreSQL
  std::string connectionString =
      "host=" +
      std::string(std::getenv("DB_HOST") ? std::getenv("DB_HOST")
                                         : "localhost") +
      " port=" +
      std::string(std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432") +
      " dbname=" +
      std::string(std::getenv("DB_NAME") ? std::getenv("DB_NAME")
                                         : "chiloe_flora") +
      " user=" +
      std::string(std::getenv("DB_USER") ? std::getenv("DB_USER")
                                         : "postgres") +
      " password=" +
      std::string(std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD")
                                             : "postgres");

  printf("Connection string: %s\n", connectionString.c_str());
  auto dataBase = std::make_shared<Database>(connectionString);

  // Initialize repositories
  auto familiaRepository =
      std::make_shared<PostgresFamiliaRepository>(dataBase);
  auto generoRepository = std::make_shared<PostgresGeneroRepository>(dataBase);
  auto especieRepository =
      std::make_shared<PostgreSQLEspecieRepository>(dataBase);
  auto avistamientoRepository =
      std::make_shared<PostgresAvistamientoRepository>(dataBase);
  auto categoriaRepository =
      std::make_shared<PostgresCategoriaRepository>(dataBase);
  auto moderadorCategoriaRepository =
      std::make_shared<PostgresModeradorCategoriaRepository>(dataBase);
  auto postulacionRepository =
      std::make_shared<PostgresPostulacionRepository>(dataBase);
  auto identificacionRepository =
      std::make_shared<PostgresIdentificacionRepository>(dataBase);
  auto areaProtegidaRepository =
      std::make_shared<PostgresAreaProtegidaRepository>(dataBase);
  auto insigniaRepository =
      std::make_shared<PostgresInsigniaRepository>(dataBase);

  // El schema lo gestiona scripts/migrate.sh contra la BD antes de arrancar
  // el binario (ver services/especies-api/migrations/README.md).

  // Cargar JSON Schemas de atributos por reino. Si falla, abortar arranque:
  // un servicio sin validadores aceptaría JSONB arbitrario y rompería el
  // contrato del schema multi-reino.
  const std::string schemasDir =
      std::getenv("SCHEMAS_DIR") ? std::getenv("SCHEMAS_DIR")
                                  : "/etc/chiloe-especies-api/schemas";
  auto schemaValidator =
      std::make_shared<AtributosSchemaValidator>(schemasDir);
  std::cout << "JSON Schemas cargados desde: " << schemasDir << std::endl;

  // Initialize services
  auto familiaService = std::make_shared<FamiliaService>(familiaRepository);
  auto generoService = std::make_shared<GeneroService>(generoRepository);
  auto uploadPresignService = std::make_shared<UploadPresignService>();
  auto moderacionService =
      std::make_shared<ModeracionService>(moderadorCategoriaRepository);
  auto especieService =
      std::make_shared<EspecieService>(especieRepository, schemaValidator,
                                       uploadPresignService);
  auto avistamientoService =
      std::make_shared<AvistamientoService>(avistamientoRepository,
                                            uploadPresignService);
  auto categoriaService = std::make_shared<CategoriaService>(categoriaRepository);
  auto areaProtegidaService =
      std::make_shared<AreaProtegidaService>(areaProtegidaRepository);
  auto postulacionService = std::make_shared<PostulacionService>(
      postulacionRepository, categoriaRepository);
  auto identificacionService = std::make_shared<IdentificacionService>(
      identificacionRepository, avistamientoRepository, especieRepository,
      moderacionService);
  auto portadaService =
      std::make_shared<PortadaService>(especieService, avistamientoService);
  auto insigniaService = std::make_shared<InsigniaService>(insigniaRepository);
  auto authUsuariosClient = std::make_shared<AuthUsuariosClient>();
  auto curaduriaUsuariosService = std::make_shared<CuraduriaUsuariosService>(
      authUsuariosClient, moderadorCategoriaRepository);

  // Initialize controllers
  auto familiaController = std::make_shared<FamiliaController>(familiaService);
  auto generoController = std::make_shared<GeneroController>(generoService);
  auto especieController =
      std::make_shared<EspecieController>(especieService, moderacionService);
  auto avistamientoController =
      std::make_shared<AvistamientoController>(avistamientoService);
  auto categoriaController =
      std::make_shared<CategoriaController>(categoriaService);
  auto areaProtegidaController =
      std::make_shared<AreaProtegidaController>(areaProtegidaService);
  auto moderadorController =
      std::make_shared<ModeradorController>(moderacionService);
  auto postulacionController =
      std::make_shared<PostulacionController>(postulacionService);
  auto identificacionController =
      std::make_shared<IdentificacionController>(identificacionService);
  auto uploadController =
      std::make_shared<UploadController>(uploadPresignService);
  auto schemaController = std::make_shared<SchemaController>(schemaValidator);
  auto portadaController = std::make_shared<PortadaController>(portadaService);
  auto insigniaController =
      std::make_shared<InsigniaController>(insigniaService);
  auto curaduriaUsuariosController =
      std::make_shared<CuraduriaUsuariosController>(curaduriaUsuariosService);

  // Setup router
  auto router = std::make_shared<Pistache::Rest::Router>();

  // Health check endpoint
  Pistache::Rest::Routes::Get(*router, "/health",
                              [](const Pistache::Rest::Request& request,
                                 Pistache::Http::ResponseWriter response)
                                  -> Pistache::Rest::Route::Result {
                                response.send(Pistache::Http::Code::Ok, "OK");
                                return Pistache::Rest::Route::Result::Ok;
                              });

  // Setup routes
  FamiliaController::setupRoutes(*router, familiaController);
  GeneroController::setupRoutes(*router, generoController);
  EspecieController::setupRoutes(*router, especieController);
  AvistamientoController::setupRoutes(*router, avistamientoController);
  AreaProtegidaController::setupRoutes(*router, areaProtegidaController);
  PortadaController::setupRoutes(*router, portadaController);
  CategoriaController::setupRoutes(*router, categoriaController);
  ModeradorController::setupRoutes(*router, moderadorController);
  PostulacionController::setupRoutes(*router, postulacionController);
  IdentificacionController::setupRoutes(*router, identificacionController);
  UploadController::setupRoutes(*router, uploadController);
  SchemaController::setupRoutes(*router, schemaController);
  InsigniaController::setupRoutes(*router, insigniaController);
  CuraduriaUsuariosController::setupRoutes(*router, curaduriaUsuariosController);

  // Configure server - MEJORADO para red local
  Pistache::Http::Endpoint server(addr);
  auto opts =
      Pistache::Http::Endpoint::options()
          .threads(4)  // Number of threads
          .flags(Pistache::Tcp::Options::ReuseAddr |
                 Pistache::Tcp::Options::ReusePort)  // Añadido ReusePort
          .maxRequestSize(
              50 * 1024 *
              1024);  // 50MB máximo por request (reemplaza maxPayload)

  server.init(opts);
  server.setHandler(router->handler());

  // Start server - MEJORADO el mensaje
  std::cout << "==================================" << std::endl;
  std::cout << "Server starting on port " << port << std::endl;
  std::cout << "Local access: http://localhost:" << port << std::endl;
  std::cout << "Network access: http://[YOUR_LOCAL_IP]:" << port << std::endl;
  std::cout << "Health check: http://localhost:" << port << "/health"
            << std::endl;
  std::cout << "==================================" << std::endl;

  server.serve();

  return 0;
}
