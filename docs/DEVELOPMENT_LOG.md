# Development Log

This document captures recent enhancements, bug fixes, and refinements to the Contractor Quotes & Sourcing application following the initial Phase 1–6 development (see `docs/DOCUMENT_LOG.md`).

---

## Phase 7: UI Branding & Layout Improvements

### Branding
- Applied Imagery branding across both desktop (Wt) and mobile (React/Capacitor) apps
- Added brand images to `resources/` directory
- Reverted navbar to the original dark background color (`#1a2332`) after branding pass

### Quote Builder UX Overhaul
- **List/editor toggle**: The quote list is now hidden while editing a quote; saving or cancelling returns the user to the list view
- **Quote Details layout**: Rearranged to a responsive 3-column and 2-column grid for better use of space
- **Padding/spacing**: Tightened grid padding inside the Quote Details card to prevent spacing bloat
- **Save button**: Added top margin inside the details card so the Save button doesn't crowd the fields
- **Back navigation**: Replaced the "Back to Quotes" text button with a compact arrow icon + Quote ID label

### Desktop Navbar
- Made the navbar sticky at the top of the viewport so it remains visible during scroll
- Renamed the application title to "Contractor Quotes"
- Added an **About** dialog accessible from a navbar info button (pushed to the far right via `margin-left: auto`)
- Updated the info button tooltip to "Display Information"

### Files Changed
- `src/views/QuoteBuilderView.cpp` - List/editor toggling, layout restructure, back-button redesign
- `src/views/ContractorApp.cpp` - Sticky navbar, About dialog, title rename
- `resources/themes/contractor/style.css` - Grid padding, spacing, branding colors
- `mobile/src/App.css` - Mobile branding styles

---

## Phase 8: Quote List Data Not Updating After Save (Bug Fix)

### Problem
After saving a quote in the Quote Builder, the list view showed stale data:
- **Client** column always displayed "(none)"
- **Total** column always showed "$0.00"
- **Date Created** column showed "-"
- **Items** count always showed "0"

### Root Cause
The app defaults to `architecture: api` (see `model/app_config.yaml`). The ApiLogicServer REST API returns only raw database columns — foreign key IDs (e.g., `client_id` as an integer), not resolved names or computed aggregates. The `parseQuote()` function in `ApiDataProvider` read these raw fields but never resolved the denormalized values (`clientName`, `totalAmount`, `lineItemCount`) that the list view depends on.

The `LocalDataProvider` (Wt::Dbo mode) was unaffected because its `quoteToDTO()` helper resolves relationships via ORM lazy-loading within a transaction.

### Fix
Added an `enrichQuotes()` method to `ApiDataProvider` that batch-resolves computed fields after parsing:
- Fetches all clients in one call and builds a name lookup map
- Fetches all line items in one call and aggregates totals/counts per quote
- Called from `findAllQuotes()`, `findRecentQuotes()`, and `findQuoteById()`

Also ensured `createQuote()` always sends a `created_date` timestamp when one isn't provided, so new quotes have a creation date regardless of server-side defaults.

### Files Changed
- `include/data/ApiDataProvider.h` - Added `enrichQuotes()` private method declaration
- `src/data/ApiDataProvider.cpp` - Implemented enrichment; updated `createQuote()` to set `createdDate`

---

## Phase 9: Comprehensive Denormalized Field Resolution (Bug Fix)

### Problem
A full audit of the codebase revealed that the Phase 8 issue — missing denormalized fields in API mode — was not limited to quotes. **Every DTO type** with denormalized or computed fields had the same gap in the `ApiDataProvider` (C++) and the mobile `api.ts` (TypeScript). The `LocalDataProvider` was correct throughout because Wt::Dbo resolves relationships via ORM joins.

### Affected DTOs and Missing Fields

| DTO Type | Missing Fields |
|---|---|
| **QuoteLineItemDTO** | `productName`, `productUnit`, `supplierName` |
| **SupplierProductDTO** | `productName`, `productSku`, `productCategory`, `supplierName`, `supplierRating`, `supplierLeadDays`, `supplierLat`, `supplierLon` |
| **QuoteDTO** | `clientName`, `totalAmount`, `lineItemCount` |
| **ProductDTO** | `supplierCount`, `minPrice`, `maxPrice` |
| **SupplierDTO** | `productCount` |
| **ClientDTO** | `quoteCount` |

### Fix

Added 6 enrichment methods to both the C++ `ApiDataProvider` and the mobile TypeScript `api.ts`:

| Method | Resolves |
|---|---|
| `enrichProducts()` | Fetches all SupplierProducts, computes `supplierCount`, `minPrice`, `maxPrice` per product |
| `enrichSuppliers()` | Fetches all SupplierProducts, counts products per supplier |
| `enrichClients()` | Fetches all Quotes (raw), counts quotes per client |
| `enrichSupplierProducts()` | Fetches all Products + Suppliers, resolves 8 denormalized fields |
| `enrichQuotes()` | Fetches all Clients + LineItems (batch), resolves `clientName` and computes `totalAmount`/`lineItemCount` |
| `enrichLineItems()` | Fetches all Products + Suppliers, resolves `productName`, `productUnit`, `supplierName` |

#### Design Decisions

1. **Raw HTTP to avoid cascade**: Each enrichment method uses raw `httpGet()` (C++) or `apiGet()` (TS) calls with static parse functions, rather than calling the public `find*()` methods. This prevents enrichment methods from triggering other enrichment methods in a cascade (e.g., `enrichQuotes` fetching clients which would trigger `enrichClients` which fetches quotes, etc.).

2. **Batch fetching**: All enrichment methods fetch reference data in bulk (one HTTP call per entity type) and build in-memory lookup maps, avoiding N+1 query patterns. For example, `enrichQuotes()` fetches all line items in a single call and groups totals by `quoteId`, instead of making one call per quote.

3. **Forward declarations**: In the C++ implementation, static parse functions are forward-declared at the top of `ApiDataProvider.cpp` so that enrichment methods (defined between entity sections) can reference parse functions defined later in the file.

4. **Parallel fetches in mobile**: The TypeScript enrichment methods use `Promise.all()` where two independent API calls are needed (e.g., products + suppliers), reducing latency.

### Files Changed

**C++ Desktop App:**
- `include/data/ApiDataProvider.h` - Added 6 private enrichment method declarations
- `src/data/ApiDataProvider.cpp` - Implemented all enrichment methods; added forward declarations for static parse functions; updated all `find*()` methods to call enrichment before returning; refactored `enrichQuotes()` from N+1 to batch fetching

**React/Capacitor Mobile App:**
- `mobile/src/services/api.ts` - Added 6 async enrichment functions; updated all `fetch*()` functions to call enrichment; fixed `createQuote()` to always send `created_date`
