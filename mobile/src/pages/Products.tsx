import { useState, useEffect } from 'react';
import { fetchProducts, fetchSupplierProductsByProductId } from '../services/api';
import type { Product, SupplierProduct } from '../types/models';

export default function Products() {
  const [products, setProducts] = useState<Product[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [filter, setFilter] = useState('');
  const [selectedProduct, setSelectedProduct] = useState<Product | null>(null);
  const [supplierProducts, setSupplierProducts] = useState<SupplierProduct[]>([]);
  const [loadingSP, setLoadingSP] = useState(false);

  useEffect(() => {
    fetchProducts()
      .then(setProducts)
      .catch((e) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  const showSuppliers = async (product: Product) => {
    setSelectedProduct(product);
    setLoadingSP(true);
    try {
      const sp = await fetchSupplierProductsByProductId(product.id);
      setSupplierProducts(sp);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Failed to load suppliers');
    }
    setLoadingSP(false);
  };

  const categories = [...new Set(products.map((p) => p.category))].sort();
  const filtered = filter
    ? products.filter((p) => p.category === filter)
    : products;

  if (selectedProduct) {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => setSelectedProduct(null)}>Back</button>
          <h1 className="page-title">{selectedProduct.name}</h1>
          <div />
        </div>
        <div className="detail-card">
          <div className="detail-row"><span className="detail-label">SKU</span><span>{selectedProduct.sku}</span></div>
          <div className="detail-row"><span className="detail-label">Category</span><span>{selectedProduct.category}</span></div>
          <div className="detail-row"><span className="detail-label">Unit</span><span>{selectedProduct.unit}</span></div>
          <div className="detail-row"><span className="detail-label">Specs</span><span>{selectedProduct.specifications}</span></div>
        </div>

        <h2 className="section-title">Suppliers ({supplierProducts.length})</h2>
        {loadingSP && <div className="loading">Loading...</div>}
        <div className="card-list">
          {supplierProducts.map((sp) => (
            <div key={sp.id} className="list-item">
              <div>
                <div className="list-item-title">${sp.unitPrice.toFixed(2)}</div>
                <div className="list-item-subtitle">
                  {sp.inStock ? 'In Stock' : 'Out of Stock'}
                  {sp.stockQty > 0 ? ` (${sp.stockQty} units)` : ''}
                  {sp.canBackorder ? ' - Backorder OK' : ''}
                </div>
              </div>
              <span className="badge">{sp.inStock ? 'Available' : 'N/A'}</span>
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="page">
      <h1 className="page-title">Products</h1>
      {error && <div className="error-banner">{error}</div>}

      <div className="filter-bar">
        <button
          className={`filter-chip ${!filter ? 'active' : ''}`}
          onClick={() => setFilter('')}
        >
          All
        </button>
        {categories.map((cat) => (
          <button
            key={cat}
            className={`filter-chip ${filter === cat ? 'active' : ''}`}
            onClick={() => setFilter(cat)}
          >
            {cat}
          </button>
        ))}
      </div>

      {loading && <div className="loading">Loading...</div>}

      <div className="card-list">
        {filtered.map((p) => (
          <div key={p.id} className="list-item" onClick={() => showSuppliers(p)}>
            <div>
              <div className="list-item-title">{p.name}</div>
              <div className="list-item-subtitle">{p.sku} &middot; {p.category} &middot; {p.unit}</div>
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
