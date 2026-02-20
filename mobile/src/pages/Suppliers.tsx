import { useState, useEffect } from 'react';
import { fetchSuppliers, fetchSupplierProductsBySupplierId } from '../services/api';
import type { Supplier, SupplierProduct } from '../types/models';

export default function Suppliers() {
  const [suppliers, setSuppliers] = useState<Supplier[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [selected, setSelected] = useState<Supplier | null>(null);
  const [products, setProducts] = useState<SupplierProduct[]>([]);
  const [loadingProducts, setLoadingProducts] = useState(false);

  useEffect(() => {
    fetchSuppliers()
      .then(setSuppliers)
      .catch((e) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  const showProducts = async (supplier: Supplier) => {
    setSelected(supplier);
    setLoadingProducts(true);
    try {
      const sp = await fetchSupplierProductsBySupplierId(supplier.id);
      setProducts(sp);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Failed to load products');
    }
    setLoadingProducts(false);
  };

  const renderStars = (rating: number) => {
    const full = Math.floor(rating);
    const half = rating - full >= 0.5;
    return (
      <span className="stars">
        {'★'.repeat(full)}{half ? '½' : ''}{'☆'.repeat(5 - full - (half ? 1 : 0))}
        <span className="rating-num">({rating.toFixed(1)})</span>
      </span>
    );
  };

  if (selected) {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => setSelected(null)}>Back</button>
          <h1 className="page-title">{selected.name}</h1>
          <div />
        </div>
        <div className="detail-card">
          <div className="detail-row"><span className="detail-label">Rating</span>{renderStars(selected.rating)}</div>
          <div className="detail-row"><span className="detail-label">Lead Time</span><span>{selected.leadTimeDays} days</span></div>
          <div className="detail-row"><span className="detail-label">Location</span><span>{selected.city}, {selected.state}</span></div>
          <div className="detail-row"><span className="detail-label">Phone</span><span>{selected.phone}</span></div>
          <div className="detail-row"><span className="detail-label">Email</span><span>{selected.email}</span></div>
        </div>

        <h2 className="section-title">Products ({products.length})</h2>
        {loadingProducts && <div className="loading">Loading...</div>}
        <div className="card-list">
          {products.map((sp) => (
            <div key={sp.id} className="list-item">
              <div>
                <div className="list-item-title">${sp.unitPrice.toFixed(2)}</div>
                <div className="list-item-subtitle">
                  {sp.inStock ? 'In Stock' : 'Out of Stock'}
                  {sp.stockQty > 0 ? ` (${sp.stockQty})` : ''}
                </div>
              </div>
              <span className={`badge ${sp.inStock ? 'badge-green' : 'badge-red'}`}>
                {sp.inStock ? 'Available' : 'N/A'}
              </span>
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="page">
      <h1 className="page-title">Suppliers</h1>
      {error && <div className="error-banner">{error}</div>}
      {loading && <div className="loading">Loading...</div>}

      <div className="card-list">
        {suppliers.map((s) => (
          <div key={s.id} className="list-item" onClick={() => showProducts(s)}>
            <div>
              <div className="list-item-title">{s.name}</div>
              <div className="list-item-subtitle">
                {s.city}, {s.state} &middot; {renderStars(s.rating)} &middot; {s.leadTimeDays}d lead
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
