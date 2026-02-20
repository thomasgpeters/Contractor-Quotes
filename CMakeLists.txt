cmake_minimum_required(VERSION 3.16)
project(ContractorQuotes
    VERSION 1.0.0
    DESCRIPTION "Contractor Quote & Product Sourcing Application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Wt libraries
find_package(wt REQUIRED COMPONENTS wt wthttp wtdbo wtdbosqlite3)

# Find Boost (required by Wt)
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

# Include directories
target_include_directories(contractor_quotes PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(contractor_quotes PRIVATE
    Wt::Wt
    Wt::HTTP
    Wt::Dbo
    Wt::DboSqlite3
    Boost::system
    Boost::filesystem
    Boost::thread
)

# Copy CSS resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/resources/css DESTINATION ${CMAKE_BINARY_DIR})

# Install target
install(TARGETS contractor_quotes DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/contractor_quotes/resources)
