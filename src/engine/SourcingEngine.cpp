#include "engine/SourcingEngine.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"
#include <cmath>
#include <algorithm>
#include <limits>

SourcingEngine::SourcingEngine(Wt::Dbo::Session& session)
    : session_(session)
{
}

std::vector<SourcingResult> SourcingEngine::findBestSources(
    long long productId,
    int quantity,
    double jobLat,
    double jobLon,
    const SourcingWeights& weights)
{
    Wt::Dbo::Transaction t(session_);

    auto product = session_.find<Product>().where("id = ?").bind(productId).resultValue();
    if (!product) return {};

    // Gather all supplier offerings for this product
    std::vector<SourcingResult> results;

    auto offerings = session_.find<SupplierProduct>()
        .where("product_id = ?").bind(productId)
        .resultList();

    for (auto& sp : offerings) {
        SourcingResult r;
        r.supplierProduct = sp;
        r.supplierName    = sp->supplier->name;
        r.unitPrice       = sp->unitPrice;
        r.availableQty    = sp->stockQty;
        r.inStock         = sp->inStock;
        r.supplierRating  = sp->supplier->rating;
        r.leadTimeDays    = sp->supplier->leadTimeDays;

        // Check if quantity is achievable
        r.meetsQty = (sp->stockQty >= quantity) ||
                     (sp->canBackorder && quantity >= sp->minOrderQty);

        // Compute effective price with bulk discount
        if (sp->bulkDiscount > 0 && quantity >= sp->bulkThreshold) {
            r.effectivePrice = sp->unitPrice * (1.0 - sp->bulkDiscount / 100.0);
        } else {
            r.effectivePrice = sp->unitPrice;
        }

        // Compute distance from job site to supplier
        r.distanceMiles = haversineDistance(
            jobLat, jobLon,
            sp->supplier->latitude, sp->supplier->longitude
        );

        results.push_back(r);
    }

    if (results.empty()) return results;

    // Find min/max for normalization
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = 0;
    double minDist  = std::numeric_limits<double>::max();
    double maxDist  = 0;
    double minRating = 5.0;
    double maxRating = 0.0;
    double minLead  = std::numeric_limits<double>::max();
    double maxLead  = 0;

    for (auto& r : results) {
        minPrice  = std::min(minPrice,  r.effectivePrice);
        maxPrice  = std::max(maxPrice,  r.effectivePrice);
        minDist   = std::min(minDist,   r.distanceMiles);
        maxDist   = std::max(maxDist,   r.distanceMiles);
        minRating = std::min(minRating, r.supplierRating);
        maxRating = std::max(maxRating, r.supplierRating);
        minLead   = std::min(minLead,   static_cast<double>(r.leadTimeDays));
        maxLead   = std::max(maxLead,   static_cast<double>(r.leadTimeDays));
    }

    // Compute composite score for each result
    for (auto& r : results) {
        // Price score: lower is better → normalize so lowest gets 0
        double priceScore = normalize(r.effectivePrice, minPrice, maxPrice);

        // Availability score: in-stock and meets quantity = 0 (best), else penalized
        double availScore = 0.0;
        if (!r.meetsQty)    availScore += 0.5;
        if (!r.inStock)     availScore += 0.5;

        // Proximity score: closer is better → normalize distance
        double proxScore = normalize(r.distanceMiles, minDist, maxDist);

        // Quality score: higher rating is better → invert
        double qualScore = 1.0 - normalize(r.supplierRating, minRating, maxRating);

        // Include lead time as part of availability
        double leadScore = normalize(static_cast<double>(r.leadTimeDays), minLead, maxLead);
        availScore = availScore * 0.7 + leadScore * 0.3;

        // Weighted composite (lower is better)
        r.compositeScore = weights.priceWeight     * priceScore
                         + weights.availWeight     * availScore
                         + weights.proximityWeight * proxScore
                         + weights.qualityWeight   * qualScore;
    }

    // Sort: best (lowest composite) first, but prioritize those that meet qty
    std::sort(results.begin(), results.end(), [](const SourcingResult& a, const SourcingResult& b) {
        // Suppliers meeting quantity always rank above those that don't
        if (a.meetsQty != b.meetsQty) return a.meetsQty;
        return a.compositeScore < b.compositeScore;
    });

    return results;
}

double SourcingEngine::haversineDistance(double lat1, double lon1,
                                        double lat2, double lon2)
{
    constexpr double R = 3958.8; // Earth radius in miles
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2)
             + std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0)
             * std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

double SourcingEngine::normalize(double value, double minVal, double maxVal)
{
    if (maxVal - minVal < 1e-9) return 0.0;
    return (value - minVal) / (maxVal - minVal);
}
