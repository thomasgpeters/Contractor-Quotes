import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { fetchDashboardStats } from '../services/api';
import { QuoteStatusLabels } from '../types/models';
import type { DashboardStats } from '../types/models';

export default function Dashboard() {
  const [stats, setStats] = useState<DashboardStats | null>(null);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  useEffect(() => {
    fetchDashboardStats()
      .then(setStats)
      .catch((e) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  if (loading) return <div className="page"><div className="loading">Loading...</div></div>;
  if (error) return <div className="page"><div className="error-banner">{error}</div></div>;
  if (!stats) return null;

  return (
    <div className="page">
      <h1 className="page-title">Dashboard</h1>

      <div className="stat-grid">
        <div className="stat-card" onClick={() => navigate('/products')}>
          <div className="stat-value">{stats.productCount}</div>
          <div className="stat-label">Products</div>
        </div>
        <div className="stat-card" onClick={() => navigate('/suppliers')}>
          <div className="stat-value">{stats.supplierCount}</div>
          <div className="stat-label">Suppliers</div>
        </div>
        <div className="stat-card" onClick={() => navigate('/clients')}>
          <div className="stat-value">{stats.clientCount}</div>
          <div className="stat-label">Clients</div>
        </div>
        <div className="stat-card" onClick={() => navigate('/quotes')}>
          <div className="stat-value">{stats.quoteCount}</div>
          <div className="stat-label">Quotes</div>
        </div>
      </div>

      <h2 className="section-title">Categories</h2>
      <div className="card-list">
        {stats.categories.map((cat) => (
          <div key={cat.category} className="list-item">
            <span className="list-item-title">{cat.category}</span>
            <span className="badge">{cat.productCount}</span>
          </div>
        ))}
      </div>

      <h2 className="section-title">Recent Quotes</h2>
      <div className="card-list">
        {stats.recentQuotes.length === 0 && (
          <div className="empty-state">No quotes yet</div>
        )}
        {stats.recentQuotes.map((q) => (
          <div key={q.id} className="list-item" onClick={() => navigate('/quotes')}>
            <div>
              <div className="list-item-title">{q.title || `Quote #${q.id}`}</div>
              <div className="list-item-subtitle">
                {QuoteStatusLabels[q.status] || 'Draft'} &middot; {q.createdDate?.split('T')[0] || ''}
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
