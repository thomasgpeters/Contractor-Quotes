# AI Adoption Plan: Intelligent Supplier Sourcing

## Executive Summary

This plan outlines the phased adoption of AI, geolocation intelligence, social media sentiment analysis, and web scraping capabilities into the Contractor Quotes & Sourcing application. The goal is to transform the "Find Best Sources" action from a static database lookup into a **live, AI-augmented supplier discovery and evaluation platform** for contractors in the building and remodeling markets.

The current `SourcingEngine` ranks suppliers by a composite of price, availability, proximity, and quality — but only from **known suppliers already in the database**. The AI-enhanced sourcing engine will discover new suppliers, validate real-time pricing, gather reputation signals from the open web, and present contractors with a complete market picture before they commit to a purchase.

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

---

## Architecture Principle: Extend, Don't Replace

The existing `DataProvider → SourcingEngine → Views` pipeline is well-designed. The AI adoption plan **extends this pipeline** with new data sources rather than replacing it:

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SOURCING ENGINE v2                           │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │
│  │  DataProvider │  │  AI Discovery│  │  Live Market Intelligence│  │
│  │  (existing)   │  │  Service     │  │  Service                 │  │
│  │              │  │              │  │                          │  │
│  │  • Known     │  │  • Web search│  │  • Price scraping        │  │
│  │    suppliers │  │  • LLM parse │  │  • Stock checking        │  │
│  │  • DB prices │  │  • Geo lookup│  │  • Social sentiment      │  │
│  │  • Ratings   │  │  • Classify  │  │  • Review aggregation    │  │
│  └──────┬───────┘  └──────┬───────┘  └────────────┬─────────────┘  │
│         │                 │                        │                │
│         └─────────────────┼────────────────────────┘                │
│                           ▼                                         │
│                 ┌──────────────────┐                                │
│                 │  AI Scoring &    │                                │
│                 │  Recommendation  │                                │
│                 │  Engine          │                                │
│                 └────────┬─────────┘                                │
│                          ▼                                          │
│                 ┌──────────────────┐                                │
│                 │  Ranked Results  │                                │
│                 │  + Explanations  │                                │
│                 └──────────────────┘                                │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Geolocation-Enhanced Supplier Discovery

**Objective:** Find new suppliers near the job site that aren't in the database yet.

### 1.1 Geocoding Service Integration

Add a geocoding layer that converts addresses to coordinates and vice versa.

- **Geocoding API:** Integrate Google Maps Geocoding API or OpenStreetMap Nominatim
- **Address → Lat/Lon:** Convert client address to coordinates for job site proximity
- **Reverse geocoding:** Convert supplier coordinates to human-readable addresses
- **Radius search:** Define a configurable search radius (default: 50 miles) around the job site

**Implementation:**
- New class: `GeoService` with `geocode(address)` → `{lat, lon}` and `reverseGeocode(lat, lon)` → address
- Extend `app_config.yaml` with geocoding API key and search radius settings
- Update `ClientDTO` and `SupplierDTO` to auto-geocode when address is provided but lat/lon are missing

### 1.2 Places API Supplier Discovery

Use Google Places API (or Yelp Fusion API) to discover building material suppliers near a job site.

- **Search query construction:** "building materials supplier", "lumber yard", "concrete supplier", "roofing supply" — queries derived from the product categories in the database
- **Radius-based search:** Search within configurable radius of the client's job site
- **Result mapping:** Map Places API results to `SupplierDTO` fields (name, address, phone, rating, lat/lon)
- **Deduplication:** Match discovered suppliers against existing database entries by name + proximity

**Implementation:**
- New class: `SupplierDiscoveryService`
- Method: `discoverNearby(productCategory, lat, lon, radiusMiles)` → `vector<DiscoveredSupplier>`
- `DiscoveredSupplier` struct: extends SupplierDTO with `source` (Google/Yelp), `placeId`, `businessHours`, `photoUrl`, `reviewCount`

### 1.3 Drive Time & Route Distance

Replace straight-line Haversine distance with actual drive time estimates.

- **Routing API:** Google Directions API or OSRM (Open Source Routing Machine)
- **Drive time scoring:** Factor in drive time, not just distance — a supplier 30 miles away on a highway is closer than one 15 miles away through city traffic
- **Delivery zones:** Some suppliers deliver within a zone; model this as an availability enhancement

