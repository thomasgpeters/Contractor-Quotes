cmake_minimum_required(VERSION 3.16)
project(ContractorQuotes VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress deprecation warnings from Wt internal headers (e.g., sprintf in WLogger.h)
add_compile_options(-Wno-deprecated-declarations)

# Find dependencies
find_package(Wt REQUIRED COMPONENTS Wt HTTP Dbo DboSqlite3)
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

# Source files
set(SOURCES
    src/main.cpp
    src/models/Session.cpp
    src/engine/SourcingEngine.cpp
    src/views/ContractorApp.cpp
    src/views/DashboardView.cpp
    src/views/ClientView.cpp
    src/views/ProductCatalogView.cpp
    src/views/SupplierView.cpp
    src/views/QuoteBuilderView.cpp
)

# Build executable
add_executable(contractor_quotes ${SOURCES})

target_include_directories(contractor_quotes PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Wt_INCLUDE_DIRS}
)

target_link_libraries(contractor_quotes
    Wt::Wt
    Wt::HTTP
    Wt::Dbo
    Wt::DboSqlite3
    Boost::system
    Boost::filesystem
    Boost::thread
)

# Configure HTTP port
set(QUOTES_HTTP_PORT 8080 CACHE STRING "HTTP port for Contractor Quotes")

# Copy resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

# Copy start script to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts/start.sh DESTINATION ${CMAKE_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)

# Install targets
install(TARGETS contractor_quotes RUNTIME DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/contractor_quotes/resources)

# Custom target to run
add_custom_target(run
    COMMAND contractor_quotes
        --docroot resources
        --http-address 0.0.0.0
        --http-port ${QUOTES_HTTP_PORT}
    DEPENDS contractor_quotes
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Testing
option(BUILD_TESTS "Build the test suite" ON)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
