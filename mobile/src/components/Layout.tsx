import { useState } from 'react';
import { NavLink, Outlet } from 'react-router-dom';

const tabs = [
  { to: '/dashboard', label: 'Home', icon: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6' },
  { to: '/clients', label: 'Clients', icon: 'M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0z' },
  { to: '/products', label: 'Products', icon: 'M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4' },
  { to: '/suppliers', label: 'Suppliers', icon: 'M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4' },
  { to: '/quotes', label: 'Quotes', icon: 'M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z' },
];

export default function Layout() {
  const [showAbout, setShowAbout] = useState(false);

  return (
    <div className="app-layout">
      <header className="app-header">
        <img src="/images/imagery_logo.png" alt="Imagery" className="app-header-logo logo-light" />
        <img src="/images/imagery_logo_white.png" alt="Imagery" className="app-header-logo logo-dark" />
        <span className="app-header-title">Contractor Quotes</span>
        <button className="about-btn" onClick={() => setShowAbout(true)} aria-label="About">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth={2} strokeLinecap="round" strokeLinejoin="round" width="20" height="20">
            <circle cx="12" cy="12" r="10" />
            <line x1="12" y1="16" x2="12" y2="12" />
            <line x1="12" y1="8" x2="12.01" y2="8" />
          </svg>
        </button>
      </header>

      {showAbout && (
        <div className="about-overlay" onClick={() => setShowAbout(false)}>
          <div className="about-modal" onClick={(e) => e.stopPropagation()}>
            <img src="/images/imagery_logo.png" alt="Imagery Business Systems" className="about-modal-logo logo-light" />
            <img src="/images/imagery_logo_white.png" alt="Imagery Business Systems" className="about-modal-logo logo-dark" />
            <h2>Contractor Quotes &amp; Sourcing</h2>
            <p>A comprehensive quoting and material sourcing platform for contractors. Build accurate quotes with automatic best-source supplier selection, real-time pricing, and intelligent inventory-aware recommendations.</p>
            <p className="about-copy">&copy; 2026 Imagery Business Systems. All rights reserved.</p>
            <a href="https://imagery-business-systems.com" target="_blank" rel="noopener noreferrer">imagery-business-systems.com</a>
            <button className="btn-primary full-width" style={{ marginTop: '1rem' }} onClick={() => setShowAbout(false)}>Close</button>
          </div>
        </div>
      )}

      <main className="app-main">
        <Outlet />
      </main>
      <nav className="tab-bar">
        {tabs.map((tab) => (
          <NavLink
            key={tab.to}
            to={tab.to}
            className={({ isActive }) => `tab-item ${isActive ? 'active' : ''}`}
          >
            <svg className="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth={1.5} strokeLinecap="round" strokeLinejoin="round">
              <path d={tab.icon} />
            </svg>
            <span className="tab-label">{tab.label}</span>
          </NavLink>
        ))}
      </nav>
    </div>
  );
}
