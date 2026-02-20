-- Contractor Quotes & Sourcing – SQLite Schema
-- Compatible with ApiLogicServer / SQLAlchemy

PRAGMA foreign_keys = ON;

-- ───────────────────────────────────────────────
-- Product catalog
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS product (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    name            TEXT    NOT NULL,
    category        TEXT    NOT NULL,
    unit            TEXT    NOT NULL,
    specifications  TEXT,
    sku             TEXT
);

-- ───────────────────────────────────────────────
-- Suppliers / vendors
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS supplier (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    name            TEXT    NOT NULL,
    address         TEXT,
    city            TEXT,
    state           TEXT,
    zip_code        TEXT,
    phone           TEXT,
    email           TEXT,
    website         TEXT,
    latitude        REAL    DEFAULT 0.0,
    longitude       REAL    DEFAULT 0.0,
    rating          REAL    DEFAULT 0.0,
    lead_time_days  INTEGER DEFAULT 0
);

-- ───────────────────────────────────────────────
-- Which supplier stocks which product (bridge)
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS supplier_product (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    product_id      INTEGER NOT NULL REFERENCES product(id),
    supplier_id     INTEGER NOT NULL REFERENCES supplier(id),
    unit_price      REAL    DEFAULT 0.0,
    stock_qty       INTEGER DEFAULT 0,
    in_stock        INTEGER DEFAULT 0,   -- boolean 0/1
    can_backorder   INTEGER DEFAULT 0,   -- boolean 0/1
    min_order_qty   INTEGER DEFAULT 1,
    bulk_discount   REAL    DEFAULT 0.0,
    bulk_threshold  INTEGER DEFAULT 0,
    last_updated    TEXT                  -- ISO-8601 datetime
);

-- ───────────────────────────────────────────────
-- Clients (contractor's customers)
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS client (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    name            TEXT    NOT NULL,
    company         TEXT,
    address         TEXT,
    city            TEXT,
    state           TEXT,
    zip_code        TEXT,
    phone           TEXT,
    email           TEXT,
    latitude        REAL    DEFAULT 0.0,
    longitude       REAL    DEFAULT 0.0
);

-- ───────────────────────────────────────────────
-- Quotes
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS quote (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    client_id       INTEGER REFERENCES client(id),
    title           TEXT,
    description     TEXT,
    created_date    TEXT,                 -- ISO-8601 datetime
    expiry_date     TEXT,                 -- ISO-8601 datetime
    status          INTEGER DEFAULT 0,   -- 0=Draft,1=Sent,2=Accepted,3=Rejected,4=Expired
    tax_rate        REAL    DEFAULT 0.0,
    markup_rate     REAL    DEFAULT 0.0,
    notes           TEXT
);

-- ───────────────────────────────────────────────
-- Quote line items
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS quote_line_item (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    version         INTEGER NOT NULL DEFAULT 1,
    quote_id        INTEGER REFERENCES quote(id),
    product_id      INTEGER REFERENCES product(id),
    supplier_id     INTEGER REFERENCES supplier(id),
    quantity        INTEGER DEFAULT 0,
    unit_price      REAL    DEFAULT 0.0,
    markup          REAL    DEFAULT 0.0,
    line_total      REAL    DEFAULT 0.0,
    notes           TEXT
);
