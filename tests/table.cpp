// NOLINTBEGIN
#include "trutils/table.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

using Catch::Generators::RandomIntegerGenerator;

TEST_CASE("Basic Sanity Test", "[Table]") {
   using namespace tr;
   Table table;
}

// NOLINTEND