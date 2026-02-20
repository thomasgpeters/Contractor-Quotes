import { useState, useEffect, useCallback } from 'react';
import { fetchClients, createClient, updateClient, deleteClient } from '../services/api';
import type { Client } from '../types/models';

const emptyClient: Partial<Client> = {
  name: '', company: '', address: '', city: '', state: '',
  zipCode: '', phone: '', email: '', latitude: 0, longitude: 0,
};

export default function Clients() {
  const [clients, setClients] = useState<Client[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [editing, setEditing] = useState<Partial<Client> | null>(null);
  const [editId, setEditId] = useState<number | null>(null);

  const load = useCallback(() => {
    setLoading(true);
    fetchClients()
      .then(setClients)
      .catch((e) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => { load(); }, [load]);

  const handleSave = async () => {
    if (!editing) return;
    try {
      if (editId) {
        await updateClient(editId, editing);
      } else {
        await createClient(editing);
      }
      setEditing(null);
      setEditId(null);
      load();
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Save failed');
    }
  };

  const handleDelete = async (id: number) => {
    if (!confirm('Delete this client and all their quotes?')) return;
    try {
      await deleteClient(id);
      load();
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Delete failed');
    }
  };

  const field = (label: string, key: keyof Client, type = 'text') => (
    <div className="form-field">
      <label>{label}</label>
      <input
        type={type}
        value={String(editing?.[key] ?? '')}
        onChange={(e) => setEditing({ ...editing, [key]: type === 'number' ? Number(e.target.value) : e.target.value })}
      />
    </div>
  );

  if (editing) {
    return (
      <div className="page">
        <div className="page-header">
          <button className="btn-text" onClick={() => { setEditing(null); setEditId(null); }}>Cancel</button>
          <h1 className="page-title">{editId ? 'Edit Client' : 'New Client'}</h1>
          <button className="btn-primary" onClick={handleSave}>Save</button>
        </div>
        <div className="form">
          {field('Name', 'name')}
          {field('Company', 'company')}
          {field('Email', 'email')}
          {field('Phone', 'phone')}
          {field('Address', 'address')}
          {field('City', 'city')}
          {field('State', 'state')}
          {field('Zip Code', 'zipCode')}
        </div>
      </div>
    );
  }

  return (
    <div className="page">
      <div className="page-header">
        <h1 className="page-title">Clients</h1>
        <button className="btn-primary" onClick={() => { setEditing({ ...emptyClient }); setEditId(null); }}>
          + Add
        </button>
      </div>

      {error && <div className="error-banner">{error}</div>}
      {loading && <div className="loading">Loading...</div>}

      <div className="card-list">
        {clients.length === 0 && !loading && (
          <div className="empty-state">No clients yet. Tap + Add to create one.</div>
        )}
        {clients.map((c) => (
          <div key={c.id} className="list-item">
            <div className="list-item-content" onClick={() => { setEditing({ ...c }); setEditId(c.id); }}>
              <div className="list-item-title">{c.name}</div>
              <div className="list-item-subtitle">
                {c.company ? `${c.company} - ` : ''}{c.city}, {c.state}
              </div>
            </div>
            <button className="btn-danger-sm" onClick={() => handleDelete(c.id)}>Delete</button>
          </div>
        ))}
      </div>
    </div>
  );
}
