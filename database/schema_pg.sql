-- Contractor Quotes & Sourcing – PostgreSQL Schema
-- Compatible with ApiLogicServer / SQLAlchemy

-- ───────────────────────────────────────────────
-- Product catalog
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS product (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    name            VARCHAR(255) NOT NULL,
    category        VARCHAR(100) NOT NULL,
    unit            VARCHAR(50) NOT NULL,
    specifications  TEXT,
    sku             VARCHAR(50)
);

-- ───────────────────────────────────────────────
-- Suppliers / vendors
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS supplier (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    name            VARCHAR(255) NOT NULL,
    address         VARCHAR(255),
    city            VARCHAR(100),
    state           VARCHAR(2),
    zip_code        VARCHAR(10),
    phone           VARCHAR(20),
    email           VARCHAR(255),
    website         VARCHAR(255),
    latitude        DOUBLE PRECISION DEFAULT 0.0,
    longitude       DOUBLE PRECISION DEFAULT 0.0,
    rating          DOUBLE PRECISION DEFAULT 0.0,
    lead_time_days  INTEGER DEFAULT 0
);

-- ───────────────────────────────────────────────
-- Which supplier stocks which product (bridge)
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS supplier_product (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    product_id      INTEGER NOT NULL REFERENCES product(id),
    supplier_id     INTEGER NOT NULL REFERENCES supplier(id),
    unit_price      DOUBLE PRECISION DEFAULT 0.0,
    stock_qty       INTEGER DEFAULT 0,
    in_stock        BOOLEAN DEFAULT FALSE,
    can_backorder   BOOLEAN DEFAULT FALSE,
    min_order_qty   INTEGER DEFAULT 1,
    bulk_discount   DOUBLE PRECISION DEFAULT 0.0,
    bulk_threshold  INTEGER DEFAULT 0,
    last_updated    TIMESTAMP
);

-- ───────────────────────────────────────────────
-- Clients (contractor's customers)
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS client (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    name            VARCHAR(255) NOT NULL,
    company         VARCHAR(255),
    address         VARCHAR(255),
    city            VARCHAR(100),
    state           VARCHAR(2),
    zip_code        VARCHAR(10),
    phone           VARCHAR(20),
    email           VARCHAR(255),
    latitude        DOUBLE PRECISION DEFAULT 0.0,
    longitude       DOUBLE PRECISION DEFAULT 0.0
);

-- ───────────────────────────────────────────────
-- Quotes
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS quote (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    client_id       INTEGER REFERENCES client(id),
    title           VARCHAR(255),
    description     TEXT,
    created_date    TIMESTAMP,
    expiry_date     TIMESTAMP,
    status          INTEGER DEFAULT 0,   -- 0=Draft,1=Sent,2=Accepted,3=Rejected,4=Expired
    tax_rate        DOUBLE PRECISION DEFAULT 0.0,
    markup_rate     DOUBLE PRECISION DEFAULT 0.0,
    notes           TEXT
);

-- ───────────────────────────────────────────────
-- Quote line items
-- ───────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS quote_line_item (
    id              SERIAL PRIMARY KEY,
    version         INTEGER NOT NULL DEFAULT 1,
    quote_id        INTEGER REFERENCES quote(id),
    product_id      INTEGER REFERENCES product(id),
    supplier_id     INTEGER REFERENCES supplier(id),
    quantity        INTEGER DEFAULT 0,
    unit_price      DOUBLE PRECISION DEFAULT 0.0,
    markup          DOUBLE PRECISION DEFAULT 0.0,
    line_total      DOUBLE PRECISION DEFAULT 0.0,
    notes           TEXT
);
