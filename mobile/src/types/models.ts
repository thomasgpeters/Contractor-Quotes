// TypeScript interfaces matching the C++ DTOs in include/data/DataTypes.h

export interface Product {
  id: number;
  name: string;
  category: string;
  unit: string;
  specifications: string;
  sku: string;
  supplierCount?: number;
  minPrice?: number;
  maxPrice?: number;
}

export interface Supplier {
  id: number;
  name: string;
  address: string;
  city: string;
  state: string;
  zipCode: string;
  phone: string;
  email: string;
  website: string;
  latitude: number;
  longitude: number;
  rating: number;
  leadTimeDays: number;
  productCount?: number;
}

export interface SupplierProduct {
  id: number;
  productId: number;
  supplierId: number;
  unitPrice: number;
  stockQty: number;
  inStock: boolean;
  canBackorder: boolean;
  minOrderQty: number;
  bulkDiscount: number;
  bulkThreshold: number;
  lastUpdated: string;
  productName?: string;
  productSku?: string;
  productCategory?: string;
  supplierName?: string;
  supplierRating?: number;
  supplierLeadDays?: number;
  supplierLat?: number;
  supplierLon?: number;
}

export interface Client {
  id: number;
  name: string;
  company: string;
  address: string;
  city: string;
  state: string;
  zipCode: string;
  phone: string;
  email: string;
  latitude: number;
  longitude: number;
  quoteCount?: number;
}

export const QuoteStatus = {
  Draft: 0,
  Sent: 1,
  Accepted: 2,
  Rejected: 3,
  Expired: 4,
} as const;

export const QuoteStatusLabels: Record<number, string> = {
  0: 'Draft',
  1: 'Sent',
  2: 'Accepted',
  3: 'Rejected',
  4: 'Expired',
};

export interface Quote {
  id: number;
  clientId: number;
  title: string;
  description: string;
  createdDate: string;
  expiryDate: string;
  status: number;
  taxRate: number;
  markupRate: number;
  notes: string;
  clientName?: string;
  totalAmount?: number;
  lineItemCount?: number;
}

export interface QuoteLineItem {
  id: number;
  quoteId: number;
  productId: number;
  supplierId: number;
  quantity: number;
  unitPrice: number;
  markup: number;
  lineTotal: number;
  notes: string;
  productName?: string;
  productUnit?: string;
  supplierName?: string;
}

export interface CategoryStat {
  category: string;
  productCount: number;
}

export interface DashboardStats {
  productCount: number;
  supplierCount: number;
  clientCount: number;
  quoteCount: number;
  categories: CategoryStat[];
  recentQuotes: Quote[];
}
