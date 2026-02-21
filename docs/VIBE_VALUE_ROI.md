# Vibe Coding: Business Value & ROI

## Executive Summary

The **Contractor Quotes & Sourcing** application demonstrates how AI-assisted "Vibe Coding" — the practice of describing intent in natural language and collaborating with an AI coding agent to produce working software — can deliver production-grade, multi-platform business applications at a fraction of the time and cost of traditional development. Over the course of 9 development phases, a series of conversational prompts produced a fully functional enterprise application spanning **three technology stacks**, **two runtime architectures**, and **six database tables** with complete CRUD operations, a domain-specific sourcing engine, and cross-platform mobile support.

This document captures the prompts that drove development, quantifies the output, and makes the case that **Domain-Driven Design (DDD) is the essential discipline** that separates productive Vibe Coding from chaotic prompt-and-pray experimentation.

---

## The Prompt Chronicle

The following is a reconstruction of the user prompts that drove each development phase, showing how concise, domain-focused direction produced compounding results.

### Phase 1 — Initial Application
> *"Implement a contractor quote sourcing application with C++ and Wt"*

A single prompt produced the entire foundation: 6 ORM-mapped entity models, a multi-criteria sourcing engine, 6 Bootstrap-themed views, SQLite persistence, session management, seed data for 27 products / 8 suppliers / 144 pricing relationships, and a Boost.Test suite.

### Phase 2 — Bootstrap 5 Resource Fix
> *"Wt is returning 404 for Bootstrap 5 theme files"*

A precise bug report. The AI diagnosed the `--docroot` vs `--resources-dir` distinction and patched both the startup script and CMake target.

### Phase 3 — Database Schema & Seed Data Export
> *"Create standalone SQL files to support external database provisioning for ApiLogicServer and PostgreSQL deployments"*

The prompt described the *why* (external deployment), not the *how*. The AI produced SQLite DDL, PostgreSQL DDL, seed data for both dialects, and a Python script that reproduces the deterministic pricing algorithm from C++.

### Phase 4 — ApiLogicServer Integration & Architecture Abstraction
> *"Refactor to support dual runtime architectures: direct local database or REST API client/server via ApiLogicServer"*

The most architecturally significant prompt. It triggered: a DataProvider abstraction layer, two concrete implementations (LocalDataProvider with Wt::Dbo, ApiDataProvider with Boost.Asio HTTP), DTO structs, a YAML configuration system, JSON:API request/response handling, and a rewrite of all 6 views plus the SourcingEngine to depend on the abstract interface.

### Phase 5 — Documentation
> *"Create README and development documentation"*

### Phase 6 — Mobile Application
> *"Add a hybrid mobile app that connects to the same ApiLogicServer REST backend"*

One prompt produced a complete React 18 + TypeScript + Capacitor 6 mobile app: 6 screens, a full JSON:API client mirroring the C++ provider, TypeScript interfaces matching every C++ DTO, mobile-first responsive CSS, and iOS/Android packaging configuration.

### Phase 7 — UI Branding & Layout Improvements
> *"Apply Imagery branding to mobile and desktop apps"*
> *"Hide quote list when editing; return to list on save"*
> *"Rearrange Quote Details to 3-column and 2-column grid layout"*
> *"Make desktop navbar sticky, rename title, add About dialog"*
> *"Replace Back to Quotes button with arrow + Quote ID"*

A series of rapid UX refinement prompts, each producing targeted, non-breaking changes.

### Phase 8 — Quote List Data Bug
> *"The totals and date created, as well as client are not being updated in the Quote Builder list after saving a Quote"*

A user-reported bug described in business terms. The AI traced the full data flow — from UI save handler through the DataProvider abstraction to the API response parser — and identified that the ApiDataProvider's `parseQuote()` never resolved denormalized fields from the raw JSON:API response.

### Phase 9 — Comprehensive Denormalized Field Audit
> *"Sourcing results card has no Supplier Name, another case where there is a denormalized value. Validate the repository for this issue throughout."*