**Implementation:**
- New method on `GeoService`: `driveTime(fromLat, fromLon, toLat, toLon)` → `{distanceMiles, driveMinutes}`
- Extend `SourcingResult` with `driveMinutes` field
- Update `SourcingEngine` proximity scoring to weight drive time over straight-line distance

### Deliverables
- [ ] `GeoService` class with geocoding and routing
- [ ] `SupplierDiscoveryService` with Places API integration
- [ ] Config entries for API keys and search radius
- [ ] Updated SourcingEngine proximity scoring
- [ ] Unit tests for geocoding and discovery deduplication

---

## Phase 2: Web Scraping for Live Pricing & Availability

**Objective:** Validate and update supplier pricing in real-time by scraping supplier websites and major distributor platforms.

### 2.1 Scraping Infrastructure

Build a modular scraping framework that can target different supplier website formats.

- **Headless browser:** Use Puppeteer (Node.js) or Playwright for JavaScript-rendered pages
- **HTTP scraper:** Use libcurl or Boost.Beast for static HTML pages
- **Rate limiting:** Configurable delays between requests (respect robots.txt)
- **Caching:** Cache scraped data with configurable TTL (e.g., 24 hours for pricing, 4 hours for stock)
- **Proxy rotation:** Optional proxy support for high-volume scraping

**Implementation:**
- New microservice: `scraping-service/` (Node.js + Playwright recommended for ecosystem)
- REST API: `POST /scrape/price` with `{supplierUrl, productSku, productName}` → `{price, inStock, lastChecked}`
- Queue-based architecture: scraping jobs dispatched via message queue for async processing

### 2.2 Target Sources

| Source Type | Examples | Data Available |
|---|---|---|
| **Supplier websites** | Individual supplier catalog pages | Pricing, stock status, MOQ |
| **Distributor platforms** | Home Depot Pro, Lowe's for Pros, ABC Supply | Pricing, availability, bulk pricing tiers |
| **Manufacturer catalogs** | James Hardie, Owens Corning, LP Building Solutions | MSRP, spec sheets, dealer locators |
| **Price aggregators** | BuildingConnected, SmartBid | Competitive bid data |
| **Industry marketplaces** | BuildDirect, Materials Market | Market pricing signals |

### 2.3 Product Matching & SKU Resolution

Scraped data must be matched to products in the database.

- **SKU matching:** Direct lookup when supplier uses standard SKUs
- **Fuzzy name matching:** Use Levenshtein distance or TF-IDF similarity for product name matching
- **LLM-assisted matching:** Send product description + scraped listing to an LLM for classification confidence score
- **Unit normalization:** Convert between units (e.g., "per board foot" vs "per linear foot" vs "per piece")

**Implementation:**
- New class: `ProductMatcher`
- Method: `matchProduct(scrapedName, scrapedDescription, scrapedUnit)` → `{productId, confidence}`
- LLM integration for ambiguous matches (Claude API call with structured output)

### 2.4 Price Intelligence Dashboard

Surface scraped pricing data in the UI.

- **Market price range:** Show min/median/max across all discovered sources (not just database suppliers)
- **Price trend indicator:** Arrow up/down based on price change from last scrape
- **Freshness badge:** "Live" / "24h ago" / "Stale" indicator on each price
- **Alert system:** Notify contractor when a tracked product's price drops below threshold

**Implementation:**
- New DTO: `MarketPriceDTO` with `{productId, source, price, inStock, lastChecked, priceChange}`
- New view panel in Quote Builder: "Market Intelligence" card beside line items
- Mobile: new "Market" tab on product detail screen

### Deliverables
- [ ] Scraping microservice with Playwright
- [ ] Product matching engine with fuzzy + LLM matching
- [ ] Scrape targets for 3+ major distributor platforms
- [ ] Market price dashboard in desktop and mobile UI
- [ ] Price alert notification system
- [ ] Caching layer with TTL management

---

## Phase 3: Social Media & Review Sentiment Analysis

**Objective:** Replace the single `rating` field with a rich, AI-analyzed reputation score drawn from multiple sources.

### 3.1 Review Aggregation

Collect supplier reviews from multiple platforms:

