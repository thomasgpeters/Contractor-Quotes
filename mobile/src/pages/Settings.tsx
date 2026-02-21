import { useState } from 'react';
import { getApiBaseUrl, setApiBaseUrl } from '../services/api';

export default function Settings() {
  const [url, setUrl] = useState(getApiBaseUrl());
  const [saved, setSaved] = useState(false);

  const handleSave = () => {
    setApiBaseUrl(url);
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  return (
    <div className="page">
      <h1 className="page-title">Settings</h1>

      <div className="form">
        <div className="form-field">
          <label>API Server URL</label>
          <input
            value={url}
            onChange={(e) => setUrl(e.target.value)}
            placeholder="http://localhost:5667/api"
          />
          <div className="form-hint">
            The base URL of the ApiLogicServer REST API. Change this to point to your server.
          </div>
        </div>
        <button className="btn-primary full-width" onClick={handleSave}>
          {saved ? 'Saved!' : 'Save'}
        </button>
      </div>

      <div className="detail-card" style={{ marginTop: '2rem' }}>
        <h2 className="section-title">About</h2>
        <div className="detail-row">
          <span className="detail-label">App</span>
          <span>Contractor Quotes Mobile</span>
        </div>
        <div className="detail-row">
          <span className="detail-label">Version</span>
          <span>1.0.0</span>
        </div>
        <div className="detail-row">
          <span className="detail-label">Platform</span>
          <span>React + Capacitor</span>
        </div>
      </div>
    </div>
  );
}
