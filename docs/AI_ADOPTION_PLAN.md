# AI Adoption Plan: Intelligent Supplier Sourcing via MCP Services

## Executive Summary

This plan outlines the phased adoption of AI, geolocation intelligence, social media sentiment analysis, and web scraping capabilities into the Contractor Quotes & Sourcing application. The goal is to transform the "Find Best Sources" action from a static database lookup into a **live, AI-augmented supplier discovery and evaluation platform** for contractors in the building and remodeling markets.

**Architectural Foundation:** Each intelligence capability is implemented as an independent **MCP Server** (Model Context Protocol) exposing domain-specific tools over JSON-RPC 2.0. The Contractor Quotes application acts as the **MCP Host/Client**, and a new **Sourcing Orchestrator** coordinates tool discovery, asynchronous execution, token governance, and result aggregation across all registered MCP servers through governed, multi-threaded back channels.

This MCP-native architecture makes the intelligence layer **extensible by design** — new suppliers of intelligence (a new review platform, a new pricing feed, a regional supplier directory) are added by registering a new MCP server, not by modifying the host application.

---

## Current State Assessment

### What Exists Today

| Capability | Current Implementation |
|---|---|
| **Supplier Matching** | `SourcingEngine::findBestSources()` queries `SupplierProduct` table for known suppliers carrying a given product |
| **Pricing** | Static `unit_price` stored per supplier-product pair in database; bulk discount logic applied |
| **Geolocation** | Haversine distance calculation from job site (client lat/lon) to supplier lat/lon |
| **Quality Signal** | Single `rating` field (0.0–5.0) manually entered per supplier |
| **Composite Scoring** | Weighted formula: 40% price + 25% availability + 20% proximity + 15% quality |
| **Data Provider** | Abstract `DataProvider` interface with Local (Wt::Dbo) and API (REST) implementations |
| **Architecture** | Configurable via `app_config.yaml`; all views depend on abstract interfaces |

### What's Missing

1. **No supplier discovery** — the system only knows suppliers manually entered into the database
2. **No live pricing** — prices are snapshots that go stale immediately
3. **No reputation intelligence** — the rating is a single number with no provenance
4. **No real-time availability** — stock quantities are static database values
5. **No AI reasoning** — the scoring is a fixed linear formula with no learning or adaptation
6. **No service extensibility** — adding a new intelligence source requires modifying application code

---

## Architecture: MCP Service Mesh with Governed Orchestration

### Why MCP?

The **Model Context Protocol** (MCP) is an open standard introduced by Anthropic that standardizes how AI applications integrate with external tools and data sources. MCP uses **JSON-RPC 2.0** over pluggable transports (stdio, Server-Sent Events, Streamable HTTP) and defines a capability negotiation lifecycle where servers declare their tools, resources, and prompts at connection time.

For Contractor Quotes, MCP provides three critical properties:

1. **Extensibility** — New intelligence sources are added by registering a new MCP server. The host application discovers capabilities at runtime; no recompilation or redeployment required.
2. **Governance** — Each MCP server operates under token-based authorization (OAuth 2.1). The orchestrator controls concurrency, rate limits, and cost budgets per server per request.
3. **Async by design** — The MCP 2025-11-25 specification introduces **Tasks** — an async primitive where any request can return a task handle for "call-now, fetch-later" semantics, enabling true multi-threaded parallel execution across servers.

### Architectural Roles

| MCP Role | Component | Responsibility |
|---|---|---|
| **Host** | Contractor Quotes (Wt desktop, React mobile) | User-facing application; initiates sourcing requests; presents results |
| **Client** | Sourcing Orchestrator | Maintains connections to MCP servers; dispatches tool calls; governs concurrency and tokens; aggregates results |
| **Server** | Geo Discovery Server | Geolocation, geocoding, supplier discovery, drive time tools |
| **Server** | Market Pricing Server | Web scraping, price extraction, product matching, availability tools |
| **Server** | Reputation Server | Review aggregation, sentiment analysis, trust scoring tools |
| **Server** | AI Recommendation Server | Context-aware scoring, explanation generation, outcome learning tools |

### Master Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                       CONTRACTOR QUOTES HOST                             │
│                                                                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐  │
│  │ Wt Desktop App  │  │ React Mobile App│  │ ApiLogicServer          │  │
│  │ (C++ / Boost)   │  │ (TypeScript)    │  │ (Python / Flask)        │  │
│  └────────┬────────┘  └────────┬────────┘  └─────────────────────────┘  │
│           │                    │                                         │
│           └──────────┬─────────┘                                         │
│                      ▼                                                   │
│  ┌───────────────────────────────────────────────────────────────────┐   │
│  │                    SOURCING ORCHESTRATOR                           │   │
│  │                    (MCP Client Layer)                              │   │
│  │                                                                   │   │
│  │  ┌─────────────┐ ┌──────────────┐ ┌────────────┐ ┌────────────┐ │   │
│  │  │ Server      │ │ Token        │ │ Thread Pool│ │ Result     │ │   │
│  │  │ Registry &  │ │ Governance & │ │ Governor   │ │ Aggregator │ │   │
│  │  │ Discovery   │ │ OAuth 2.1    │ │ (bounded)  │ │ & Merger   │ │   │
│  │  └─────────────┘ └──────────────┘ └────────────┘ └────────────┘ │   │
│  │                                                                   │   │
│  │  ┌─────────────────────────────────────────────────────────────┐ │   │
│  │  │              Async Task Dispatcher                          │ │   │
│  │  │  • Fan-out tool calls to multiple MCP servers in parallel   │ │   │
│  │  │  • Collect results via MCP Task handles (call-now/fetch-    │ │   │
│  │  │    later)                                                   │ │   │
│  │  │  • Timeout & circuit-breaker per server                     │ │   │
│  │  │  • Progress reporting back to host                          │ │   │
│  │  └─────────────────────────────────────────────────────────────┘ │   │
│  └──────────┬──────────┬──────────────┬───────────────┬─────────────┘   │
│             │          │              │               │                   │
└─────────────┼──────────┼──────────────┼───────────────┼──────────────────┘
              │          │              │               │
    JSON-RPC/HTTP   JSON-RPC/HTTP  JSON-RPC/HTTP  JSON-RPC/HTTP
    + OAuth 2.1     + OAuth 2.1    + OAuth 2.1    + OAuth 2.1
              │          │              │               │
              ▼          ▼              ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│  MCP Server  │ │  MCP Server  │ │  MCP Server  │ │  MCP Server  │
