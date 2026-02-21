// JSON:API client for ApiLogicServer (SAFRS)
// Mirrors the C++ ApiDataProvider implementation

import type {
  Product,
  Supplier,
  SupplierProduct,
  Client,
  Quote,
  QuoteLineItem,
  CategoryStat,
  DashboardStats,
} from '../types/models';

// Default API URL - can be overridden via localStorage
const DEFAULT_API_URL = 'http://localhost:5667/api';

function getBaseUrl(): string {
  return localStorage.getItem('api_base_url') || DEFAULT_API_URL;
}

export function setApiBaseUrl(url: string): void {
  localStorage.setItem('api_base_url', url);
}

export function getApiBaseUrl(): string {
  return getBaseUrl();
}

// ── JSON:API helpers ────────────────────────────────────────

interface JsonApiResource {
  id: string;
  type: string;
  attributes: Record<string, unknown>;
}

interface JsonApiResponse {
  data: JsonApiResource | JsonApiResource[];
  meta?: { count?: number; total?: number };
}

function attr(resource: JsonApiResource, key: string, fallback: unknown = ''): unknown {
  const val = resource.attributes[key];
  return val !== null && val !== undefined ? val : fallback;
}

function strAttr(r: JsonApiResource, key: string): string {
  return String(attr(r, key, ''));
}

function numAttr(r: JsonApiResource, key: string): number {
  const v = attr(r, key, 0);
  return typeof v === 'number' ? v : Number(v) || 0;
}

function boolAttr(r: JsonApiResource, key: string): boolean {
  const v = attr(r, key, false);
  if (typeof v === 'boolean') return v;
  if (typeof v === 'number') return v !== 0;
  return false;
}

async function apiGet(path: string): Promise<JsonApiResponse> {
  const res = await fetch(`${getBaseUrl()}${path}`, {
    headers: { Accept: 'application/json' },
  });
  if (!res.ok) throw new Error(`API ${res.status}: ${path}`);
  return res.json();
}

