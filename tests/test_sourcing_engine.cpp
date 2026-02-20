#define BOOST_TEST_MODULE SourcingEngineTests
#include <boost/test/unit_test.hpp>

#include "engine/SourcingEngine.h"
#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"

#include <cmath>
#include <cstdio>

struct DatabaseFixture {
    Session session;

    DatabaseFixture() : session(":memory:") {
        session.seedIfEmpty();
    }
};

BOOST_FIXTURE_TEST_SUITE(SourcingEngineTestSuite, DatabaseFixture)

BOOST_AUTO_TEST_CASE(haversine_same_point_is_zero)
{
    double d = SourcingEngine::haversineDistance(30.267, -97.743, 30.267, -97.743);
    BOOST_CHECK_CLOSE(d, 0.0, 0.001);
}

BOOST_AUTO_TEST_CASE(haversine_known_distance)
{
    // Austin TX to Dallas TX ~ 195 miles
    double d = SourcingEngine::haversineDistance(30.267, -97.743, 32.783, -96.797);
    BOOST_CHECK(d > 180.0 && d < 210.0);
}

BOOST_AUTO_TEST_CASE(find_best_sources_returns_results)
{
    Wt::Dbo::Transaction t(session.dbo());

    auto product = session.dbo().find<Product>().where("sku = ?").bind("LBR-2408").resultValue();
    BOOST_REQUIRE(product);

    SourcingEngine engine(session.dbo());
    auto results = engine.findBestSources(product.id(), 10, 30.267, -97.743);

    BOOST_CHECK(!results.empty());
    t.commit();
}

BOOST_AUTO_TEST_CASE(results_are_sorted_by_composite_score)
{
    Wt::Dbo::Transaction t(session.dbo());

    auto product = session.dbo().find<Product>().where("sku = ?").bind("LBR-2408").resultValue();
    BOOST_REQUIRE(product);

    SourcingEngine engine(session.dbo());
    auto results = engine.findBestSources(product.id(), 10, 30.267, -97.743);

    // Verify suppliers meeting qty come first, then sorted by score
    bool seenNotMeeting = false;
    double prevScore = -1.0;

    for (const auto& r : results) {
        if (!r.meetsQty) {
            seenNotMeeting = true;
        } else {
            BOOST_CHECK(!seenNotMeeting);
            if (prevScore >= 0) {
                BOOST_CHECK(r.compositeScore >= prevScore - 0.0001);
            }
            prevScore = r.compositeScore;
        }
    }
    t.commit();
}

BOOST_AUTO_TEST_CASE(best_source_has_lowest_composite_score)
{
    Wt::Dbo::Transaction t(session.dbo());

    auto product = session.dbo().find<Product>().where("sku = ?").bind("CON-80NM").resultValue();
    BOOST_REQUIRE(product);

    SourcingEngine engine(session.dbo());
    auto results = engine.findBestSources(product.id(), 5, 29.760, -95.370);

    if (results.size() >= 2 && results[0].meetsQty && results[1].meetsQty) {
        BOOST_CHECK(results[0].compositeScore <= results[1].compositeScore);
    }
    t.commit();
}

BOOST_AUTO_TEST_CASE(custom_weights_change_ranking)
{
    Wt::Dbo::Transaction t(session.dbo());

    auto product = session.dbo().find<Product>().where("sku = ?").bind("LBR-2408").resultValue();
    BOOST_REQUIRE(product);

    SourcingEngine engine(session.dbo());

    // Price-heavy weighting
    SourcingWeights priceWeights{0.80, 0.10, 0.05, 0.05};
    auto priceResults = engine.findBestSources(product.id(), 10, 30.267, -97.743, priceWeights);

    // Proximity-heavy weighting
    SourcingWeights proxWeights{0.05, 0.10, 0.80, 0.05};
    auto proxResults = engine.findBestSources(product.id(), 10, 30.267, -97.743, proxWeights);

    BOOST_CHECK(!priceResults.empty());
    BOOST_CHECK(!proxResults.empty());

    // Different weights must produce different composite scores for at least
    // one supplier that appears in both result sets.
    bool foundDifference = false;
    for (const auto& pr : priceResults) {
        for (const auto& xr : proxResults) {
            if (pr.supplierName == xr.supplierName) {
                if (std::fabs(pr.compositeScore - xr.compositeScore) > 0.0001) {
                    foundDifference = true;
                    break;
                }
            }
        }
        if (foundDifference) break;
    }
    BOOST_CHECK(foundDifference);
    t.commit();
}

BOOST_AUTO_TEST_CASE(nonexistent_product_returns_empty)
{
    Wt::Dbo::Transaction t(session.dbo());

    SourcingEngine engine(session.dbo());
    auto results = engine.findBestSources(99999, 10, 30.267, -97.743);

    BOOST_CHECK(results.empty());
    t.commit();
}

BOOST_AUTO_TEST_SUITE_END()
