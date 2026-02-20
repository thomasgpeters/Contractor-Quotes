#!/usr/bin/env python3
"""
Generate seed_data.sql (SQLite) and seed_data_pg.sql (PostgreSQL)
for the Contractor Quotes & Sourcing application.

The supplier_product rows use the same deterministic algorithm as
src/models/Session.cpp::seedSupplierProducts().
"""

import datetime

# ── Static seed data ────────────────────────────────────────────

PRODUCTS = [
    # (name, category, unit, specifications, sku)
    ("2x4x8 Stud Grade SPF",         "Lumber",      "piece",  "Spruce-Pine-Fir, #2 & Better, KD",  "LBR-2408"),
    ("2x6x10 #2 Doug Fir",           "Lumber",      "piece",  "Douglas Fir, #2 Grade, S4S",         "LBR-2610"),
    ("4x4x8 Treated Post",           "Lumber",      "piece",  "Pressure Treated, Ground Contact",   "LBR-4408T"),
    ('3/4" CDX Plywood 4x8',         "Lumber",      "sheet",  "CDX Grade, Exposure 1",              "PLY-3448"),
    ('7/16" OSB Sheathing 4x8',      "Lumber",      "sheet",  "Structural, APA Rated",              "PLY-0748"),
    ("80lb Concrete Mix",             "Concrete",    "bag",    "4000 PSI, Normal Set",               "CON-80NM"),
    ("60lb Quikrete Fast-Set",        "Concrete",    "bag",    "4000 PSI, Fast Setting",             "CON-60FS"),
    ("#4 Rebar 20ft",                 "Concrete",    "piece",  'Grade 60, 1/2" diameter',            "REB-0420"),
    ("Wire Mesh 6x6 W1.4",           "Concrete",    "sheet",  "Welded, 5ft x 150ft roll",           "MSH-6614"),
    ("Architectural Shingles",        "Roofing",     "bundle", "30-Year, Algae Resistant",           "ROF-ARCH"),
    ("30# Felt Underlayment",         "Roofing",     "roll",   "ASTM D226, 36\" x 144'",            "ROF-30FL"),
    ("Ice & Water Shield",            "Roofing",     "roll",   "Self-Adhesive, 36\" x 75'",          "ROF-IAWS"),
    ("Drip Edge 10ft",                "Roofing",     "piece",  'Aluminum, 2" x 2"',                  "ROF-DRIP"),
    ("12/2 NM-B Romex 250ft",        "Electrical",  "roll",   "With Ground, UL Listed",             "ELE-1225"),
    ("14/2 NM-B Romex 250ft",        "Electrical",  "roll",   "With Ground, UL Listed",             "ELE-1425"),
    ("200A Main Panel Box",           "Electrical",  "piece",  "42-Space, Indoor, NEMA 1",           "ELE-200P"),
    ("20A GFCI Outlet",               "Electrical",  "piece",  "Tamper-Resistant, Self-Test",         "ELE-GFCI"),
    ('1/2" Copper Type L 10ft',       "Plumbing",   "piece",  "Type L, Hard Drawn",                  "PLB-05CL"),
    ('3/4" PEX Tubing 100ft',        "Plumbing",   "roll",   "Oxygen Barrier, ASTM F876",           "PLB-075P"),
    ("40-Gal Water Heater",           "Plumbing",   "piece",  "Gas, 40000 BTU, Energy Star",         "PLB-40WH"),
    ('3" PVC DWV Pipe 10ft',         "Plumbing",   "piece",  "Schedule 40, ASTM D2665",             "PLB-3PVC"),
    ('R-19 Fiberglass Batt 6.25"',   "Insulation", "bag",    'Kraft Faced, 15" wide',               "INS-R19F"),
    ('R-30 Fiberglass Batt 10"',     "Insulation", "bag",    'Kraft Faced, 15" wide',               "INS-R30F"),
    ('2" Rigid Foam XPS 4x8',        "Insulation", "sheet",  "R-10, Moisture Resistant",            "INS-2XPS"),
    ('1/2" Drywall 4x8',             "Drywall",    "sheet",  "Standard, UL Fire Rated",             "DRY-1248"),
    ('5/8" Fire-Rated Drywall',       "Drywall",    "sheet",  "Type X, 4x8",                        "DRY-5848"),
    ("Joint Compound 5-Gal",          "Drywall",    "bucket", "All-Purpose, Ready Mixed",           "DRY-JC5G"),
]

