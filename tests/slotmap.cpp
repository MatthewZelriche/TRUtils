// NOLINTBEGIN(*-magic-numbers)

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "trutils/slot_map.hpp"

using Catch::Generators::RandomIntegerGenerator;

namespace {
template<typename uint64_t, typename K>
void check_iter(tr::SlotMap<K, uint64_t> &map, std::unordered_map<K, uint64_t> &refData) {
   for (auto [key, value] : refData) {
      auto slotmapVal = map.get(key);
      REQUIRE(*slotmapVal == value);
   }
}

template<typename uint64_t, typename K>
void fill(tr::SlotMap<K, uint64_t> &map, std::unordered_map<K, uint64_t> &refData, size_t count) {
   RandomIntegerGenerator<uint64_t> valGen(0, UINT64_MAX, 0);
   for (size_t i = 0; i < count; i++) {
      auto val = valGen.get();
      valGen.next();
      refData.insert({map.insert(val), val});
   }

   REQUIRE(map.size() == refData.size());
   check_iter(map, refData);
}

template<typename uint64_t, typename K>
void drain(tr::SlotMap<K, uint64_t> &map, std::unordered_map<K, uint64_t> &refData, size_t count) {
   std::unordered_set<K> removed;
   for (size_t i = 0; i < count; i++) {
      auto idx = RandomIntegerGenerator<uint64_t>(0, refData.size() - 1, (uint32_t)i).get();
      auto iter = refData.begin();
      std::advance(iter, idx);
      auto [key, val] = *iter;
      refData.erase(iter);
      removed.insert(key);

      // Ensure the returned value is correct
      REQUIRE(map.remove(key) == val);
   }

   // Ensure the removed keys are really gone
   for (auto el : removed) { REQUIRE_FALSE(map.contains(el)); }

   REQUIRE(map.size() == refData.size());
   check_iter(map, refData);
}
} // namespace

TEST_CASE("Basic Sanity Tests For SlotMap", "[SlotMap]") {
   const size_t MAX_EL = 5000;
   using namespace tr;

   SlotMap<Key<DefaultTag>, uint64_t> map;
   std::unordered_map<Key<DefaultTag>, uint64_t> data;
   REQUIRE(map.size() == 0);

   // Perform many insertions
   fill(map, data, MAX_EL);

   // Remove MAXE_EL / 2 elements at random
   drain(map, data, MAX_EL / 2);

   // Insert MAX_EL more items for a total of MAX_EL * 1.5
   fill(map, data, MAX_EL);

   // Now remove every element in a random order
   drain(map, data, MAX_EL + (MAX_EL / 2));

   // Add a few more in and remove them, just to be safe...
   fill(map, data, 50);

   drain(map, data, 30);

   // Test clearing the data structure
   map.clear();
   data.clear();
   REQUIRE(map.size() == 0);

   // Now stress test with a few larger insertions and deletions
   fill(map, data, 10000);

   drain(map, data, 5000);

   drain(map, data, 4999);

   drain(map, data, 1);
}

// NOLINTEND(*-magic-numbers)