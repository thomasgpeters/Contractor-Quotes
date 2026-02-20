#pragma once

#include <vector>
#include <string>
#include <Wt/Dbo/Session.h>
#include <Wt/Dbo/ptr.h>

class Product;
class Supplier;
class SupplierProduct;

/// Result of evaluating a single supplier for a specific product request.
struct SourcingResult {
    Wt::Dbo::ptr<SupplierProduct> supplierProduct;
    std::string supplierName;
    double      unitPrice       = 0.0;
    double      effectivePrice  = 0.0; // after bulk discount
    int         availableQty    = 0;
    bool        meetsQty        = false;
    bool        inStock         = false;
    double      distanceMiles   = 0.0;
    double      supplierRating  = 0.0;
    int         leadTimeDays    = 0;
    double      compositeScore  = 0.0; // overall ranking score (lower is better)
};

/// Weights for the multi-criteria scoring function.
struct SourcingWeights {
    double priceWeight     = 0.40;
    double availWeight     = 0.25;
    double proximityWeight = 0.20;
    double qualityWeight   = 0.15;
};

/// Core engine that evaluates multiple supplier sources for a product request
/// and ranks them by a weighted composite of price, availability, proximity,
/// and quality.
class SourcingEngine {
public:
    explicit SourcingEngine(Wt::Dbo::Session& session);

    /// Find and rank all suppliers for the given product and quantity.
    /// @param productId   The database ID of the product to source.
    /// @param quantity     Required quantity.
    /// @param jobLat       Latitude of the job site (for proximity).
    /// @param jobLon       Longitude of the job site (for proximity).
    /// @param weights      Scoring weights.
    /// @return Sorted vector of results (best first).
    std::vector<SourcingResult> findBestSources(
        long long productId,
        int quantity,
        double jobLat,
        double jobLon,
        const SourcingWeights& weights = SourcingWeights{}
    );

    /// Compute the distance in miles between two lat/lon points (Haversine).
    static double haversineDistance(double lat1, double lon1,
                                   double lat2, double lon2);

private:
    Wt::Dbo::Session& session_;

    /// Normalize a value into [0,1] range given min and max observed values.
    static double normalize(double value, double minVal, double maxVal);
};
