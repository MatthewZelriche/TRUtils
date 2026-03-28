// NOLINTBEGIN

#include "trutils/simple_flatmap.hpp"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace tr;

namespace {

template<typename Key, typename T, typename Hash, typename KeyEqual>
void expect_matches_ref(const simple_flatmap<Key, T, Hash, KeyEqual> &map,
                        const std::unordered_map<Key, T, Hash, KeyEqual> &ref) {
   REQUIRE(map.size() == ref.size());
   REQUIRE(map.empty() == ref.empty());
   for (const auto &[k, v] : ref) {
      REQUIRE(map.contains(k));
      REQUIRE(map.at(k) == v);
   }
}

} // namespace

TEST_CASE("simple_flatmap default is empty", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   REQUIRE(map.empty());
   REQUIRE(map.size() == 0);
   REQUIRE_FALSE(map.contains(0));
}

TEST_CASE("simple_flatmap clear", "[simple_flatmap]") {
   simple_flatmap<int, std::string> map;
   map[1] = "a";
   map[2] = "b";
   REQUIRE(map.size() == 2);
   map.clear();
   REQUIRE(map.empty());
   REQUIRE_FALSE(map.contains(1));
}

TEST_CASE("simple_flatmap operator[] inserts default and allows assign", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   map[7] = 42;
   REQUIRE(map.contains(7));
   REQUIRE(map[7] == 42);
   map[7] = 99;
   REQUIRE(map.at(7) == 99);

   int &r1 = map[3];
   int &r2 = map[3];
   REQUIRE(&r1 == &r2);
   REQUIRE(r1 == 0);
   r1 = 5;
   REQUIRE(map.at(3) == 5);
}

TEST_CASE("simple_flatmap at throws when missing", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   map[1] = 2;
   REQUIRE_THROWS_AS(map.at(99), std::out_of_range);
   const auto &cm = map;
   REQUIRE_THROWS_AS(cm.at(99), std::out_of_range);
}

TEST_CASE("simple_flatmap insert lvalue", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   int v = 100;
   auto [ok1, ref1] = map.insert(1, v);
   REQUIRE(ok1);
   REQUIRE(ref1 == 100);
   REQUIRE(&ref1 == &map.at(1));

   auto [ok2, ref2] = map.insert(1, 200);
   REQUIRE_FALSE(ok2);
   REQUIRE(ref2 == 100);
   REQUIRE(&ref1 == &ref2);
   REQUIRE(map.at(1) == 100);
}

TEST_CASE("simple_flatmap insert rvalue", "[simple_flatmap]") {
   simple_flatmap<int, std::string> map;
   std::string s = "hello";
   auto [ok, ref] = map.insert(1, std::move(s));
   REQUIRE(ok);
   REQUIRE(ref == "hello");
   auto [ok2, ref2] = map.insert(1, std::string("world"));
   REQUIRE_FALSE(ok2);
   REQUIRE(ref2 == "hello");
}

TEST_CASE("simple_flatmap erase missing returns false", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   REQUIRE_FALSE(map.erase(0));
   map[0] = 1;
   REQUIRE(map.erase(0));
   REQUIRE_FALSE(map.erase(0));
}

TEST_CASE("simple_flatmap erase preserves index and iteration", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   map.insert(1, 10);
   map.insert(2, 20);
   map.insert(3, 30);

   std::unordered_map<int, int> ref{{1, 10}, {2, 20}, {3, 30}};
   expect_matches_ref(map, ref);

   REQUIRE(map.erase(2));
   ref.erase(2);
   expect_matches_ref(map, ref);

   map.insert(4, 40);
   ref[4] = 40;
   expect_matches_ref(map, ref);

   REQUIRE(map.erase(1));
   ref.erase(1);
   expect_matches_ref(map, ref);
}

TEST_CASE("simple_flatmap iteration matches stored pairs", "[simple_flatmap]") {
   simple_flatmap<int, std::string> map;
   map.insert(2, "b");
   map.insert(5, "e");
   map.insert(1, "a");

   std::unordered_map<int, std::string> seen;
   for (const auto &[k, v] : map) { seen[k] = v; }
   REQUIRE(seen.size() == 3);
   REQUIRE(seen[1] == "a");
   REQUIRE(seen[2] == "b");
   REQUIRE(seen[5] == "e");
}

TEST_CASE("simple_flatmap const iteration", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   map[0] = 1;
   const auto &cm = map;
   int sum = 0;
   for (const auto &[k, v] : cm) {
      sum += k + v;
   }
   REQUIRE(sum == 1);
}

TEST_CASE("simple_flatmap reserve then grow", "[simple_flatmap]") {
   simple_flatmap<int, int> map;
   map.reserve(64);
   for (int i = 0; i < 50; ++i) { map.insert(i, i * i); }
   REQUIRE(map.size() == 50);
   for (int i = 0; i < 50; ++i) { REQUIRE(map.at(i) == i * i); }
}

struct Point {
   int x {};
   int y {};
   bool operator==(const Point &o) const { return x == o.x && y == o.y; }
};

struct PointHash {
   std::size_t operator()(const Point &p) const noexcept {
      return std::hash<int> {}(p.x) ^ (std::hash<int> {}(p.y) << 1);
   }
};

TEST_CASE("simple_flatmap custom hash and equality", "[simple_flatmap]") {
   simple_flatmap<Point, int, PointHash> map;
   Point a {1, 2};
   Point b {3, 4};
   map.insert(a, 10);
   map.insert(b, 20);
   REQUIRE(map.at(a) == 10);
   REQUIRE(map.at(b) == 20);
   REQUIRE(map.contains(Point {1, 2}));
}


// NOLINTEND