The user spotted one more symptom and asked for a **repository-wide audit**. The AI systematically examined all 7 DTO types across both providers (C++ and TypeScript), identified 18 missing denormalized fields across 6 DTO types, and implemented batch-enrichment methods in both codebases — with cascade prevention, N+1 avoidance, and parallel fetch optimization.

---

## What Was Delivered

| Metric | Value |
|---|---|
| Development phases | 9 |
| Total commits | 34 |
| C++ source files (`.cpp` / `.h`) | 32 |
| Mobile source files (`.ts` / `.tsx` / `.css`) | 13 |
| Database & config files | 5 SQL + 2 YAML + 1 Python |
| C++ lines of code | ~4,500 |
| Mobile lines of code | ~2,200 |
| Database/seed lines | ~1,500 |
| **Total lines of code** | **~8,200** |
| Domain entity models | 6 (Product, Supplier, SupplierProduct, Client, Quote, QuoteLineItem) |
| DTO types | 7 (+ CategoryStatDTO) |
| UI views / screens | 6 desktop + 6 mobile |
| Runtime architectures | 2 (local SQLite, REST API) |
| Technology stacks | 3 (C++/Wt/Boost, React/TypeScript/Capacitor, Python/SQL) |
| Automated test cases | 7 (Boost.Test sourcing engine suite) |

---

## The DDD Imperative: Why Domain-Driven Design Is Non-Negotiable

Vibe Coding amplifies whatever design discipline — or lack thereof — is present in the conversation. The Contractor Quotes project demonstrates that **Domain-Driven Design is the force multiplier** that turns Vibe Coding from a novelty into a serious engineering practice.

### 1. The Ubiquitous Language Scales Across Stacks

The domain model — Products, Suppliers, Clients, Quotes, Line Items, Sourcing — was established in Phase 1 and remained stable through 9 phases, 3 technology stacks, and 34 commits. Because the entities, their relationships, and their business rules were expressed in a shared language from the start, the AI could:

- Generate C++ ORM models that mapped cleanly to SQL tables
- Produce TypeScript interfaces that mirror the C++ DTOs field-for-field
- Build a JSON:API client whose endpoint names (`/Product/`, `/Quote/`, `/QuoteLineItem/`) are direct reflections of the domain
- Implement a SourcingEngine whose scoring algorithm speaks in domain terms (price weight, reliability weight, proximity weight)

**Without a ubiquitous language, the AI generates code that works in isolation but doesn't compose.** With one, every new component slots into the existing architecture like a puzzle piece.

### 2. Bounded Contexts Prevent Architectural Drift

The Phase 4 refactoring — introducing the DataProvider abstraction — is a textbook example of a bounded context boundary. The domain logic (SourcingEngine, views) lives in one context; data access (LocalDataProvider, ApiDataProvider) lives in another; the API contract (JSON:API format, DTO structs) is the anti-corruption layer between them.

This boundary paid dividends immediately:

- **Phase 6**: The mobile app was built against the same API contract without touching the C++ code
- **Phase 8–9**: The denormalized field bug was isolated entirely to the ApiDataProvider layer — the views and domain logic were untouched
- **Testing**: The Boost.Test suite runs against LocalDataProvider while production uses ApiDataProvider — same domain, different infrastructure

**Without bounded contexts, Vibe Coding produces a monolith where every change ripples everywhere.** With them, the AI can work on one bounded context without destabilizing others.

### 3. Aggregates and Value Objects Reduce Prompt Ambiguity

When the user said *"Sourcing results card has no Supplier Name"*, the AI didn't need a 20-line specification. It understood immediately:

- `SupplierProduct` is an aggregate that references `Supplier` and `Product`
- `supplierName` is a denormalized value carried on the DTO for display convenience
- The fix requires resolving the relationship at the data access layer, not the view layer