SUPPLIERS = [
    # (name, address, city, state, zip, phone, email, lat, lon, rating, lead_time_days)
    ("BuildRight Supply Co.",    "1200 Industrial Blvd", "Austin",         "TX", "78701",
     "(512) 555-0101", "sales@buildright.example.com",    30.267, -97.743, 4.5, 2),
    ("Metro Building Materials", "450 Commerce St",      "Dallas",         "TX", "75201",
     "(214) 555-0202", "orders@metrobm.example.com",      32.783, -96.797, 4.2, 3),
    ("Lone Star Lumber",        "890 Timber Rd",         "Houston",        "TX", "77001",
     "(713) 555-0303", "info@lonestarlbr.example.com",    29.760, -95.370, 4.8, 1),
    ("SunBelt Wholesale",       "2100 Supply Dr",        "San Antonio",    "TX", "78201",
     "(210) 555-0404", "wholesale@sunbelt.example.com",   29.424, -98.494, 3.9, 4),
    ("ProTrade Distributors",   "560 Warehouse Ave",     "Fort Worth",     "TX", "76102",
     "(817) 555-0505", "sales@protrade.example.com",      32.755, -97.331, 4.6, 2),
    ("Capital City Supply",     "700 Main Plaza",        "Austin",         "TX", "78702",
     "(512) 555-0606", "orders@capitalcity.example.com",  30.260, -97.738, 4.0, 3),
    ("Gulf Coast Materials",    "1500 Port Blvd",        "Corpus Christi", "TX", "78401",
     "(361) 555-0707", "sales@gulfcoastmat.example.com",  27.801, -97.396, 3.7, 5),
    ("Hill Country Hardware",   "310 Ranch Rd",          "Fredericksburg", "TX", "78624",
     "(830) 555-0808", "info@hillcountry.example.com",    30.275, -98.872, 4.3, 3),
]

CLIENTS = [
    # (name, company, address, city, state, zip, phone, email, lat, lon)
    ("John Martinez",   "Martinez Construction",  "500 Oak Hill Dr",     "Austin",      "TX", "78745",
     "(512) 555-1001", "john@martinezconst.example.com",  30.210, -97.790),
    ("Sarah Chen",      "Chen Renovations LLC",   "1820 Elm St",         "Dallas",      "TX", "75202",
     "(214) 555-1002", "sarah@chenreno.example.com",      32.780, -96.800),
    ("Robert Williams", "Williams Custom Homes",  "4050 Cypress Creek",  "Houston",     "TX", "77014",
     "(713) 555-1003", "rob@williamscustom.example.com",  29.800, -95.400),
    ("Emily Johnson",   "Johnson Property Mgmt",  "230 River Walk",      "San Antonio", "TX", "78205",
     "(210) 555-1004", "emily@johnsonpm.example.com",     29.420, -98.490),
    ("Mike Thompson",   "Thompson Builders Inc",  "910 Prairie View Ln", "Fort Worth",  "TX", "76104",
     "(817) 555-1005", "mike@thompsonbld.example.com",    32.730, -97.340),
]


def sql_str(val):
    """Escape a string for SQL single-quote literals."""
    if val is None:
        return "NULL"
    return "'" + val.replace("'", "''") + "'"


