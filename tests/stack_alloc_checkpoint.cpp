// NOLINTBEGIN

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <memory_resource>

#include "trutils/stack_alloc.hpp"

using namespace tr;

TEST_CASE("StackAllocCheckpoint rewinds bump pointer", "[stack_alloc][checkpoint]") {
   StackAlloc<> sa;
   auto outer = sa.allocArr<int>(4);
   outer[0] = 1;
   outer[1] = 2;
   outer[2] = 3;
   outer[3] = 4;

   {
      StackAllocCheckpoint cp(sa);
      auto inner = sa.allocArr<int>(10);
      for (int i = 0; i < 10; ++i) { inner[static_cast<size_t>(i)] = i * 100; }
      REQUIRE(inner[0] == 0);
      REQUIRE(inner[9] == 900);
   }

   REQUIRE(outer[0] == 1);
   REQUIRE(outer[3] == 4);

   auto after = sa.allocArr<int>(2);
   after[0] = 7;
   after[1] = 8;
   REQUIRE(after[0] == 7);
   REQUIRE(after[1] == 8);
}

TEST_CASE("StackAllocCheckpoint nested scopes", "[stack_alloc][checkpoint]") {
   StackAlloc<512> sa;
   auto base = sa.allocArr<int>(1);
   base[0] = 42;

   {
      StackAllocCheckpoint outer(sa);
      auto mid = sa.allocArr<int>(8);
      mid[0] = -1;

      {
         StackAllocCheckpoint inner(sa);
         auto leaf = sa.allocArr<int>(16);
         leaf[15] = 99;
         REQUIRE(leaf[15] == 99);
      }

      REQUIRE(mid[0] == -1);
   }

   REQUIRE(base[0] == 42);
}

TEST_CASE("StackAlloc keeps first-block pointers valid when a new block is appended",
          "[stack_alloc][stability]") {
   constexpr size_t kBlock = 256;
   StackAlloc<kBlock> sa;

   constexpr size_t kUsable = kBlock - 2 * sizeof(size_t);
   constexpr size_t kChunk = kUsable / 2;
   const auto align = alignof(std::max_align_t);

   auto *p0 = static_cast<unsigned char *>(sa.allocate(kChunk, align));
   REQUIRE(p0 != nullptr);
   std::memset(p0, 0xAB, kChunk);

   auto *p1 = static_cast<unsigned char *>(sa.allocate(kChunk, align));
   REQUIRE(p1 != nullptr);
   REQUIRE(p1 != p0);

   REQUIRE(p0[0] == 0xAB);
   REQUIRE(p0[kChunk - 1] == 0xAB);
}

TEST_CASE("StackAllocCheckpoint drops trailing blocks", "[stack_alloc][checkpoint]") {
   constexpr size_t kBlock = 256;
   StackAlloc<kBlock> sa;

   constexpr size_t kUsable = kBlock - 2 * sizeof(size_t);
   constexpr size_t kChunk = kUsable / 2;
   const auto align = alignof(std::max_align_t);

   auto *p0 = static_cast<unsigned char *>(sa.allocate(kChunk, align));
   REQUIRE(p0 != nullptr);
   std::memset(p0, 0xAB, kChunk);

   {
      StackAllocCheckpoint cp(sa);
      auto *p1 = static_cast<unsigned char *>(sa.allocate(kChunk, align));
      REQUIRE(p1 != nullptr);
      auto *p2 = static_cast<unsigned char *>(sa.allocate(kChunk, align));
      REQUIRE(p2 != nullptr);
      std::memset(p1, 0xCD, kChunk);
      std::memset(p2, 0xEF, kChunk);
   }

   REQUIRE(p0[0] == 0xAB);
   REQUIRE(p0[kChunk - 1] == 0xAB);

   auto *pAgain = static_cast<unsigned char *>(sa.allocate(8, 8));
   REQUIRE(pAgain != nullptr);
}

TEST_CASE("StackAlloc allocArr with default fill", "[stack_alloc]") {
   StackAlloc<> sa;
   auto s = sa.allocArr<int>(6, -3);
   for (size_t i = 0; i < 6; ++i) { REQUIRE(s[i] == -3); }
}

TEST_CASE("StackAlloc works with pmr::polymorphic_allocator", "[stack_alloc][pmr]") {
   StackAlloc<> sa;
   std::pmr::polymorphic_allocator<int> pa {&sa};
   int *p = pa.allocate(32);
   p[0] = 123;
   p[31] = 456;
   REQUIRE(p[0] == 123);
   REQUIRE(p[31] == 456);
   pa.deallocate(p, 32);
}

TEST_CASE("StackAllocCheckpoint with pmr::vector inside scope", "[stack_alloc][pmr][checkpoint]") {
   StackAlloc<> sa;
   std::pmr::vector<int> persistent(&sa);
   persistent.push_back(10);
   persistent.push_back(20);

   {
      StackAllocCheckpoint cp(sa);
      std::pmr::vector<int> scratch(&sa);
      for (int i = 0; i < 50; ++i) { scratch.push_back(i); }
      REQUIRE(scratch.size() == 50U);
      REQUIRE(scratch.back() == 49);
   }

   REQUIRE(persistent.size() == 2U);
   REQUIRE(persistent[0] == 10);
   REQUIRE(persistent[1] == 20);
}

TEST_CASE("StackAlloc do_is_equal identity", "[stack_alloc][pmr]") {
   StackAlloc<> a;
   StackAlloc<> b;
   REQUIRE(a.is_equal(a));
   REQUIRE_FALSE(a.is_equal(b));
}

// NOLINTEND
