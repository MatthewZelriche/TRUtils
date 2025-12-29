// NOLINTBEGIN(*-magic-numbers)
#include "trutils/type_erased_vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

using Catch::Generators::RandomIntegerGenerator;

TEST_CASE("Push, at, data, reserve, resize, pop", "[TypeErasedVector][basic]") {
   using namespace tr;

   auto v = TypeErasedVector::create<uint32_t>();
   REQUIRE(v.empty());

   // push 3 values using placement new into push_back_uninit
   void *p = v.push_back_uninit();
   new (p) uint32_t(10u);
   p = v.push_back_uninit();
   new (p) uint32_t(20u);
   p = v.push_back_uninit();
   new (p) uint32_t(30u);

   REQUIRE(v.size() == 3);
   auto span = v.data<uint32_t>();
   REQUIRE(span[0] == 10u);
   REQUIRE(span[1] == 20u);
   REQUIRE(span[2] == 30u);

   // at() reference assignment
   v.at<uint32_t>(1) = 42u;
   REQUIRE(v.at<uint32_t>(1) == 42u);

   // reserve and resize
   v.reserve(64);
   REQUIRE(v.capacity() >= 64);
   v.resize(5);
   REQUIRE(v.size() == 5);
   // set newly-resized elements
   v.at<uint32_t>(3) = 100u;
   v.at<uint32_t>(4) = 200u;
   // Check original elements, re-assigned and newly resized ones
   REQUIRE(v.at<uint32_t>(0) == 10u);
   REQUIRE(v.at<uint32_t>(1) == 42u);
   REQUIRE(v.at<uint32_t>(2) == 30u);
   REQUIRE(v.at<uint32_t>(3) == 100u);
   REQUIRE(v.at<uint32_t>(4) == 200u);

   // pop_back
   v.pop_back();
   REQUIRE(v.size() == 4);
   // Confirm span is also correct
   auto spn = v.data<uint32_t>();
   REQUIRE(spn.size() == 4);
   REQUIRE(spn[0] == 10u);
   REQUIRE(spn[1] == 42u);
   REQUIRE(spn[2] == 30u);
   REQUIRE(spn[3] == 100u);

   // popping empty container throws
   auto emptyVec = TypeErasedVector::create<int>();
   REQUIRE_THROWS(emptyVec.pop_back());
}

TEST_CASE("Swap and pop behavior", "[TypeErasedVector][swap]") {
   using namespace tr;
   auto v = TypeErasedVector::create<int>();
   for (int i = 0; i < 5; ++i) {
      void *p = v.push_back_uninit();
      new (p) int(i);
   }
   REQUIRE(v.size() == 5);

   // swap_and_pop index 1 should copy last element into position 1
   int last = v.at<int>(4);
   v.swap_and_pop(1);
   REQUIRE(v.size() == 4);
   REQUIRE(v.at<int>(1) == last);
}

TEST_CASE("Copy and move semantics", "[TypeErasedVector][copy_move]") {
   using namespace tr;

   auto a = TypeErasedVector::create<uint64_t>();
   for (uint64_t i = 1; i <= 3; ++i) {
      void *p = a.push_back_uninit();
      new (p) uint64_t(i * 10ull);
   }
   REQUIRE(a.size() == 3);

   // copy-construct
   TypeErasedVector b = a;
   REQUIRE(b.size() == 3);
   REQUIRE(b.at<uint64_t>(0) == 10ull);

   // move-construct
   TypeErasedVector c = std::move(a);
   REQUIRE(c.size() == 3);

   // move-assign
   TypeErasedVector d = TypeErasedVector::create<uint64_t>();
   d = std::move(c);
   REQUIRE(d.size() == 3);
}

TEST_CASE("Type-safety guard for assignment and const data() behavior",
          "[TypeErasedVector][safety][const]") {
   using namespace tr;

   auto u64 = TypeErasedVector::create<uint64_t>();
   void *p = u64.push_back_uninit();
   new (p) uint64_t(123ull);

   auto u32 = TypeErasedVector::create<uint32_t>();
   // assignment between different stored types must throw
   REQUIRE_THROWS_AS(u64 = u32, std::runtime_error);

   // const data() returns a view that can be read from a const reference
   const TypeErasedVector &cref = u64;
   auto cs = cref.data<uint64_t>();
   REQUIRE(cs.size() == 1);
   REQUIRE(cs[0] == 123ull);
}

// NOLINTEND(*-magic-numbers)