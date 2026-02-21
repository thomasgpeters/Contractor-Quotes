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

      <div className="dashboard-card">
        <h2 className="section-title">Product Categories</h2>
        <table className="dashboard-table">
          <thead>
            <tr>
              <th>Category</th>
              <th>Products</th>
              <th>Suppliers</th>
            </tr>
          </thead>
          <tbody>
            {stats.categories.map((cat) => (
              <tr key={cat.category}>
                <td>{cat.category}</td>
                <td>{cat.productCount}</td>
                <td>{cat.supplierCount}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="dashboard-card">
        <h2 className="section-title">Recent Quotes</h2>
        {stats.recentQuotes.length === 0 ? (
          <div className="empty-state">No quotes yet</div>
        ) : (
          <table className="dashboard-table">
            <thead>
              <tr>
                <th>Title</th>
                <th>Status</th>
                <th>Date</th>
              </tr>
            </thead>
            <tbody>
              {stats.recentQuotes.map((q) => (
                <tr key={q.id} onClick={() => navigate('/quotes')} style={{ cursor: 'pointer' }}>
                  <td>{q.title || `Quote #${q.id}`}</td>
                  <td>{QuoteStatusLabels[q.status] || 'Draft'}</td>
                  <td>{q.createdDate?.split('T')[0] || ''}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}
