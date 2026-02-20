# Development Document Log

This document captures the development history of the Contractor Quotes & Sourcing application.

## Phase 1: Initial Application

The application was built as a C++ web app using the Wt (Witty) framework with Bootstrap 5 styling.

### Core Features
- **Wt::Dbo ORM** with SQLite backend for data persistence
- **Session model** with automatic seeding of sample data (products, suppliers, clients)
- **SourcingEngine** for multi-supplier comparison using composite scoring (price, reliability, proximity, lead time)
- **6 Wt views**: Dashboard, Client Management, Product Catalog, Supplier Directory, Quote Builder, and Supplier Comparison dialog
- **Bootstrap 5** theme with custom CSS
- **Boost.Test** unit test suite for the sourcing engine

### Database
- 6 tables: product, supplier, supplier_product, client, quote, quote_line_item
- 27 products across 7 building material categories
- 8 suppliers in Texas metro areas (Austin, Houston, Dallas, San Antonio)
- 144 supplier-product relationships with deterministic pricing algorithm
- 5 sample clients

## Phase 2: Bootstrap 5 Resource Fix

**Issue**: Wt was returning 404 for its internal theme files (Bootstrap 5 JS/CSS) because `--docroot` only covers application static files.

**Fix**: Added `--resources-dir` flag to both `scripts/start.sh` and the CMake `run` target so Wt can locate its internal resources directory alongside the app's `--docroot`.

**Files changed**: `scripts/start.sh`, `CMakeLists.txt`

## Phase 3: Database Schema & Seed Data Export

Created standalone SQL files to support external database provisioning (for ApiLogicServer and PostgreSQL deployments).

### Files Created
- `database/schema.sql` - SQLite DDL for all 6 tables
- `database/schema_pg.sql` - PostgreSQL DDL (SERIAL, BOOLEAN, TIMESTAMP types)
- `database/seed_data.sql` - SQLite INSERT statements reproducing the C++ seeding algorithm
- `database/seed_data_pg.sql` - PostgreSQL seed data with TRUE/FALSE booleans and sequence resets
- `database/generate_seed.py` - Python script that reproduces the deterministic supplier-product pricing algorithm from C++

### Validation
- SQLite in-memory test confirmed: 27 products, 8 suppliers, 144 supplier_products, 5 clients, 0 orphan foreign keys

## Phase 4: ApiLogicServer Integration & Architecture Abstraction

Major refactoring to support dual runtime architectures: direct local database or REST API client/server via ApiLogicServer.

### Design

Introduced a **DataProvider** abstraction layer with two implementations:

1. **LocalDataProvider** - Wraps the existing Wt::Dbo Session for direct SQLite/PostgreSQL access
2. **ApiDataProvider** - HTTP client using Boost.Asio synchronous TCP sockets, communicating with ApiLogicServer's JSON:API endpoints

All views and the SourcingEngine were refactored to depend on the abstract `DataProvider` interface rather than `Wt::Dbo::Session` directly.

### Configuration

Runtime architecture is selected via `model/app_config.yaml`:
```yaml
architecture: api    # or "local"
```

A custom minimal YAML parser handles the configuration file (yaml-cpp was not available in the build environment).

### ApiLogicServer Compatibility

- Entity names use PascalCase: `Product`, `Supplier`, `SupplierProduct`, `Client`, `Quote`, `QuoteLineItem`
- Column/attribute names use snake_case: `zip_code`, `lead_time_days`, `unit_price`, `product_id`, etc.
- Request/response format follows JSON:API (SAFRS): `{ "data": { "id": "1", "type": "Product", "attributes": {...} } }`
- Filtering: `?filter[product_id]=X`
- Pagination: `?page[limit]=1000`
- Sorting: `?sort=-created_date`
- Validated against `model/app_model.yaml` deployed from ApiLogicServer

### Files Created
- `include/data/DataTypes.h` - DTO structs (ProductDTO, SupplierDTO, etc.)
- `include/data/DataProvider.h` - Abstract interface for all data operations
- `include/data/LocalDataProvider.h` - Wt::Dbo implementation header
- `include/data/ApiDataProvider.h` - REST client implementation header
- `include/config/AppConfig.h` - Configuration struct with Architecture/LocalDatabase enums
- `src/config/AppConfig.cpp` - YAML parser implementation
- `src/data/LocalDataProvider.cpp` - Full Wt::Dbo data provider (CRUD for all 6 entities)
- `src/data/ApiDataProvider.cpp` - Full HTTP/JSON:API data provider using Boost.Asio
- `model/app_config.yaml` - Runtime configuration file