def compute_supplier_products():
    """Reproduce the C++ seedSupplierProducts() algorithm exactly."""
    now = datetime.datetime(2025, 1, 15, 12, 0, 0)  # fixed seed timestamp
    rows = []
    idx = 0
    sp_id = 0
    for prod_idx, prod in enumerate(PRODUCTS):
        product_id = prod_idx + 1  # 1-based
        category = prod[1]
        for sup_idx, sup in enumerate(SUPPLIERS):
            supplier_id = sup_idx + 1  # 1-based
            rating = sup[9]

            if (idx + supplier_id) % 3 == 0:
                idx += 1
                continue

            base_price_multiplier = 0.85 + (rating / 5.0) * 0.30

            if   category == "Lumber":      base_price = 4.50 + (idx % 7) * 1.2
            elif category == "Concrete":    base_price = 5.00 + (idx % 5) * 0.8
            elif category == "Roofing":     base_price = 25.0 + (idx % 10) * 3.0
            elif category == "Electrical":  base_price = 15.0 + (idx % 8) * 10.0
            elif category == "Plumbing":    base_price = 8.0  + (idx % 9) * 5.0
            elif category == "Insulation":  base_price = 18.0 + (idx % 6) * 2.5
            elif category == "Drywall":     base_price = 10.0 + (idx % 5) * 1.5
            else:                           base_price = 5.0

            unit_price = round(base_price * base_price_multiplier, 2)
            stock_qty = 20 + (idx * 17 + supplier_id * 31) % 500
            in_stock = 1 if stock_qty > 0 else 0
            can_backorder = 1 if supplier_id % 2 == 0 else 0
            min_order_qty = 1 + (idx % 3)
            bulk_discount = 5.0 if idx % 4 == 0 else (3.0 if idx % 3 == 0 else 0.0)
            bulk_threshold = (50 + (idx % 50)) if bulk_discount > 0 else 0

            sp_id += 1
            rows.append((
                sp_id, product_id, supplier_id,
                unit_price, stock_qty, in_stock, can_backorder,
                min_order_qty, bulk_discount, bulk_threshold, now
            ))
            idx += 1

    return rows


def generate_sqlite_seed():
    lines = []
    lines.append("-- Contractor Quotes & Sourcing – SQLite Seed Data")
    lines.append("-- Generated by generate_seed.py\n")

    # Products
    lines.append("-- ── Products ──────────────────────────────────────────")
    for i, p in enumerate(PRODUCTS, 1):
        lines.append(
            f"INSERT INTO product (id, version, name, category, unit, specifications, sku) "
            f"VALUES ({i}, 1, {sql_str(p[0])}, {sql_str(p[1])}, {sql_str(p[2])}, {sql_str(p[3])}, {sql_str(p[4])});"
        )

    lines.append("")

    # Suppliers
    lines.append("-- ── Suppliers ─────────────────────────────────────────")
    for i, s in enumerate(SUPPLIERS, 1):
        lines.append(
            f"INSERT INTO supplier (id, version, name, address, city, state, zip_code, phone, email, website, latitude, longitude, rating, lead_time_days) "
            f"VALUES ({i}, 1, {sql_str(s[0])}, {sql_str(s[1])}, {sql_str(s[2])}, {sql_str(s[3])}, {sql_str(s[4])}, "
            f"{sql_str(s[5])}, {sql_str(s[6])}, NULL, {s[7]}, {s[8]}, {s[9]}, {s[10]});"
        )

    lines.append("")

    # Supplier-Products
    lines.append("-- ── Supplier Products ─────────────────────────────────")
    sp_rows = compute_supplier_products()
    for r in sp_rows:
        ts = r[10].strftime("%Y-%m-%dT%H:%M:%S")
        lines.append(
            f"INSERT INTO supplier_product (id, version, product_id, supplier_id, unit_price, stock_qty, in_stock, can_backorder, min_order_qty, bulk_discount, bulk_threshold, last_updated) "
            f"VALUES ({r[0]}, 1, {r[1]}, {r[2]}, {r[3]}, {r[4]}, {r[5]}, {r[6]}, {r[7]}, {r[8]}, {r[9]}, '{ts}');"
        )

    lines.append("")

    # Clients
    lines.append("-- ── Clients ──────────────────────────────────────────")
    for i, c in enumerate(CLIENTS, 1):
        lines.append(
            f"INSERT INTO client (id, version, name, company, address, city, state, zip_code, phone, email, latitude, longitude) "
            f"VALUES ({i}, 1, {sql_str(c[0])}, {sql_str(c[1])}, {sql_str(c[2])}, {sql_str(c[3])}, {sql_str(c[4])}, "
            f"{sql_str(c[5])}, {sql_str(c[6])}, {sql_str(c[7])}, {c[8]}, {c[9]});"
        )

    lines.append("")
    return "\n".join(lines)