│  ───────────── │  ───────────── │  ───────────── │  ─────────────│
│  GEO         │ │  MARKET      │ │  REPUTATION  │ │  AI          │
│  DISCOVERY   │ │  PRICING     │ │              │ │  RECOMMEND   │
│              │ │              │ │              │ │              │
│  Tools:      │ │  Tools:      │ │  Tools:      │ │  Tools:      │
│  • geocode   │ │  • scrape_   │ │  • aggregate_│ │  • score_    │
│  • reverse_  │ │    price     │ │    reviews   │ │    context   │
│    geocode   │ │  • match_    │ │  • analyze_  │ │  • explain_  │
│  • discover_ │ │    product   │ │    sentiment │ │    recommend │
│    suppliers │ │  • check_    │ │  • compute_  │ │  • learn_    │
│  • drive_    │ │    avail     │ │    trust     │ │    outcome   │
│    time      │ │  • market_   │ │  • summarize_│ │  • dynamic_  │
│  • radius_   │ │    price_    │ │    reputation│ │    weights   │
│    search    │ │    range     │ │  • flag_     │ │              │
│              │ │              │ │    anomalies │ │              │
├──────────────┤ ├──────────────┤ ├──────────────┤ ├──────────────┤
│  External:   │ │  External:   │ │  External:   │ │  External:   │
│  Google Maps │ │  Playwright  │ │  Google Revs │ │  Claude API  │
│  OSM/OSRM   │ │  Supplier    │ │  Yelp API    │ │  (Anthropic) │
│  Yelp Fusion │ │  websites    │ │  BBB scrape  │ │              │
│  Places API  │ │  Distributor │ │  Angi scrape │ │              │
│              │ │  platforms   │ │  Facebook    │ │              │
└──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
```

### MCP Message Flow: "Find Best Sources" Enhanced

When a contractor clicks "Find Best Sources" for a line item, the following orchestrated flow executes:

```
1. HOST: User clicks "Find Best Sources" for Product #12 (2x4 Lumber), Qty 200
          Job site: Austin, TX (30.267, -97.743)

2. ORCHESTRATOR: Receives sourcing request
   ├── Checks Server Registry for available MCP servers
   ├── Validates OAuth 2.1 tokens for each server (refresh if expired)
   ├── Allocates thread pool slots (governed concurrency)
   └── Fans out parallel tool calls:

