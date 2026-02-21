import { useState, useEffect, useCallback } from 'react';
import {
  fetchQuotes, fetchClients, fetchProducts,
  createQuote, updateQuote, deleteQuote,
  fetchLineItemsByQuoteId, createLineItem, deleteLineItem,
} from '../services/api';
import { QuoteStatusLabels } from '../types/models';
import type { Quote, Client, Product, QuoteLineItem } from '../types/models';

type View = 'list' | 'detail' | 'edit' | 'add-item';

export default function QuoteBuilder() {
  const [quotes, setQuotes] = useState<Quote[]>([]);
  const [clients, setClients] = useState<Client[]>([]);
  const [products, setProducts] = useState<Product[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [view, setView] = useState<View>('list');

  // Detail / Edit state
  const [selectedQuote, setSelectedQuote] = useState<Quote | null>(null);
  const [lineItems, setLineItems] = useState<QuoteLineItem[]>([]);
  const [editQuote, setEditQuote] = useState<Partial<Quote>>({});

  // Add line item state
  const [newItem, setNewItem] = useState<Partial<QuoteLineItem>>({
    productId: 0, quantity: 1, unitPrice: 0, markup: 0, notes: '',
  });

  const loadQuotes = useCallback(() => {
    setLoading(true);
    Promise.all([fetchQuotes(), fetchClients(), fetchProducts()])
      .then(([q, c, p]) => { setQuotes(q); setClients(c); setProducts(p); })
      .catch((e) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => { loadQuotes(); }, [loadQuotes]);

  const openDetail = async (quote: Quote) => {
    setSelectedQuote(quote);
    setView('detail');
    try {
      const items = await fetchLineItemsByQuoteId(quote.id);
      setLineItems(items);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Failed to load line items');
    }
  };

  const openEdit = (quote?: Quote) => {
    if (quote) {
      setEditQuote({ ...quote });
    } else {
      setEditQuote({
        title: '', description: '', clientId: 0,
        status: 0, taxRate: 8.25, markupRate: 15, notes: '',
      });
    }
    setView('edit');
  };

  const handleSaveQuote = async () => {
    try {
      if (editQuote.id) {
        await updateQuote(editQuote.id, editQuote);
      } else {
        const created = await createQuote(editQuote);
        setSelectedQuote(created);
      }
      loadQuotes();
      setView('list');
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Save failed');
    }
  };

  const handleDeleteQuote = async (id: number) => {
    if (!confirm('Delete this quote and all line items?')) return;
    try {
      await deleteQuote(id);
      setSelectedQuote(null);
      loadQuotes();
      setView('list');
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Delete failed');
    }
  };

  const handleAddItem = async () => {
    if (!selectedQuote) return;
    try {
      await createLineItem({ ...newItem, quoteId: selectedQuote.id });
      const items = await fetchLineItemsByQuoteId(selectedQuote.id);
      setLineItems(items);
      setNewItem({ productId: 0, quantity: 1, unitPrice: 0, markup: 0, notes: '' });
      setView('detail');
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Failed to add item');
    }
  };

  const handleDeleteItem = async (itemId: number) => {
    if (!selectedQuote) return;
    try {
      await deleteLineItem(itemId);
      const items = await fetchLineItemsByQuoteId(selectedQuote.id);
      setLineItems(items);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Delete failed');
    }
  };

  const getClientName = (clientId: number) =>
    clients.find((c) => c.id === clientId)?.name || '';

  const lineTotal = lineItems.reduce((sum, li) => sum + li.lineTotal, 0);

  // ── Add Line Item view ─────────────────────────────────────
  if (view === 'add-item') {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => setView('detail')}>Cancel</button>
          <h1 className="page-title">Add Item</h1>
          <button className="btn-primary" onClick={handleAddItem}>Add</button>
        </div>
        <div className="form">
          <div className="form-field">
            <label>Product</label>
            <select
              value={newItem.productId || 0}
              onChange={(e) => setNewItem({ ...newItem, productId: Number(e.target.value) })}
            >
              <option value={0}>Select product...</option>
              {products.map((p) => (
                <option key={p.id} value={p.id}>{p.name} ({p.sku})</option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label>Quantity</label>
            <input type="number" value={newItem.quantity || 0}
              onChange={(e) => setNewItem({ ...newItem, quantity: Number(e.target.value) })} />
          </div>
          <div className="form-field">
            <label>Unit Price</label>
            <input type="number" step="0.01" value={newItem.unitPrice || 0}
              onChange={(e) => setNewItem({ ...newItem, unitPrice: Number(e.target.value) })} />
          </div>
          <div className="form-field">
            <label>Markup (%)</label>
            <input type="number" step="0.5" value={newItem.markup || 0}
              onChange={(e) => setNewItem({ ...newItem, markup: Number(e.target.value) })} />
          </div>
          <div className="form-field">
            <label>Notes</label>
            <textarea value={newItem.notes || ''}
              onChange={(e) => setNewItem({ ...newItem, notes: e.target.value })} />
          </div>
        </div>
      </div>
    );
  }

  // ── Edit / New Quote view ──────────────────────────────────
  if (view === 'edit') {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => setView(selectedQuote ? 'detail' : 'list')}>Cancel</button>
          <h1 className="page-title">{editQuote.id ? 'Edit Quote' : 'New Quote'}</h1>
          <button className="btn-primary" onClick={handleSaveQuote}>Save</button>
        </div>
        <div className="form">
          <div className="form-field">
            <label>Title</label>
            <input value={editQuote.title || ''}
              onChange={(e) => setEditQuote({ ...editQuote, title: e.target.value })} />
          </div>
          <div className="form-field">
            <label>Client</label>
            <select
              value={editQuote.clientId || 0}
              onChange={(e) => setEditQuote({ ...editQuote, clientId: Number(e.target.value) })}
            >
              <option value={0}>Select client...</option>
              {clients.map((c) => (
                <option key={c.id} value={c.id}>{c.name}{c.company ? ` (${c.company})` : ''}</option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label>Status</label>
            <select
              value={editQuote.status ?? 0}
              onChange={(e) => setEditQuote({ ...editQuote, status: Number(e.target.value) })}
            >
              {Object.entries(QuoteStatusLabels).map(([v, l]) => (
                <option key={v} value={v}>{l}</option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label>Tax Rate (%)</label>
            <input type="number" step="0.25" value={editQuote.taxRate ?? 0}
              onChange={(e) => setEditQuote({ ...editQuote, taxRate: Number(e.target.value) })} />
          </div>
          <div className="form-field">
            <label>Markup Rate (%)</label>
            <input type="number" step="0.5" value={editQuote.markupRate ?? 0}
              onChange={(e) => setEditQuote({ ...editQuote, markupRate: Number(e.target.value) })} />
          </div>
          <div className="form-field">
            <label>Description</label>
            <textarea value={editQuote.description || ''}
              onChange={(e) => setEditQuote({ ...editQuote, description: e.target.value })} />
          </div>
          <div className="form-field">
            <label>Notes</label>
            <textarea value={editQuote.notes || ''}
              onChange={(e) => setEditQuote({ ...editQuote, notes: e.target.value })} />
          </div>
        </div>
      </div>
    );
  }

  // ── Quote Detail view ──────────────────────────────────────
  if (view === 'detail' && selectedQuote) {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => { setView('list'); setSelectedQuote(null); }}>Back</button>
          <h1 className="page-title">Quote #{selectedQuote.id}</h1>
          <button className="btn-text" onClick={() => openEdit(selectedQuote)}>Edit</button>
        </div>
        <div className="detail-card">
          <div className="detail-row"><span className="detail-label">Title</span><span>{selectedQuote.title}</span></div>
          <div className="detail-row"><span className="detail-label">Client</span><span>{getClientName(selectedQuote.clientId) || '—'}</span></div>
          <div className="detail-row"><span className="detail-label">Status</span><span className="badge">{QuoteStatusLabels[selectedQuote.status]}</span></div>
          <div className="detail-row"><span className="detail-label">Created</span><span>{selectedQuote.createdDate?.split('T')[0]}</span></div>
          <div className="detail-row"><span className="detail-label">Tax</span><span>{selectedQuote.taxRate}%</span></div>
          <div className="detail-row"><span className="detail-label">Markup</span><span>{selectedQuote.markupRate}%</span></div>
        </div>

        <div className="page-header" style={{ marginTop: '1rem' }}>
          <h2 className="section-title">Line Items ({lineItems.length})</h2>
          <button className="btn-primary" onClick={() => setView('add-item')}>+ Item</button>
        </div>

        <div className="card-list">
          {lineItems.map((li) => (
            <div key={li.id} className="list-item">
              <div>
                <div className="list-item-title">{li.productName || `Product #${li.productId}`}</div>
                <div className="list-item-subtitle">
                  {li.quantity} x ${li.unitPrice.toFixed(2)} = ${li.lineTotal.toFixed(2)}
                </div>
              </div>
              <button className="btn-danger-sm" onClick={() => handleDeleteItem(li.id)}>X</button>
            </div>
          ))}
          {lineItems.length === 0 && <div className="empty-state">No line items. Tap + Item to add.</div>}
        </div>

        <div className="total-bar">
          <span>Subtotal</span>
          <span className="total-amount">${lineTotal.toFixed(2)}</span>
        </div>

        <button className="btn-danger full-width" onClick={() => handleDeleteQuote(selectedQuote.id)}>
          Delete Quote
        </button>
      </div>
    );
  }

  // ── Quote List view ────────────────────────────────────────
  return (
    <div className="page">
      <div className="page-header">
        <h1 className="page-title">Quotes</h1>
        <button className="btn-primary" onClick={() => openEdit()}>+ New</button>
      </div>

      {error && <div className="error-banner">{error}</div>}
      {loading && <div className="loading">Loading...</div>}

      <div className="card-list">
        {quotes.length === 0 && !loading && (
          <div className="empty-state">No quotes yet. Tap + New to create one.</div>
        )}
        {quotes.map((q) => (
          <div key={q.id} className="list-item" onClick={() => openDetail(q)}>
            <div>
              <div className="list-item-title">{q.title || `Quote #${q.id}`}</div>
              <div className="list-item-subtitle">
                {getClientName(q.clientId) || 'No client'} &middot;
                {QuoteStatusLabels[q.status]} &middot;
                {q.createdDate?.split('T')[0] || ''}
              </div>
            </div>
            <svg className="chevron" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth={2}>
              <path d="M9 5l7 7-7 7" />
            </svg>
          </div>
        ))}
      </div>
    </div>
  );
}