async function apiPost(path: string, type: string, attributes: Record<string, unknown>): Promise<JsonApiResponse> {
  const res = await fetch(`${getBaseUrl()}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', Accept: 'application/json' },
    body: JSON.stringify({ data: { type, attributes } }),
  });
  if (!res.ok) throw new Error(`API ${res.status}: POST ${path}`);
  return res.json();
}

async function apiPatch(path: string, id: number, type: string, attributes: Record<string, unknown>): Promise<JsonApiResponse> {
  const res = await fetch(`${getBaseUrl()}${path}`, {
    method: 'PATCH',
    headers: { 'Content-Type': 'application/json', Accept: 'application/json' },
    body: JSON.stringify({ data: { id: String(id), type, attributes } }),
  });
  if (!res.ok) throw new Error(`API ${res.status}: PATCH ${path}`);
  return res.json();
}

async function apiDelete(path: string): Promise<void> {
  const res = await fetch(`${getBaseUrl()}${path}`, { method: 'DELETE' });
  if (!res.ok && res.status !== 204) throw new Error(`API ${res.status}: DELETE ${path}`);
}

function getDataArray(json: JsonApiResponse): JsonApiResource[] {
  return Array.isArray(json.data) ? json.data : [];
}

function getDataObject(json: JsonApiResponse): JsonApiResource | null {
  return !Array.isArray(json.data) ? json.data : null;
}

// ── Products ─────────────────────────────────────────────────

function parseProduct(r: JsonApiResource): Product {
  return {
    id: Number(r.id),
    name: strAttr(r, 'name'),
    category: strAttr(r, 'category'),
    unit: strAttr(r, 'unit'),
    specifications: strAttr(r, 'specifications'),
    sku: strAttr(r, 'sku'),
  };
}

export async function fetchProducts(): Promise<Product[]> {
  const json = await apiGet('/Product/?page[limit]=1000');
  return getDataArray(json).map(parseProduct);
}

export async function fetchProductById(id: number): Promise<Product | null> {
  try {
    const json = await apiGet(`/Product/${id}/`);
    const r = getDataObject(json);
    return r ? parseProduct(r) : null;
  } catch {
    return null;
  }
}

export async function fetchDistinctCategories(): Promise<string[]> {
  const products = await fetchProducts();
  const cats = new Set(products.map((p) => p.category));
  return [...cats].sort();
}

// ── Suppliers ────────────────────────────────────────────────

function parseSupplier(r: JsonApiResource): Supplier {
  return {
    id: Number(r.id),
    name: strAttr(r, 'name'),
    address: strAttr(r, 'address'),
    city: strAttr(r, 'city'),
    state: strAttr(r, 'state'),
    zipCode: strAttr(r, 'zip_code'),
    phone: strAttr(r, 'phone'),
    email: strAttr(r, 'email'),
    website: strAttr(r, 'website'),
    latitude: numAttr(r, 'latitude'),
    longitude: numAttr(r, 'longitude'),
    rating: numAttr(r, 'rating'),
    leadTimeDays: numAttr(r, 'lead_time_days'),
  };
}

export async function fetchSuppliers(): Promise<Supplier[]> {
  const json = await apiGet('/Supplier/?page[limit]=1000');
  return getDataArray(json).map(parseSupplier);
}

export async function fetchSupplierById(id: number): Promise<Supplier | null> {
  try {
    const json = await apiGet(`/Supplier/${id}/`);
    const r = getDataObject(json);
    return r ? parseSupplier(r) : null;
  } catch {
    return null;
  }
}

// ── Supplier-Products ────────────────────────────────────────

function parseSupplierProduct(r: JsonApiResource): SupplierProduct {
  return {
    id: Number(r.id),
    productId: numAttr(r, 'product_id'),
    supplierId: numAttr(r, 'supplier_id'),
    unitPrice: numAttr(r, 'unit_price'),
    stockQty: numAttr(r, 'stock_qty'),
    inStock: boolAttr(r, 'in_stock'),
    canBackorder: boolAttr(r, 'can_backorder'),
    minOrderQty: numAttr(r, 'min_order_qty'),
    bulkDiscount: numAttr(r, 'bulk_discount'),
    bulkThreshold: numAttr(r, 'bulk_threshold'),
    lastUpdated: strAttr(r, 'last_updated'),
  };
}

export async function fetchSupplierProductsByProductId(productId: number): Promise<SupplierProduct[]> {
  const json = await apiGet(`/SupplierProduct/?filter[product_id]=${productId}&page[limit]=1000`);
  return getDataArray(json).map(parseSupplierProduct);
}

export async function fetchSupplierProductsBySupplierId(supplierId: number): Promise<SupplierProduct[]> {
  const json = await apiGet(`/SupplierProduct/?filter[supplier_id]=${supplierId}&page[limit]=1000`);
  return getDataArray(json).map(parseSupplierProduct);
}

// ── Clients ──────────────────────────────────────────────────

function parseClient(r: JsonApiResource): Client {
  return {
    id: Number(r.id),
    name: strAttr(r, 'name'),
    company: strAttr(r, 'company'),
    address: strAttr(r, 'address'),
    city: strAttr(r, 'city'),
    state: strAttr(r, 'state'),
    zipCode: strAttr(r, 'zip_code'),
    phone: strAttr(r, 'phone'),
    email: strAttr(r, 'email'),
    latitude: numAttr(r, 'latitude'),
    longitude: numAttr(r, 'longitude'),
  };
}

export async function fetchClients(): Promise<Client[]> {
  const json = await apiGet('/Client/?page[limit]=1000');
  return getDataArray(json).map(parseClient);
}

export async function fetchClientById(id: number): Promise<Client | null> {
  try {
    const json = await apiGet(`/Client/${id}/`);
    const r = getDataObject(json);
    return r ? parseClient(r) : null;
  } catch {
    return null;
  }
}

export async function createClient(client: Partial<Client>): Promise<Client> {
  const json = await apiPost('/Client/', 'Client', {
    name: client.name || '',
    company: client.company || '',
    address: client.address || '',
    city: client.city || '',
    state: client.state || '',
    zip_code: client.zipCode || '',
    phone: client.phone || '',
    email: client.email || '',
    latitude: client.latitude || 0,
    longitude: client.longitude || 0,
  });
  const r = getDataObject(json);
  return r ? parseClient(r) : { ...client, id: 0 } as Client;
}

export async function updateClient(id: number, client: Partial<Client>): Promise<Client> {
  const json = await apiPatch(`/Client/${id}/`, id, 'Client', {
    name: client.name || '',
    company: client.company || '',
    address: client.address || '',
    city: client.city || '',
    state: client.state || '',
    zip_code: client.zipCode || '',
    phone: client.phone || '',
    email: client.email || '',
    latitude: client.latitude || 0,
    longitude: client.longitude || 0,
  });
  const r = getDataObject(json);
  return r ? parseClient(r) : { ...client, id } as Client;
}

export async function deleteClient(id: number): Promise<void> {
  await apiDelete(`/Client/${id}/`);
}

// ── Quotes ───────────────────────────────────────────────────

function parseQuote(r: JsonApiResource): Quote {
  return {
    id: Number(r.id),
    clientId: numAttr(r, 'client_id'),
    title: strAttr(r, 'title'),
    description: strAttr(r, 'description'),
    createdDate: strAttr(r, 'created_date'),
    expiryDate: strAttr(r, 'expiry_date'),
    status: numAttr(r, 'status'),
    taxRate: numAttr(r, 'tax_rate'),
    markupRate: numAttr(r, 'markup_rate'),
    notes: strAttr(r, 'notes'),
  };
}

export async function fetchQuotes(): Promise<Quote[]> {
  const json = await apiGet('/Quote/?page[limit]=1000&sort=-created_date');
  return getDataArray(json).map(parseQuote);
}

export async function fetchRecentQuotes(limit: number): Promise<Quote[]> {
  const json = await apiGet(`/Quote/?page[limit]=${limit}&sort=-created_date`);
  return getDataArray(json).map(parseQuote);
}

export async function fetchQuoteById(id: number): Promise<Quote | null> {
  try {
    const json = await apiGet(`/Quote/${id}/`);
    const r = getDataObject(json);
    return r ? parseQuote(r) : null;
  } catch {
    return null;
  }
}

export async function createQuote(quote: Partial<Quote>): Promise<Quote> {
  const attrs: Record<string, unknown> = {
    title: quote.title || '',
    description: quote.description || '',
    status: quote.status ?? 0,
    tax_rate: quote.taxRate ?? 0,
    markup_rate: quote.markupRate ?? 0,
    notes: quote.notes || '',
  };
  if (quote.clientId && quote.clientId > 0) attrs.client_id = quote.clientId;
  if (quote.createdDate) attrs.created_date = quote.createdDate;
  if (quote.expiryDate) attrs.expiry_date = quote.expiryDate;

  const json = await apiPost('/Quote/', 'Quote', attrs);
  const r = getDataObject(json);
  return r ? parseQuote(r) : { ...quote, id: 0 } as Quote;
}

export async function updateQuote(id: number, quote: Partial<Quote>): Promise<Quote> {
  const attrs: Record<string, unknown> = {
    title: quote.title || '',
    description: quote.description || '',
    status: quote.status ?? 0,
    tax_rate: quote.taxRate ?? 0,
    markup_rate: quote.markupRate ?? 0,
    notes: quote.notes || '',
  };
  if (quote.clientId && quote.clientId > 0) attrs.client_id = quote.clientId;
  if (quote.expiryDate) attrs.expiry_date = quote.expiryDate;

  const json = await apiPatch(`/Quote/${id}/`, id, 'Quote', attrs);
  const r = getDataObject(json);
  return r ? parseQuote(r) : { ...quote, id } as Quote;
}

export async function deleteQuote(id: number): Promise<void> {
  // Delete line items first
  const items = await fetchLineItemsByQuoteId(id);
  for (const li of items) {
    await deleteLineItem(li.id);
  }
  await apiDelete(`/Quote/${id}/`);
}

// ── Quote Line Items ─────────────────────────────────────────

function parseLineItem(r: JsonApiResource): QuoteLineItem {
  return {
    id: Number(r.id),
    quoteId: numAttr(r, 'quote_id'),
    productId: numAttr(r, 'product_id'),
    supplierId: numAttr(r, 'supplier_id'),
    quantity: numAttr(r, 'quantity'),
    unitPrice: numAttr(r, 'unit_price'),
    markup: numAttr(r, 'markup'),
    lineTotal: numAttr(r, 'line_total'),
    notes: strAttr(r, 'notes'),
  };
}

export async function fetchLineItemsByQuoteId(quoteId: number): Promise<QuoteLineItem[]> {
  const json = await apiGet(`/QuoteLineItem/?filter[quote_id]=${quoteId}&page[limit]=1000`);
  return getDataArray(json).map(parseLineItem);
}

export async function createLineItem(item: Partial<QuoteLineItem>): Promise<QuoteLineItem> {
  const lineTotal = (item.quantity || 0) * (item.unitPrice || 0) * (1 + (item.markup || 0) / 100);
  const attrs: Record<string, unknown> = {
    quote_id: item.quoteId,
    quantity: item.quantity || 0,
    unit_price: item.unitPrice || 0,
    markup: item.markup || 0,
    line_total: lineTotal,
    notes: item.notes || '',
  };
  if (item.productId && item.productId > 0) attrs.product_id = item.productId;
  if (item.supplierId && item.supplierId > 0) attrs.supplier_id = item.supplierId;

  const json = await apiPost('/QuoteLineItem/', 'QuoteLineItem', attrs);
  const r = getDataObject(json);
  return r ? parseLineItem(r) : { ...item, id: 0, lineTotal } as QuoteLineItem;
}

export async function updateLineItem(id: number, item: Partial<QuoteLineItem>): Promise<QuoteLineItem> {
  const lineTotal = (item.quantity || 0) * (item.unitPrice || 0) * (1 + (item.markup || 0) / 100);
  const attrs: Record<string, unknown> = {
    quote_id: item.quoteId,
    quantity: item.quantity || 0,
    unit_price: item.unitPrice || 0,
    markup: item.markup || 0,
    line_total: lineTotal,
    notes: item.notes || '',
  };
  if (item.productId && item.productId > 0) attrs.product_id = item.productId;
  if (item.supplierId && item.supplierId > 0) attrs.supplier_id = item.supplierId;

  const json = await apiPatch(`/QuoteLineItem/${id}/`, id, 'QuoteLineItem', attrs);
  const r = getDataObject(json);
  return r ? parseLineItem(r) : { ...item, id, lineTotal } as QuoteLineItem;
}

export async function deleteLineItem(id: number): Promise<void> {
  await apiDelete(`/QuoteLineItem/${id}/`);
}

// ── Dashboard Aggregates ─────────────────────────────────────

export async function fetchDashboardStats(): Promise<DashboardStats> {
  const [products, suppliers, clients, quotes] = await Promise.all([
    fetchProducts(),
    fetchSuppliers(),
    fetchClients(),
    fetchRecentQuotes(5),
  ]);

  const catCounts: Record<string, number> = {};
  for (const p of products) {
    catCounts[p.category] = (catCounts[p.category] || 0) + 1;
  }
  const categories: CategoryStat[] = Object.entries(catCounts)
    .map(([category, productCount]) => ({ category, productCount }))
    .sort((a, b) => a.category.localeCompare(b.category));

  return {
    productCount: products.length,
    supplierCount: suppliers.length,
    clientCount: clients.length,
    quoteCount: quotes.length,
    categories,
    recentQuotes: quotes,
  };
}