| Platform | Data Available | Access Method |
|---|---|---|
| **Google Reviews** | Star rating, review text, reviewer info | Places API |
| **Yelp** | Star rating, review text, response history | Yelp Fusion API |
| **BBB (Better Business Bureau)** | Accreditation, complaints, resolution rate | Web scraping |
| **Angi (formerly Angie's List)** | Grade, review text, hire rate | Web scraping |
| **Facebook Business** | Star rating, recommendations, comments | Graph API |
| **Nextdoor** | Neighborhood recommendations | Web scraping (limited) |
| **Houzz** | Pro reviews, project photos | Web scraping |

### 3.2 AI Sentiment Analysis Pipeline

Process raw reviews through an NLP pipeline to extract structured signals:

```
Raw Reviews → Preprocessing → Sentiment Analysis → Theme Extraction → Trust Score
```

**Step 1: Preprocessing**
- Remove spam/fake reviews (ML classifier or LLM filter)
- Normalize ratings across platforms (Yelp 5-star, BBB A-F, etc.)
- Weight by recency (recent reviews count more)

**Step 2: Sentiment Classification**
- Use Claude API for nuanced sentiment analysis beyond star ratings
- Classify each review into: Positive / Neutral / Negative / Mixed
- Extract specific themes: pricing fairness, delivery reliability, product quality, customer service, dispute resolution

**Step 3: Theme Extraction (Contractor-Specific)**
- **Delivery reliability:** "delivered on time" / "always late"
- **Pricing accuracy:** "quote matched final bill" / "hidden charges"
- **Product quality:** "materials were as described" / "substituted inferior"
- **Communication:** "responsive" / "hard to reach"
- **Problem resolution:** "fixed the issue" / "ghosted me"
- **Contractor-specific needs:** "understands commercial jobs" / "residential only"

**Step 4: Composite Trust Score**
- Weighted aggregate: `0.3 * sentiment + 0.2 * recency + 0.2 * volume + 0.15 * theme_consistency + 0.15 * platform_diversity`
- Generate confidence interval: "4.2 ± 0.3 (based on 47 reviews across 4 platforms)"
- Flag anomalies: sudden rating drops, suspicious review patterns

### 3.3 Reputation Dashboard

- **Trust Score card:** Replaces the single `rating` field in the SourcingResults dialog
- **Review summary:** AI-generated 2-3 sentence summary of what contractors say
- **Red flags:** Prominent warnings for BBB complaints, recent negative trends
- **Source breakdown:** Show how many reviews from each platform contributed to the score

**Implementation:**
- New class: `ReputationService`
- Method: `analyzeReputation(supplierName, city, state)` → `ReputationReport`
- `ReputationReport` struct: `trustScore`, `confidence`, `reviewCount`, `platforms[]`, `themes[]`, `summary`, `redFlags[]`
- Claude API integration for sentiment analysis and summary generation
- Caching: reputation data refreshed weekly or on-demand

### Deliverables
- [ ] Review aggregation from Google, Yelp, BBB (minimum 3 platforms)
- [ ] AI sentiment analysis pipeline using Claude API
- [ ] Contractor-specific theme extraction
- [ ] Composite trust score algorithm
- [ ] Reputation dashboard in sourcing results dialog
- [ ] Mobile reputation view on supplier detail screen
- [ ] Weekly refresh cron job

---

## Phase 4: AI-Powered Sourcing Recommendations

**Objective:** Replace the fixed linear scoring formula with an AI agent that reasons about the best supplier for each specific job context.

### 4.1 Context-Aware Scoring

The current scoring formula applies the same weights to every sourcing decision. An AI-powered engine considers **context**:

- **Job type:** A kitchen remodel values speed; a new construction values bulk pricing
- **Budget sensitivity:** Some clients are price-driven; others prioritize quality
- **Seasonal factors:** Roofing materials are scarce after storm seasons
- **Relationship history:** Prefer suppliers the contractor has worked with successfully before
- **Risk tolerance:** For a high-stakes commercial job, reliability outweighs price

**Implementation:**
- Extend `SourcingWeights` to be dynamically generated by an LLM prompt that considers the full quote context
- Claude API call: "Given this quote context [title, client, job description, budget, timeline], recommend scoring weights and explain your reasoning"
- Return structured JSON: `{priceWeight, availWeight, proximityWeight, qualityWeight, reasoning}`

### 4.2 Explainable Recommendations

Every sourcing recommendation should include a natural-language explanation:

> "Recommended **Texas Building Supply** for 2x4 Stud Grade Lumber (qty 200). They're 12 miles from the job site, currently have 500+ in stock at $3.42/unit (15% below market average), and have a 4.6 trust score across 89 reviews. Their delivery reliability is rated 'Excellent' based on recent contractor feedback. Note: ABC Materials is $0.18/unit cheaper but has a 2-week lead time that may impact your project timeline."

**Implementation:**
- After scoring, pass the top 3–5 results + context to Claude API
- Prompt: "Explain why you'd recommend result #1 over the alternatives for this job"
- Display explanation in the SourcingResults dialog below the ranking table

### 4.3 Learning from Outcomes

Track which sourcing decisions the contractor actually accepts, and whether the outcome was positive:

- **Acceptance tracking:** Record which supplier was selected for each line item
- **Outcome feedback:** After quote completion, prompt for "How did this supplier perform?"
- **Pattern learning:** Over time, build a contractor-specific preference model
- **Prompt enrichment:** Include past outcomes in the AI recommendation prompt

**Implementation:**
- New fields on `QuoteLineItemDTO`: `sourcingAccepted`, `supplierOutcomeRating`, `outcomeNotes`
- New database table: `sourcing_outcome` with feedback data
- Enhanced Claude prompt includes: "This contractor has previously chosen [supplier X] 3 times with average satisfaction 4.5/5"

### Deliverables
- [ ] Context-aware dynamic weight generation via Claude API
- [ ] Natural-language recommendation explanations
- [ ] Outcome tracking schema and UI
- [ ] Preference learning prompt enrichment
- [ ] A/B testing framework: AI recommendations vs. fixed formula

---

## Phase 5: Integration & Configuration

### 5.1 Configuration Extensions

Extend `app_config.yaml` to support all new services:

```yaml
# ── AI & Intelligence Services ──────────────────────────
ai:
  # Claude API for sentiment analysis, recommendations, product matching
  claude:
    api_key: ""
    model: "claude-sonnet-4-6"
    max_tokens: 4096

  # Geolocation services
  geo:
    provider: "google"                # google | osm
    api_key: ""
    search_radius_miles: 50
    enable_drive_time: true

  # Supplier discovery
  discovery:
    enabled: true
    sources:
      - google_places
      - yelp
    refresh_interval_hours: 168       # weekly

  # Web scraping
  scraping:
    enabled: true
    service_url: "http://localhost:3100"
    cache_ttl_hours: 24
    respect_robots_txt: true
    max_concurrent_scrapes: 3

  # Reputation analysis
  reputation:
    enabled: true
    platforms:
      - google_reviews
      - yelp
      - bbb
    refresh_interval_hours: 168       # weekly
    min_reviews_for_score: 3

  # AI recommendations
  recommendations:
    enabled: true
    explain: true
    learn_from_outcomes: true
```

### 5.2 Service Architecture

```
┌────────────────────┐     ┌────────────────────┐
│  Wt Desktop App    │────▶│  ApiLogicServer    │
│  (C++ / Boost)     │     │  (Python / Flask)  │
└────────┬───────────┘     └────────────────────┘
         │
         │  HTTP
         ▼
┌────────────────────┐     ┌────────────────────┐
│  Intelligence      │────▶│  Scraping Service  │
│  Gateway           │     │  (Node / Playwright)│
│  (Python / FastAPI)│     └────────────────────┘
│                    │
│  • GeoService      │     ┌────────────────────┐
│  • DiscoveryService│────▶│  Claude API        │
│  • ReputationSvc   │     │  (Anthropic)       │
│  • PricingService  │     └────────────────────┘
│  • RecommendEngine │
│                    │     ┌────────────────────┐
│                    │────▶│  Google/Yelp APIs   │
│                    │     └────────────────────┘
└────────────────────┘
```

The **Intelligence Gateway** is a new Python/FastAPI microservice that orchestrates all AI and external API calls. The C++ desktop app and React mobile app call it through the same REST pattern as ApiLogicServer.

### 5.3 DataProvider Extension

Extend the existing `DataProvider` abstract interface with new methods:

```cpp
// New methods on DataProvider (or a new IntelligenceProvider interface)
virtual std::vector<DiscoveredSupplierDTO> discoverSuppliers(
    const std::string& productCategory, double lat, double lon, double radiusMiles) = 0;

virtual MarketPriceDTO getMarketPrice(long long productId, double lat, double lon) = 0;

virtual ReputationReport getSupplierReputation(long long supplierId) = 0;

virtual AISourcingResult getAIRecommendation(
    long long productId, int quantity, long long quoteId) = 0;
```

### Deliverables
- [ ] Intelligence Gateway microservice (FastAPI)
- [ ] Extended `app_config.yaml` with all service settings
- [ ] `IntelligenceProvider` interface in C++ and TypeScript
- [ ] End-to-end integration test suite
- [ ] Docker Compose for full stack (Wt + ApiLogicServer + Intelligence Gateway + Scraping Service)

---

## Implementation Priority & Dependencies

```
Phase 1: Geolocation         ──────────┐
(Weeks 1–3)                            │
                                       ▼
Phase 2: Web Scraping         Phase 3: Social & Reviews
(Weeks 3–6)                   (Weeks 3–6)
         │                             │
         └──────────┬──────────────────┘
                    ▼
         Phase 4: AI Recommendations
         (Weeks 6–9)
                    │
                    ▼
         Phase 5: Integration & Config
         (Weeks 9–11)
```

**Phase 1** is the foundation — geolocation is required for supplier discovery (Phase 2 and 3) and enriches the existing Haversine scoring.

**Phases 2 and 3** are independent and can run in parallel. Web scraping gives you live pricing; social analysis gives you reputation data. Both feed into the AI recommendation engine.

**Phase 4** requires data from all previous phases to generate context-aware, explainable recommendations.

**Phase 5** ties everything together with configuration, deployment, and monitoring.

---

## Risk Factors & Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| **API rate limits** (Google, Yelp) | Discovery and review collection throttled | Implement caching, batch requests, respect quotas |
| **Scraping legal/TOS issues** | Supplier websites may block or restrict scraping | Scrape only public data; implement robots.txt compliance; offer opt-in data partnerships |
| **LLM hallucination** | AI generates incorrect pricing or false supplier claims | Always show data provenance; never present AI output as fact without source attribution |
| **Cost of AI API calls** | Claude API costs scale with usage | Cache aggressively; use Haiku for classification, Sonnet for reasoning; batch where possible |
| **Stale data presentation** | Users trust "live" data that may be hours old | Always show "last checked" timestamps; distinguish "verified" vs "estimated" prices |
| **Data quality variance** | Reviews and scraped data vary wildly in reliability | Confidence scoring on all data; minimum thresholds for trust score generation |

---

## Success Metrics

| Metric | Current | Phase 1 Target | Phase 4 Target |
|---|---|---|---|
| Suppliers evaluated per sourcing request | 8 (database only) | 20+ (discovered + database) | 30+ (multi-source) |
| Price data freshness | Static (days/weeks old) | Live (within 24 hours) | Real-time with caching |
| Quality signal depth | 1 field (rating) | 3 fields (rating + distance + drive time) | Full reputation report |
| Recommendation explainability | None (numeric score only) | Scoring breakdown | Natural-language explanation |
| Contractor confidence in sourcing | Not measured | Survey baseline | 4.0+/5.0 satisfaction |

---

## Technology Stack Additions

| Component | Technology | Rationale |
|---|---|---|
| Intelligence Gateway | Python + FastAPI | Rich ecosystem for AI/ML, async HTTP, easy Claude SDK integration |
| Scraping Service | Node.js + Playwright | Best-in-class browser automation, handles JS-rendered supplier sites |
| AI/LLM | Claude API (Anthropic) | Strong reasoning for recommendations, structured output for scoring |
| Geocoding | Google Maps Platform or OSM/Nominatim | Industry standard; Nominatim for cost-free alternative |
| Caching | Redis | Fast TTL-based caching for prices, reviews, geocoding results |
| Job Queue | Redis + Bull (Node.js) or Celery (Python) | Async processing for scraping and review collection |
| Container Orchestration | Docker Compose → Kubernetes | Start with Compose for dev; Kubernetes for production scale |

---

## Conclusion

This plan transforms the Contractor Quotes application from a **static quote builder** into an **intelligent sourcing platform** that gives building and remodeling contractors a genuine competitive advantage. Each phase delivers standalone value while building toward the full AI-augmented vision:

- **Phase 1** lets contractors discover suppliers they didn't know existed near their job sites
- **Phase 2** gives them live market pricing so they know they're getting a fair deal
- **Phase 3** replaces gut-feel supplier trust with data-driven reputation intelligence
- **Phase 4** brings it all together with AI that explains *why* it recommends a specific supplier for *this specific job*

The DDD foundation already in place — clean entity models, abstract DataProvider interface, separation of domain from infrastructure — means each phase can be implemented as a new bounded context that integrates cleanly with the existing system. This is the same architectural discipline that made the first 9 phases successful, now applied at an enterprise scale.

---

*Prepared for the Contractor Quotes & Sourcing application — extending the existing 9-phase development with AI-augmented supplier intelligence.*