def generate_pg_seed():
    lines = []
    lines.append("-- Contractor Quotes & Sourcing – PostgreSQL Seed Data")
    lines.append("-- Generated by generate_seed.py\n")

    # Products
    lines.append("-- ── Products ──────────────────────────────────────────")
    for i, p in enumerate(PRODUCTS, 1):
        lines.append(
            f"INSERT INTO product (id, version, name, category, unit, specifications, sku) "
            f"VALUES ({i}, 1, {sql_str(p[0])}, {sql_str(p[1])}, {sql_str(p[2])}, {sql_str(p[3])}, {sql_str(p[4])});"
        )
    lines.append(f"SELECT setval('product_id_seq', {len(PRODUCTS)});")

    lines.append("")

    # Suppliers
    lines.append("-- ── Suppliers ─────────────────────────────────────────")
    for i, s in enumerate(SUPPLIERS, 1):
        lines.append(
            f"INSERT INTO supplier (id, version, name, address, city, state, zip_code, phone, email, website, latitude, longitude, rating, lead_time_days) "
            f"VALUES ({i}, 1, {sql_str(s[0])}, {sql_str(s[1])}, {sql_str(s[2])}, {sql_str(s[3])}, {sql_str(s[4])}, "
            f"{sql_str(s[5])}, {sql_str(s[6])}, NULL, {s[7]}, {s[8]}, {s[9]}, {s[10]});"
        )
    lines.append(f"SELECT setval('supplier_id_seq', {len(SUPPLIERS)});")

    lines.append("")

    # Supplier-Products
    lines.append("-- ── Supplier Products ─────────────────────────────────")
    sp_rows = compute_supplier_products()
    for r in sp_rows:
        ts = r[10].strftime("%Y-%m-%d %H:%M:%S")
        in_stock = "TRUE" if r[5] else "FALSE"
        can_bo   = "TRUE" if r[6] else "FALSE"
        lines.append(
            f"INSERT INTO supplier_product (id, version, product_id, supplier_id, unit_price, stock_qty, in_stock, can_backorder, min_order_qty, bulk_discount, bulk_threshold, last_updated) "
            f"VALUES ({r[0]}, 1, {r[1]}, {r[2]}, {r[3]}, {r[4]}, {in_stock}, {can_bo}, {r[7]}, {r[8]}, {r[9]}, '{ts}');"
        )
    lines.append(f"SELECT setval('supplier_product_id_seq', {len(sp_rows)});")

    lines.append("")

    # Clients
    lines.append("-- ── Clients ──────────────────────────────────────────")
    for i, c in enumerate(CLIENTS, 1):
        lines.append(
            f"INSERT INTO client (id, version, name, company, address, city, state, zip_code, phone, email, latitude, longitude) "
            f"VALUES ({i}, 1, {sql_str(c[0])}, {sql_str(c[1])}, {sql_str(c[2])}, {sql_str(c[3])}, {sql_str(c[4])}, "
            f"{sql_str(c[5])}, {sql_str(c[6])}, {sql_str(c[7])}, {c[8]}, {c[9]});"
        )
    lines.append(f"SELECT setval('client_id_seq', {len(CLIENTS)});")

    lines.append("")
    return "\n".join(lines)


if __name__ == "__main__":
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))

    sqlite_path = os.path.join(script_dir, "seed_data.sql")
    pg_path     = os.path.join(script_dir, "seed_data_pg.sql")

    with open(sqlite_path, "w") as f:
        f.write(generate_sqlite_seed())
    print(f"Wrote {sqlite_path}")

    with open(pg_path, "w") as f:
        f.write(generate_pg_seed())
    print(f"Wrote {pg_path}")

    # Summary
    sp_rows = compute_supplier_products()
    print(f"\nSeed data summary:")
    print(f"  Products:          {len(PRODUCTS)}")
    print(f"  Suppliers:         {len(SUPPLIERS)}")
    print(f"  Supplier-Products: {len(sp_rows)}")
    print(f"  Clients:           {len(CLIENTS)}")