This is DDD's **aggregate pattern** at work. Because the domain model makes ownership and relationships explicit, the AI can infer the correct fix location from a one-sentence bug report.

**Without well-defined aggregates, the AI guesses where to put the fix — and often guesses wrong**, creating patches in the view layer that should live in the data layer, or vice versa.

### 4. Strategic Design Enables Multi-Platform from Day One

The decision to separate domain from infrastructure (Phase 4) was a strategic DDD choice. It meant the domain model existed as pure DTOs and abstract interfaces — completely decoupled from Wt::Dbo, Boost.Asio, React, or any framework. This strategic separation enabled:

- A C++ desktop app and a React mobile app sharing the same domain model
- Two data providers (local and API) that are interchangeable at startup
- A sourcing engine that works identically regardless of data source
- Bug fixes (Phase 9) that apply symmetrically to both platforms

**Without strategic design, adding a mobile app means rewriting the domain.** With it, the mobile app was a single-prompt deliverable.

---

## ROI Analysis: Vibe Coding with DDD

### Time Compression

A traditional development team building this application would need:

| Activity | Traditional Estimate | Vibe Coding Actual |
|---|---|---|
| Domain modeling & schema design | 1–2 weeks | Embedded in Phase 1 prompt |
| C++ desktop app (6 views, ORM, sourcing engine) | 4–6 weeks | Phase 1 |
| Database export & multi-dialect SQL | 1 week | Phase 3 |
| Architecture abstraction (DataProvider, dual-mode) | 2–3 weeks | Phase 4 |
| Mobile app (React/TS/Capacitor, 6 screens) | 3–4 weeks | Phase 6 |
| UI polish & branding | 1 week | Phase 7 |
| Bug diagnosis & cross-platform fix | 1–2 weeks | Phases 8–9 |
| **Total** | **13–19 weeks** | **9 conversational phases** |

### Cost Multiplier

The traditional estimate assumes a small team (1 senior C++ developer, 1 React/mobile developer, 1 DBA) over 3–5 months. At blended rates, this represents **$75K–$150K** in development cost.

Vibe Coding with a well-prepared domain model reduces this to the cost of the AI tooling plus the domain expert's time directing the conversation — a fraction of the traditional budget.

### Quality Observations

- **Consistency**: The same domain model is enforced across C++, TypeScript, SQL, and YAML — no drift between platforms
- **Test coverage**: The sourcing engine has automated tests from Phase 1; the domain logic has never regressed
- **Architectural integrity**: The DataProvider abstraction has held through 5 subsequent phases without modification
- **Bug isolation**: When bugs surfaced (Phases 8–9), they were contained to a single layer and fixed without collateral damage

---

## Key Takeaways

1. **Vibe Coding is not "no design" — it is "design through conversation."** The quality of the output is directly proportional to the clarity of the domain model in the prompts.

2. **DDD provides the vocabulary that makes Vibe Coding precise.** Prompts like "refactor to support dual runtime architectures" and "validate denormalized fields throughout the repository" work because the underlying domain concepts (bounded contexts, aggregates, value objects) are well-defined.

3. **The ROI is non-linear.** Each phase builds on the domain foundation established earlier. Phase 6 (mobile app) was possible in a single prompt because Phase 4 (architecture abstraction) created the right seams. Phase 9 (cross-platform audit) was possible because the DTO contracts were consistent.

4. **Domain experts, not just developers, can drive Vibe Coding.** The prompts in this project are written in business language ("the totals and client are not updating"), not implementation language. The AI bridges the gap — but only when the domain model is sound enough to bridge *to*.

5. **The biggest risk in Vibe Coding is not bad code — it is missing design.** Without DDD, Vibe Coding produces fast, disconnected fragments. With DDD, it produces fast, composable systems that compound in value with each iteration.

---

*Generated from the development history of the Contractor Quotes & Sourcing application — 9 phases, 34 commits, ~8,200 lines of code across C++, TypeScript, SQL, and Python.*