3. PARALLEL DISPATCH (async, governed):
   │
   ├── Thread 1 → GEO DISCOVERY SERVER
   │   ├── tools/call: discover_suppliers("lumber yard", 30.267, -97.743, 50mi)
   │   └── tools/call: drive_time(jobSite → each discovered supplier)
   │   └── Returns: Task handle → polls for completion
   │
   ├── Thread 2 → MARKET PRICING SERVER
   │   ├── tools/call: scrape_price("2x4 Stud Grade Lumber", [Home Depot, Lowe's, ...])
   │   ├── tools/call: match_product("2x4 SPF Stud 8ft", product_catalog)
   │   └── tools/call: market_price_range(productId=12, region="Austin TX")
   │   └── Returns: Task handle → polls for completion
   │
   ├── Thread 3 → REPUTATION SERVER
   │   ├── tools/call: aggregate_reviews("Texas Building Supply", "Austin", "TX")
   │   ├── tools/call: analyze_sentiment(reviews, themes=contractor_specific)
   │   └── tools/call: compute_trust_score(aggregated_data)
   │   └── Returns: Task handle → polls for completion
   │
   └── Thread 4 → AI RECOMMENDATION SERVER
       └── (awaits results from Threads 1-3, then)
       ├── tools/call: score_context(quote_context, supplier_data, market_data, reputation_data)
       └── tools/call: explain_recommendation(top_5_results, job_context)

4. ORCHESTRATOR: Aggregates results
   ├── Merges discovered suppliers with database suppliers (deduplication)
   ├── Overlays live pricing onto static database prices
   ├── Replaces single rating with composite trust score
   ├── Applies AI-generated scoring weights
   └── Returns ranked results + natural-language explanation to Host

5. HOST: Displays enhanced SourcingResults dialog
   ├── Ranked supplier table with live prices, trust scores, drive times
   ├── "Market Intelligence" sidebar with price ranges and trends
   ├── AI explanation: "Recommended Texas Building Supply because..."
   └── Reputation summary with review highlights and red flags
```

### Token Governance Model

Every MCP tool call is governed by a token-based authorization and budget system:

```
┌─────────────────────────────────────────────────────────┐
│                   TOKEN GOVERNOR                         │
│                                                         │
│  ┌──────────────────────┐  ┌─────────────────────────┐ │
│  │  OAuth 2.1 Tokens    │  │  Usage Budget Tokens     │ │
│  │                      │  │                          │ │
│  │  • Per-server access │  │  • API call quotas       │ │
│  │    tokens            │  │    per server per hour   │ │
│  │  • Refresh lifecycle │  │  • LLM token budget      │ │
│  │  • Scope-limited     │  │    per sourcing request  │ │
│  │    (read-only for    │  │  • Scraping page budget  │ │
│  │    scraping servers; │  │    per session           │ │
│  │    read+write for    │  │  • Cost ceiling per      │ │
│  │    recommendation    │  │    contractor per month  │ │
│  │    server)           │  │                          │ │
│  └──────────────────────┘  └─────────────────────────┘ │
│                                                         │
│  ┌──────────────────────┐  ┌─────────────────────────┐ │
│  │  Concurrency Tokens  │  │  Circuit Breakers       │ │
│  │                      │  │                          │ │
│  │  • Max parallel      │  │  • Per-server failure    │ │
│  │    threads per       │  │    threshold             │ │
│  │    sourcing request  │  │  • Automatic fallback    │ │
│  │  • Max outstanding   │  │    to cached data        │ │
│  │    MCP Tasks         │  │  • Exponential backoff   │ │
│  │  • Priority queue    │  │    on repeated failures  │ │
│  │    for user-facing   │  │  • Health check ping     │ │
│  │    vs. background    │  │    interval              │ │
│  └──────────────────────┘  └─────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### Extensibility: Adding a New MCP Server

Adding a new intelligence source requires **zero changes** to the host application:

```
Step 1:  Build a new MCP server that exposes tools via JSON-RPC 2.0
         Example: "regional-supplier-directory-server" with tools:
           • search_regional_directory(region, product_category)
           • get_supplier_certifications(supplier_name)

Step 2:  Register the server in mcp_servers.yaml:
           regional_directory:
             transport: http
             url: "http://localhost:3104"
             auth:
               type: oauth2
               token_url: "http://auth.internal/token"
               client_id: "regional-dir"
               scopes: ["read:suppliers"]
             governance:
               max_concurrent: 2
               timeout_seconds: 30
               budget_calls_per_hour: 100

Step 3:  Orchestrator discovers new server at next initialization
         → Calls initialize → reads tools/list → registers tools
         → New tools automatically available in sourcing pipeline
```

---

## Phase 0: MCP Foundation — Orchestrator & Server Framework

**Objective:** Build the MCP Client (Orchestrator) inside Contractor Quotes and establish the server scaffolding that all subsequent phases will deploy into.

### 0.1 Sourcing Orchestrator (MCP Client)

The orchestrator is the central nervous system connecting the host application to MCP servers.

**Core Responsibilities:**
- **Server Registry:** Load MCP server configurations from `mcp_servers.yaml`; maintain connection state per server
- **Capability Discovery:** On startup (and periodically), call `initialize` → `tools/list` on each registered server to discover available tools and their input schemas
- **Token Management:** Manage OAuth 2.1 token lifecycle per server — acquire, cache, refresh, revoke
- **Async Task Dispatch:** Fan out tool calls to multiple servers in parallel using MCP's Task primitive for non-blocking execution
- **Thread Pool Governance:** Bounded thread pool with configurable max concurrency; priority queue for user-interactive vs. background refresh requests
- **Result Aggregation:** Collect and merge results from all servers into a unified `EnhancedSourcingResult`
- **Circuit Breaking:** Per-server failure tracking with automatic fallback to cached data after threshold breaches
- **Progress Reporting:** Stream progress updates back to the host UI ("Discovering suppliers... Checking prices... Analyzing reviews...")

**Implementation:**
- New C++ class: `SourcingOrchestrator` implementing MCP Client protocol over Streamable HTTP transport
- New TypeScript class: `McpOrchestrator` for the mobile app (mirrors C++ implementation)
- Uses Boost.Asio (C++) / `fetch` + `AbortController` (TypeScript) for async HTTP
- Thread pool via `std::thread` pool with `std::counting_semaphore` for governance

```cpp
class SourcingOrchestrator {
public:
    explicit SourcingOrchestrator(const McpConfig& config);

    // Initialize connections to all registered MCP servers
    void initialize();

    // Enhanced sourcing: fans out to all available MCP servers
    EnhancedSourcingResult findBestSourcesEnhanced(
        long long productId,
        int quantity,
        double jobLat,
        double jobLon,
        const QuoteDTO& quoteContext,
        ProgressCallback onProgress = nullptr
    );

    // Server management
    std::vector<McpServerInfo> listServers() const;
    McpServerHealth getServerHealth(const std::string& serverId) const;
    void refreshServerCapabilities(const std::string& serverId);

private:
    McpConfig config_;
    std::map<std::string, McpServerConnection> servers_;
    TokenGovernor tokenGovernor_;
    ThreadPoolGovernor threadPool_;
    ResultAggregator aggregator_;
    CircuitBreakerRegistry circuitBreakers_;
};
```

### 0.2 MCP Server Scaffold

Provide a reusable Python scaffold for building MCP servers using the official Python SDK (`mcp` package):

```python
# mcp-servers/base_server.py — shared scaffold for all intelligence servers

from mcp.server import Server
from mcp.server.stdio import stdio_server        # for local dev
from mcp.server.sse import SseServerTransport    # for production HTTP
import mcp.types as types

class IntelligenceServer:
    """Base class for Contractor Quotes MCP intelligence servers."""

    def __init__(self, name: str, version: str):
        self.server = Server(name)
        self.version = version
        self._register_base_handlers()

    def _register_base_handlers(self):
        @self.server.list_tools()
        async def list_tools() -> list[types.Tool]:
            return self.get_tools()

        @self.server.call_tool()
        async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
            return await self.handle_tool_call(name, arguments)

    def get_tools(self) -> list[types.Tool]:
        """Override in subclass to declare available tools."""
        raise NotImplementedError

    async def handle_tool_call(self, name: str, arguments: dict):
        """Override in subclass to handle tool invocations."""
        raise NotImplementedError
```

### 0.3 Configuration: `mcp_servers.yaml`

A new configuration file for MCP server registration, separate from `app_config.yaml`:

```yaml
# mcp_servers.yaml — MCP Server Registry for Contractor Quotes
#
# Each entry defines an MCP server that the Sourcing Orchestrator
# connects to. Servers are discovered and initialized at startup.

orchestrator:
  # Thread pool governance
  max_concurrent_threads: 8           # max parallel MCP tool calls
  max_outstanding_tasks: 20           # max async MCP Tasks in flight
  task_poll_interval_ms: 500          # how often to poll Task handles
  default_timeout_seconds: 30         # per-tool-call timeout
  progress_reporting: true            # stream progress to host UI

  # Cost governance
  monthly_budget_ceiling_usd: 50.00   # hard stop on API costs
  llm_token_budget_per_request: 8000  # max LLM tokens per sourcing request

servers:
  geo_discovery:
    name: "Geo Discovery Server"
    transport: http                   # stdio | sse | http
    url: "http://localhost:3101"
    enabled: true
    auth:
      type: oauth2
      token_url: "http://auth.internal/oauth/token"
      client_id: "geo-discovery"
      client_secret_env: "GEO_DISCOVERY_SECRET"   # read from env var
      scopes: ["read:geo", "read:places"]
    governance:
      max_concurrent: 3
      timeout_seconds: 15
      budget_calls_per_hour: 500
      circuit_breaker:
        failure_threshold: 5
        recovery_timeout_seconds: 60
    config:                           # server-specific config passed at init
      google_api_key_env: "GOOGLE_MAPS_API_KEY"
      default_radius_miles: 50
      enable_drive_time: true

  market_pricing:
    name: "Market Pricing Server"
    transport: http
    url: "http://localhost:3102"
    enabled: true
    auth:
      type: oauth2
      token_url: "http://auth.internal/oauth/token"
      client_id: "market-pricing"
      client_secret_env: "MARKET_PRICING_SECRET"
      scopes: ["read:pricing", "execute:scrape"]
    governance:
      max_concurrent: 2               # lower: scraping is resource-intensive
      timeout_seconds: 45             # higher: scraping takes longer
      budget_calls_per_hour: 200
      circuit_breaker:
        failure_threshold: 3
        recovery_timeout_seconds: 120
    config:
      cache_ttl_hours: 24
      respect_robots_txt: true
      headless_browser: "playwright"

  reputation:
    name: "Reputation Server"
    transport: http
    url: "http://localhost:3103"
    enabled: true
    auth:
      type: oauth2
      token_url: "http://auth.internal/oauth/token"
      client_id: "reputation"
      client_secret_env: "REPUTATION_SECRET"
      scopes: ["read:reviews", "execute:sentiment"]
    governance:
      max_concurrent: 3
      timeout_seconds: 30
      budget_calls_per_hour: 300
      circuit_breaker:
        failure_threshold: 5
        recovery_timeout_seconds: 60
    config:
      platforms: ["google_reviews", "yelp", "bbb"]
      min_reviews_for_score: 3
      cache_ttl_hours: 168            # weekly refresh
      claude_model: "claude-haiku-4-5-20251001"   # cost-efficient for classification

  ai_recommendation:
    name: "AI Recommendation Server"
    transport: http
    url: "http://localhost:3104"
    enabled: true
    auth:
      type: oauth2
      token_url: "http://auth.internal/oauth/token"
      client_id: "ai-recommend"
      client_secret_env: "AI_RECOMMEND_SECRET"
      scopes: ["read:context", "write:outcomes"]
    governance:
      max_concurrent: 2
      timeout_seconds: 30
      budget_calls_per_hour: 100
      circuit_breaker:
        failure_threshold: 3
        recovery_timeout_seconds: 60
    config:
      claude_model: "claude-sonnet-4-6"     # stronger model for reasoning
      max_tokens: 4096
      explain_recommendations: true
      learn_from_outcomes: true
```

### 0.4 Extended `app_config.yaml`

Add MCP orchestrator reference to the existing config:

```yaml
# ── MCP Orchestrator ──────────────────────────────────
mcp:
  enabled: true
  servers_config: "model/mcp_servers.yaml"    # path to server registry
  auth:
    provider: "internal"                       # internal | auth0 | okta
    token_url: "http://auth.internal/oauth/token"
    token_cache_path: ".mcp_tokens"
```

### Deliverables — Phase 0
- [ ] `SourcingOrchestrator` C++ class with MCP Client protocol over Streamable HTTP
- [ ] `McpOrchestrator` TypeScript class for mobile app
- [ ] `TokenGovernor` with OAuth 2.1 lifecycle management
- [ ] `ThreadPoolGovernor` with bounded concurrency and priority queue
- [ ] `CircuitBreakerRegistry` with per-server failure tracking
- [ ] `ResultAggregator` for merging multi-server results
- [ ] Python MCP server scaffold (`IntelligenceServer` base class)
- [ ] `mcp_servers.yaml` configuration schema and loader
- [ ] Integration test harness with mock MCP servers
- [ ] Docker Compose for local development with all services

---

## Phase 1: Geo Discovery MCP Server

**Objective:** Find new suppliers near the job site that aren't in the database yet.

### MCP Server: `geo-discovery-server`

**Transport:** Streamable HTTP on port 3101
**SDK:** Python (`mcp` package + `httpx` for external API calls)

### Exposed Tools

| Tool Name | Input Schema | Output | Description |
|---|---|---|---|
| `geocode` | `{address: string}` | `{lat, lon, formatted_address}` | Convert street address to coordinates |
| `reverse_geocode` | `{lat: number, lon: number}` | `{address, city, state, zip}` | Convert coordinates to address |
| `discover_suppliers` | `{category: string, lat: number, lon: number, radius_miles: number}` | `[{name, address, lat, lon, phone, rating, place_id, review_count, source}]` | Find building material suppliers via Places API and Yelp |
| `drive_time` | `{from_lat, from_lon, to_lat, to_lon: number}` | `{distance_miles, drive_minutes, route_summary}` | Compute actual driving distance and time |
| `radius_search` | `{lat, lon: number, radius_miles: number, query: string}` | `[{name, lat, lon, distance_miles}]` | General-purpose radius search for any business type |

### 1.1 Geocoding Service

- **Primary provider:** Google Maps Geocoding API
- **Fallback provider:** OpenStreetMap Nominatim (free, no API key)
- **Auto-geocoding:** When a `ClientDTO` or `SupplierDTO` has an address but no lat/lon, the orchestrator calls `geocode` before dispatching other tools
- **Result caching:** Geocoding results cached in Redis with 30-day TTL (addresses rarely change)

### 1.2 Supplier Discovery via Places API

- **Search query construction:** Derive queries from product categories in the database — "building materials supplier", "lumber yard", "concrete supplier", "roofing supply"
- **Multi-source discovery:** Query both Google Places API and Yelp Fusion API; deduplicate by name + proximity (within 200m = same business)
- **Result mapping:** Map discovered businesses to `DiscoveredSupplierDTO` with fields: `name`, `address`, `lat`, `lon`, `phone`, `rating`, `reviewCount`, `placeId`, `businessHours`, `photoUrl`, `source`

### 1.3 Drive Time & Route Distance

- **Routing API:** Google Directions API (primary) or OSRM (self-hosted fallback)
- **Scoring integration:** The orchestrator passes `drive_minutes` to the AI Recommendation Server, which factors in actual drive time rather than straight-line distance
- **Delivery zone detection:** If a supplier's website advertises a delivery radius, the discovery server flags this as an availability enhancement

### Deliverables — Phase 1
- [ ] `geo-discovery-server` Python MCP server with 5 tools
- [ ] Google Maps + Yelp Fusion API integration
- [ ] OSRM fallback for self-hosted routing
- [ ] Deduplication logic (name similarity + proximity)
- [ ] Redis caching layer for geocoding results
- [ ] Unit tests for all tools
- [ ] Docker image with health check endpoint

---

## Phase 2: Market Pricing MCP Server

**Objective:** Validate and update supplier pricing in real-time by scraping supplier websites and major distributor platforms.

### MCP Server: `market-pricing-server`

**Transport:** Streamable HTTP on port 3102
**SDK:** Python (`mcp` package + Playwright for browser automation)

### Exposed Tools

| Tool Name | Input Schema | Output | Description |
|---|---|---|---|
| `scrape_price` | `{supplier_url: string, product_sku: string, product_name: string}` | `{price, in_stock, last_checked, source_url}` | Scrape a specific supplier page for price and availability |
| `match_product` | `{scraped_name: string, scraped_desc: string, scraped_unit: string}` | `{product_id, confidence, matched_name}` | Match a scraped listing to a product in the database |
| `check_availability` | `{supplier_name: string, product_sku: string, zip_code: string}` | `{in_stock: bool, qty_available, lead_time_days, last_checked}` | Check real-time stock for a specific product at a specific supplier |
| `market_price_range` | `{product_id: number, region: string}` | `{min, median, max, avg, source_count, prices: [{source, price, date}]}` | Aggregate pricing across multiple sources for market positioning |

### 2.1 Scraping Infrastructure

- **Browser engine:** Playwright (Python) for JavaScript-rendered pages
- **Static scraper:** `httpx` for simple HTML pages (faster, cheaper)
- **Rate governance:** The MCP server enforces `robots.txt` compliance and configurable delays between requests — these limits are internal to the server, separate from the orchestrator's external governance
- **Cache layer:** Redis-backed cache with configurable TTL per data type:
  - Pricing: 24-hour TTL (prices change daily)
  - Stock status: 4-hour TTL (stock changes frequently)
  - Product catalog: 7-day TTL (catalogs change weekly)
- **Proxy support:** Optional proxy rotation via environment variable for high-volume scraping

### 2.2 Target Sources

| Source Type | Examples | Data Available |
|---|---|---|
| **Supplier websites** | Individual supplier catalog pages | Pricing, stock status, MOQ |
| **Distributor platforms** | Home Depot Pro, Lowe's for Pros, ABC Supply | Pricing, availability, bulk pricing tiers |
| **Manufacturer catalogs** | James Hardie, Owens Corning, LP Building Solutions | MSRP, spec sheets, dealer locators |
| **Price aggregators** | BuildingConnected, SmartBid | Competitive bid data |
| **Industry marketplaces** | BuildDirect, Materials Market | Market pricing signals |

### 2.3 Product Matching & SKU Resolution

- **SKU matching:** Direct lookup when supplier uses standard SKUs
- **Fuzzy name matching:** Levenshtein distance + TF-IDF for product name similarity
- **LLM-assisted matching:** For ambiguous matches, the tool calls Claude API (via the AI Recommendation Server's `match_product` capability) with product description + scraped listing for classification confidence
- **Unit normalization:** Convert between "per board foot", "per linear foot", "per piece", "per bundle", "per square"

### 2.4 Price Intelligence in UI

- **Market price range:** Show min/median/max across all discovered sources
- **Price trend indicator:** Arrow up/down/stable based on price change from last scrape
- **Freshness badge:** "Live" / "24h ago" / "Stale" indicator on each price
- **Alert system:** Contractor can set price-drop alerts on tracked products

### Deliverables — Phase 2
- [ ] `market-pricing-server` Python MCP server with 4 tools
- [ ] Playwright scraping engine with configurable site adapters
- [ ] Product matching pipeline (SKU + fuzzy + LLM)
- [ ] Redis caching with per-data-type TTL
- [ ] Scrape adapters for 3+ distributor platforms
- [ ] Market Intelligence panel in desktop SourcingResults dialog
- [ ] Mobile market pricing view on product detail
- [ ] Docker image with Playwright pre-installed

---

## Phase 3: Reputation MCP Server

**Objective:** Replace the single `rating` field with a rich, AI-analyzed reputation score drawn from multiple review platforms.

### MCP Server: `reputation-server`

**Transport:** Streamable HTTP on port 3103
**SDK:** Python (`mcp` package + `anthropic` SDK for sentiment analysis)

### Exposed Tools

| Tool Name | Input Schema | Output | Description |
|---|---|---|---|
| `aggregate_reviews` | `{supplier_name: string, city: string, state: string}` | `{reviews: [{platform, rating, text, date, reviewer}], total_count}` | Collect reviews from all configured platforms |
| `analyze_sentiment` | `{reviews: array, themes: string[]}` | `{sentiments: [{review_id, sentiment, confidence, themes: []}]}` | AI sentiment classification + theme extraction |
| `compute_trust_score` | `{aggregated_data: object}` | `{trust_score, confidence_interval, breakdown: {sentiment, recency, volume, consistency, diversity}}` | Compute composite trust score from analyzed reviews |
| `summarize_reputation` | `{supplier_name: string, analyzed_data: object}` | `{summary: string, highlights: string[], red_flags: string[]}` | Generate natural-language reputation summary |
| `flag_anomalies` | `{review_history: array}` | `{anomalies: [{type, description, severity}]}` | Detect sudden rating drops, suspicious review patterns |

### 3.1 Review Aggregation

| Platform | Data Available | Access Method |
|---|---|---|
| **Google Reviews** | Star rating, review text, reviewer info | Places API |
| **Yelp** | Star rating, review text, response history | Yelp Fusion API |
| **BBB** | Accreditation, complaints, resolution rate | Web scraping |
| **Angi** | Grade, review text, hire rate | Web scraping |
| **Facebook Business** | Star rating, recommendations | Graph API |
| **Houzz** | Pro reviews, project photos | Web scraping |

### 3.2 AI Sentiment Analysis Pipeline

```
Raw Reviews → Preprocessing → Sentiment Analysis → Theme Extraction → Trust Score
```

**Preprocessing:**
- Spam/fake review filtering (LLM classifier)
- Cross-platform rating normalization (Yelp 5-star → 0-1, BBB A-F → 0-1)
- Recency weighting (exponential decay, half-life = 6 months)

**Sentiment Classification (Claude API via Haiku for cost efficiency):**
- Classify: Positive / Neutral / Negative / Mixed
- Confidence score per classification

**Contractor-Specific Theme Extraction:**
- **Delivery reliability:** "delivered on time" / "always late"
- **Pricing accuracy:** "quote matched final bill" / "hidden charges"
- **Product quality:** "materials were as described" / "substituted inferior"
- **Communication:** "responsive" / "hard to reach"
- **Problem resolution:** "fixed the issue" / "ghosted me"
- **Contractor-specific needs:** "understands commercial jobs" / "residential only"

**Composite Trust Score:**
```
trust_score = 0.30 * sentiment_normalized
            + 0.20 * recency_weighted_avg
            + 0.20 * review_volume_normalized
            + 0.15 * theme_consistency
            + 0.15 * platform_diversity_bonus
```
- Confidence interval: "4.2 ± 0.3 (based on 47 reviews across 4 platforms)"
- Anomaly flags: sudden drops, suspicious patterns, unresolved BBB complaints

### 3.3 Reputation Dashboard

- **Trust Score card:** Replaces single `rating` in the SourcingResults dialog
- **Review summary:** AI-generated 2-3 sentence summary of contractor feedback
- **Red flags:** Prominent warnings for BBB complaints, recent negative trends
- **Source breakdown:** How many reviews from each platform contributed to the score

### Deliverables — Phase 3
- [ ] `reputation-server` Python MCP server with 5 tools
- [ ] Review aggregation from Google, Yelp, BBB (minimum 3 platforms)
- [ ] Claude API sentiment pipeline (Haiku for classification, Sonnet for summaries)
- [ ] Contractor-specific theme taxonomy and extraction
- [ ] Trust score algorithm with confidence intervals
- [ ] Reputation panel in desktop SourcingResults dialog
- [ ] Mobile reputation view on supplier detail screen
- [ ] Weekly background refresh via orchestrator scheduled tasks
- [ ] Docker image

---

## Phase 4: AI Recommendation MCP Server

**Objective:** Replace the fixed linear scoring formula with an AI agent that reasons about the best supplier for each specific job context.

### MCP Server: `ai-recommendation-server`

**Transport:** Streamable HTTP on port 3104
**SDK:** Python (`mcp` package + `anthropic` SDK for Claude reasoning)

### Exposed Tools

| Tool Name | Input Schema | Output | Description |
|---|---|---|---|
| `score_context` | `{quote: object, suppliers: array, market_data: object, reputation_data: object}` | `{weights: {price, avail, proximity, quality}, reasoning: string}` | Generate context-aware scoring weights |
| `explain_recommendation` | `{top_results: array, job_context: object}` | `{explanation: string, comparison_notes: string[]}` | Natural-language explanation of why #1 is recommended |
| `learn_outcome` | `{line_item_id: number, selected_supplier_id: number, outcome_rating: number, notes: string}` | `{acknowledged: bool}` | Record contractor's actual decision and outcome |
| `dynamic_weights` | `{job_type: string, budget_level: string, timeline: string, history: array}` | `{weights: object, confidence: number}` | Generate weights from job characteristics without full context |

### 4.1 Context-Aware Scoring

The current formula applies identical weights to every sourcing decision. The AI Recommendation Server considers **context**:

- **Job type:** A kitchen remodel values speed; new construction values bulk pricing
- **Budget sensitivity:** Price-driven clients vs. quality-driven clients
- **Seasonal factors:** Roofing materials scarce after storm seasons; lumber pricing volatile in spring
- **Relationship history:** Prefer suppliers the contractor has worked with successfully before
- **Risk tolerance:** Commercial jobs → reliability over price; residential renovations → price matters more

**Claude API prompt structure:**
```
Given this sourcing context:
- Quote: "{title}" for client "{clientName}" in {city}, {state}
- Description: "{description}"
- Product: {productName} ({category}), Quantity: {quantity}
- Contractor's past supplier preferences: [history]
- Current market conditions: [market_data summary]

Recommend scoring weights as JSON:
{priceWeight, availWeight, proximityWeight, qualityWeight}
And explain your reasoning in 2-3 sentences.
```

### 4.2 Explainable Recommendations

Every sourcing recommendation includes a natural-language explanation:

> "Recommended **Texas Building Supply** for 2x4 Stud Grade Lumber (qty 200). They're 12 miles from the job site with a 18-minute drive, currently have 500+ in stock at $3.42/unit (15% below market average), and have a 4.6 trust score across 89 reviews. Their delivery reliability is rated 'Excellent' by contractors. Note: ABC Materials is $0.18/unit cheaper but has a 2-week lead time that may impact your project timeline."

### 4.3 Learning from Outcomes

Track which sourcing decisions contractors accept and whether outcomes were positive:

- **Acceptance tracking:** Record which supplier was selected for each line item
- **Outcome feedback:** After quote completion, prompt "How did this supplier perform?"
- **Preference enrichment:** Include past outcomes in future recommendation prompts
- **New database table:** `sourcing_outcome` with `{line_item_id, selected_supplier_id, outcome_rating, notes, timestamp}`

### Deliverables — Phase 4
- [ ] `ai-recommendation-server` Python MCP server with 4 tools
- [ ] Context-aware weight generation via Claude Sonnet
- [ ] Natural-language recommendation explanations
- [ ] Outcome tracking schema and UI (desktop + mobile)
- [ ] Preference learning prompt enrichment
- [ ] A/B testing: AI recommendations vs. fixed formula
- [ ] Docker image

---

## Implementation Priority & Dependencies

```
Phase 0: MCP Foundation           ← MUST BE FIRST
(Weeks 1–3)
├── Orchestrator, Token Governor, Thread Pool, Server Scaffold
│
├──────────────────┬──────────────────┬──────────────────────┐
▼                  ▼                  ▼                      │
Phase 1:        Phase 2:           Phase 3:                  │
Geo Discovery   Market Pricing     Reputation                │
MCP Server      MCP Server         MCP Server                │
(Weeks 3–5)     (Weeks 3–6)        (Weeks 3–6)              │
│                  │                  │                       │
└──────────────────┴──────────────────┘                      │
                   │                                         │
                   ▼                                         │
            Phase 4:                                         │
            AI Recommendation                                │
            MCP Server                                       │
            (Weeks 6–9)                                      │
                   │                                         │
                   ▼                                         │
            Integration Testing & Production Deployment      │
            (Weeks 9–11)  ◄──────────────────────────────────┘
```

**Phase 0** is the foundation — the orchestrator, governance, and server scaffold must exist before any MCP server can be connected.

**Phases 1, 2, and 3** are independent MCP servers that can be developed in parallel by separate teams. Each delivers standalone value the moment it's registered with the orchestrator.

**Phase 4** is the intelligence layer that consumes data from all other servers. It can be started early (the `dynamic_weights` tool works without live data), but `explain_recommendation` reaches full capability only when geo, pricing, and reputation data are flowing.

---

## Risk Factors & Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| **API rate limits** (Google, Yelp) | Discovery and review collection throttled | MCP governance: per-server `budget_calls_per_hour` limits; Redis caching reduces repeat calls |
| **Scraping legal/TOS** | Supplier websites may block scraping | Scrape only public data; `robots.txt` compliance enforced by market pricing server; offer opt-in data partnerships |
| **LLM hallucination** | AI generates incorrect pricing or false claims | Always show data provenance; MCP tools return `source` and `last_checked` with every data point; never present AI output without attribution |
| **Cost of AI API calls** | Claude API costs scale with usage | Token Governor enforces `monthly_budget_ceiling_usd` and `llm_token_budget_per_request`; use Haiku for classification, Sonnet for reasoning |
| **MCP server failure** | One server goes down, blocks sourcing | Circuit breaker per server; graceful degradation returns partial results from healthy servers + cached data from failed ones |
| **Stale data** | Users trust "live" data that's hours old | Every tool result includes `last_checked` timestamp; UI shows freshness badges; orchestrator rejects data older than TTL |
| **Token leakage** | OAuth credentials exposed | Secrets via environment variables only (`_env` suffix in config); tokens never logged; short-lived access tokens with refresh rotation |
| **Thread exhaustion** | Too many concurrent sourcing requests | `ThreadPoolGovernor` with bounded pool; priority queue favoring interactive requests; queued overflow with user notification |

---

## Success Metrics

| Metric | Current | Phase 0+1 Target | Phase 4 Target |
|---|---|---|---|
| Suppliers evaluated per sourcing request | 8 (database only) | 20+ (discovered + database) | 30+ (multi-source) |
| Price data freshness | Static (days/weeks old) | Live (within 24 hours) | Real-time with caching |
| Quality signal depth | 1 field (rating) | 3 fields (rating + distance + drive time) | Full reputation report |
| Recommendation explainability | None (numeric score only) | Scoring breakdown | Natural-language explanation |
| MCP server response time (p95) | N/A | < 3 seconds (geo) | < 10 seconds (full pipeline) |
| Contractor confidence in sourcing | Not measured | Survey baseline | 4.0+/5.0 satisfaction |
| New intelligence sources added | 0 per year | N/A | 2+ per quarter (plug-in MCP servers) |

---

## Technology Stack

| Component | Technology | Rationale |
|---|---|---|
| **MCP Client / Orchestrator** | C++ (Boost.Asio) + TypeScript (fetch) | Embedded in existing host apps; async HTTP for MCP transport |
| **MCP Server Framework** | Python + `mcp` SDK | Official MCP Python SDK; rich AI/ML ecosystem; async-native |
| **MCP Transport** | Streamable HTTP (JSON-RPC 2.0) | Production-grade; supports authentication headers; load-balanceable |
| **Authentication** | OAuth 2.1 (per MCP 2025 spec) | MCP-native; servers are OAuth Resource Servers; token rotation built-in |
| **AI/LLM** | Claude API (Haiku for classification, Sonnet for reasoning) | Structured output for tools; strong reasoning for recommendations |
| **Browser Automation** | Playwright (Python) | Best-in-class for JS-rendered supplier sites; MCP server-internal |
| **Geocoding** | Google Maps Platform / OSM Nominatim | Industry standard; Nominatim for cost-free fallback |
| **Caching** | Redis | Fast TTL-based caching; shared across MCP servers |
| **Thread Governance** | `std::counting_semaphore` (C++) / `p-limit` (TS) | Bounded concurrency without external dependencies |
| **Container Orchestration** | Docker Compose → Kubernetes | Compose for dev; Kubernetes for production (one pod per MCP server) |
| **Server Registry** | YAML config → MCP Registry (future) | Start with static YAML; migrate to MCP Registry when available for org |

---

## Conclusion

This plan transforms the Contractor Quotes application from a **static quote builder** into an **intelligent sourcing platform** built on an **extensible MCP service mesh**. Each intelligence capability is an independent, token-governed MCP server that the Sourcing Orchestrator discovers, authenticates, and invokes through multi-threaded, asynchronous back channels.

The architecture delivers four properties that a monolithic approach cannot:

1. **Extensibility** — Adding a new intelligence source (a regional supplier directory, a new review platform, a manufacturer price feed) means deploying a new MCP server and adding 10 lines to `mcp_servers.yaml`. Zero changes to the host application.

2. **Governance** — Every tool call is authorized via OAuth 2.1, rate-limited via budget tokens, and circuit-broken on failure. The Thread Pool Governor ensures no single sourcing request starves the system.

3. **Resilience** — If the Reputation Server is down, the contractor still gets geo-discovered suppliers with live pricing. The orchestrator degrades gracefully, returning partial results from healthy servers augmented with cached data from failed ones.

4. **Composability** — The AI Recommendation Server doesn't scrape websites or aggregate reviews. It receives structured data from other MCP servers and reasons about it. Each server does one thing well, and the orchestrator composes them into a complete intelligence picture.

The DDD foundation already in place — clean entity models, abstract `DataProvider` interface, separation of domain from infrastructure — maps naturally onto the MCP architecture. Each MCP server is a bounded context with its own domain, its own data sources, and its own deployment lifecycle. The Sourcing Orchestrator is the anti-corruption layer that translates between the MCP tool protocol and the existing `SourcingEngine` domain model.

This is the same architectural discipline that made the first 9 phases successful — now expressed as a governed, distributed service mesh.

---

*Prepared for the Contractor Quotes & Sourcing application — extending the existing 9-phase development with MCP-native AI-augmented supplier intelligence.*