### Files Modified
- `include/engine/SourcingEngine.h` - Changed from `Wt::Dbo::Session&` to `DataProvider&`
- `src/engine/SourcingEngine.cpp` - Uses DataProvider methods instead of Dbo queries
- `include/views/ContractorApp.h` - Changed from `Session&` to `DataProvider&`
- `include/views/DashboardView.h` - Changed to `DataProvider&`
- `include/views/ClientView.h` - Changed to `DataProvider&`
- `include/views/ProductCatalogView.h` - Changed to `DataProvider&`
- `include/views/SupplierView.h` - Changed to `DataProvider&`
- `include/views/QuoteBuilderView.h` - Changed to `DataProvider&`
- `src/views/ContractorApp.cpp` - Uses DataProvider
- `src/views/DashboardView.cpp` - Rewritten to use provider_ and DTOs
- `src/views/ClientView.cpp` - Rewritten to use provider_ and DTOs
- `src/views/ProductCatalogView.cpp` - Rewritten to use provider_ and DTOs
- `src/views/SupplierView.cpp` - Rewritten to use provider_ and DTOs
- `src/views/QuoteBuilderView.cpp` - Rewritten to use provider_ and DTOs
- `src/main.cpp` - Loads AppConfig, creates appropriate DataProvider, passes to ContractorApp
- `CMakeLists.txt` - Added new source files, model directory copy target
- `tests/CMakeLists.txt` - Added LocalDataProvider.cpp to test build
- `tests/test_sourcing_engine.cpp` - Updated to use LocalDataProvider

## Phase 5: Documentation

- Created `README.md` with architecture switching documentation, build instructions, and project structure
- Created `docs/DOCUMENT_LOG.md` (this file)

## Phase 6: Mobile Application (React + TypeScript + Capacitor)

Added a hybrid mobile app in `/mobile` that connects to the same ApiLogicServer REST backend as the C++ Wt desktop app.

### Technology Stack
- **React 18** with functional components and hooks
- **TypeScript** for type safety, with interfaces mirroring the C++ DTOs
- **Vite** for fast development builds and production bundling
- **Capacitor 6** for native iOS and Android packaging
- **React Router 6** for client-side navigation

### Architecture
The mobile app operates in API mode only (no local database). It connects directly to ApiLogicServer using the same JSON:API format as the C++ `ApiDataProvider`. The API base URL is configurable at runtime via the Settings screen (stored in `localStorage`).

### Screens
- **Dashboard** - Summary cards (product/supplier/client/quote counts), category breakdown, recent quotes
- **Clients** - Full CRUD: list, create, edit, delete with form validation
- **Products** - Category-filtered list with drill-down to supplier pricing/availability
- **Suppliers** - List with ratings/lead times, drill-down to product inventory
- **Quote Builder** - Full quote lifecycle: create, edit status, add/remove line items, delete
- **Settings** - Configure API server URL, app info

### Files Created
- `mobile/package.json` - Project manifest with React, Capacitor, and Vite dependencies
- `mobile/tsconfig.json` - TypeScript compiler configuration
- `mobile/vite.config.ts` - Vite build configuration
- `mobile/capacitor.config.ts` - Capacitor native app configuration
- `mobile/index.html` - SPA entry point with mobile viewport meta tags
- `mobile/.gitignore` - Excludes node_modules, dist, ios/, android/
- `mobile/src/main.tsx` - React app bootstrap
- `mobile/src/App.tsx` - Route definitions
- `mobile/src/App.css` - Mobile-first responsive styles with safe area support
- `mobile/src/vite-env.d.ts` - Vite type declarations
- `mobile/src/types/models.ts` - TypeScript interfaces matching C++ DTOs
- `mobile/src/services/api.ts` - JSON:API client for ApiLogicServer
- `mobile/src/components/Layout.tsx` - Tab bar navigation shell
- `mobile/src/pages/Dashboard.tsx` - Dashboard screen
- `mobile/src/pages/Clients.tsx` - Client management with CRUD
- `mobile/src/pages/Products.tsx` - Product catalog with category filters
- `mobile/src/pages/Suppliers.tsx` - Supplier directory
- `mobile/src/pages/QuoteBuilder.tsx` - Quote builder with line items
- `mobile/src/pages/Settings.tsx` - API configuration
