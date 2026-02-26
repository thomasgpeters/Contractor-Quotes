# Contractor Quotes & Sourcing

A C++ web application built with [Wt (Witty)](https://www.webtoolkit.eu/wt) for managing contractor quotes, sourcing building materials from multiple suppliers, and comparing pricing. Features a multi-supplier sourcing engine that ranks sources by price, reliability, proximity, and lead time.

## Features

- **Dashboard** - Overview of products, suppliers, clients, and quotes with summary statistics
- **Product Catalog** - Browse building materials by category with supplier pricing ranges
- **Supplier Directory** - View registered suppliers and their inventory
- **Client Management** - Add, edit, and delete clients with full contact details
- **Quote Builder** - Create quotes with line items, assign products/suppliers, and track status
- **Sourcing Engine** - Compare supplier sources for any product with composite scoring based on price, reliability, proximity, and lead time

## Architecture Modes

The application supports two runtime architectures, configured via `model/app_config.yaml`:

### API Mode (Client/Server)

```yaml
architecture: api
```

Connects to an [ApiLogicServer](https://apilogicserver.github.io/Docs/) REST backend over HTTP. The API server manages the database and exposes JSON:API endpoints for all entities. This is the default mode.

- The Wt frontend makes HTTP requests to the API server
- Data is persisted and managed by ApiLogicServer (SQLAlchemy + Flask/SAFRS)
- Supports any database backend that ApiLogicServer supports (SQLite, PostgreSQL, MySQL, etc.)

**Setup:**
1. Start ApiLogicServer on the configured port (default: `http://localhost:5667/api`)
2. Set `architecture: api` in `model/app_config.yaml`
3. Start the Contractor Quotes application

### Local Mode (Direct Database)

```yaml
architecture: local
```

Connects directly to a local database using Wt::Dbo ORM. No external API server required.

- Supports SQLite (default) and PostgreSQL
- Database is seeded with sample data on first run
- Ideal for development, testing, and standalone deployments

**Setup:**
1. Set `architecture: local` in `model/app_config.yaml`
2. Choose database type: `local.database: sqlite` or `local.database: postgres`
3. Configure connection details as needed
4. Start the application - database will be created and seeded automatically

## Configuration

Edit `model/app_config.yaml` to switch between modes:

```yaml
# "api" for REST client/server, "local" for direct database
architecture: api

# Local database settings (used when architecture: local)
local:
  database: sqlite                        # sqlite | postgres
  sqlite_path: "contractor_quotes.db"
  postgres:
    host: "localhost"
    port: 5432
    database: "contractor_quotes"
    user: "postgres"
    password: ""

# ApiLogicServer settings (used when architecture: api)
api_logic_server:
  api_base_url: "http://localhost:5667/api"
  request_timeout: 30
```

## Building

### Prerequisites

- C++17 compiler (GCC 8+ or Clang 7+)
- CMake 3.16+
- Wt 4.x with Dbo and DboSqlite3
- Boost (system, filesystem, thread)

### Build Steps

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Running

```bash
# From the build directory
./contractor_quotes --docroot resources --resources-dir resources \
    --http-address 0.0.0.0 --http-port 8080

# Or use the start script
./start.sh
```

Then open `http://localhost:8080` in your browser.

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Project Structure

```
Contractor-Quotes/
├── include/
│   ├── config/
│   │   └── AppConfig.h          # Configuration struct and YAML loader
│   ├── data/
│   │   ├── DataTypes.h          # DTO structs (ProductDTO, SupplierDTO, etc.)
│   │   ├── DataProvider.h       # Abstract data access interface
│   │   ├── LocalDataProvider.h  # Wt::Dbo implementation
│   │   └── ApiDataProvider.h    # REST/HTTP implementation
│   ├── engine/
│   │   └── SourcingEngine.h     # Multi-supplier sourcing algorithm
│   ├── models/
│   │   └── *.h                  # Wt::Dbo model classes
│   └── views/
│       └── *.h                  # Wt UI view classes
├── src/
│   ├── config/AppConfig.cpp
│   ├── data/
│   │   ├── LocalDataProvider.cpp
│   │   └── ApiDataProvider.cpp
│   ├── engine/SourcingEngine.cpp
│   ├── models/Session.cpp
│   ├── views/*.cpp
│   └── main.cpp
├── model/
│   ├── app_config.yaml          # Runtime configuration
│   └── app_model.yaml           # ApiLogicServer entity model
├── database/
│   ├── schema.sql               # PostgreSQL DDL (default)
│   ├── schema_sqlite.sql        # SQLite DDL
│   ├── seed_data.sql            # PostgreSQL seed data (default)
│   └── seed_data_sqlite.sql     # SQLite seed data
├── mobile/                      # React + Capacitor mobile app
│   ├── src/
│   │   ├── types/models.ts      # TypeScript interfaces (mirrors C++ DTOs)
│   │   ├── services/api.ts      # JSON:API client for ApiLogicServer
│   │   ├── pages/               # Dashboard, Clients, Products, Suppliers, QuoteBuilder, Settings
│   │   └── components/          # Layout (tab bar navigation)
│   ├── capacitor.config.ts      # iOS/Android native config
│   ├── vite.config.ts           # Vite build config
│   └── package.json
├── resources/                   # Static web resources (CSS, Bootstrap 5)
├── tests/
│   └── test_sourcing_engine.cpp # Boost.Test unit tests
└── CMakeLists.txt
```

## Database Schema

Six tables: `product`, `supplier`, `supplier_product`, `client`, `quote`, `quote_line_item`.

- **27 products** across 7 categories (Lumber, Concrete, Roofing, etc.)
- **8 suppliers** in the Austin/Houston/Dallas/San Antonio TX metro areas
- **144 supplier-product** relationships with pricing and inventory
- **5 sample clients**

Schema files for both SQLite and PostgreSQL are in the `database/` directory.

## Mobile App

A hybrid mobile app in `mobile/` built with React, TypeScript, and Capacitor. It connects to the same ApiLogicServer REST backend.

### Quick Start

```bash
cd mobile
npm install
npm run dev          # Vite dev server at http://localhost:3000
```

### Build for Production

```bash
npm run build        # Outputs to mobile/dist/
```

### Native Builds (iOS/Android)

```bash
npx cap add ios      # First time only
npx cap add android  # First time only
npm run build && npx cap sync
npx cap open ios     # Opens Xcode
npx cap open android # Opens Android Studio
```

The API server URL defaults to `http://localhost:5667/api` and can be changed in the app's Settings screen.

### Mobile Screens

- **Dashboard** - Summary statistics with quick navigation
- **Clients** - Full CRUD management
- **Products** - Browse by category, view supplier pricing
- **Suppliers** - Ratings, lead times, inventory drill-down
- **Quote Builder** - Create quotes, manage line items
- **Settings** - Configure API endpoint

## Technology Stack

### Desktop (C++ / Wt)
- **C++17** with Wt (Witty) web framework
- **Wt::Dbo** ORM for local database mode
- **Boost.Asio** for synchronous HTTP client (API mode)
- **Wt::Json** for JSON parsing and serialization
- **Bootstrap 5** for responsive UI styling

### Mobile (React / Capacitor)
- **React 18** with TypeScript
- **Vite** for development and production builds
- **Capacitor 6** for native iOS and Android
- **React Router 6** for navigation

### Backend
- **ApiLogicServer** (optional) for REST API backend
