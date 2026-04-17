// NOLINTBEGIN

#include "trutils/table.hpp"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <tuple>

using namespace tr;

namespace {

struct Foo {
   int x {};
};

} // namespace

TEST_CASE("query_column returns matching cell references in order", "[table][query_column]") {
   table<> t;
   t.create_row<int>();
   t.create_row<double>();
   t.create_row<Foo>();

   const table<>::column_key col = t.insert_column();
   t.cell<int>(col) = 11;
   t.cell<double>(col) = 2.5;
   t.cell<Foo>(col).x = 99;

   auto tup = t.query_column<double, Foo, int>(col);
   REQUIRE(std::get<0>(tup) == 2.5);
   REQUIRE(std::get<1>(tup).x == 99);
   REQUIRE(std::get<2>(tup) == 11);

   std::get<0>(tup) = -1.0;
   REQUIRE(t.cell<double>(col) == -1.0);
}

TEST_CASE("query_column const overload", "[table][query_column]") {
   table<> t;
   t.create_row<int>();
   const table<>::column_key col = t.insert_column();
   t.cell<int>(col) = 100;

   const table<> &ct = t;
   auto tup = ct.query_column<int>(col);
   REQUIRE(std::get<0>(tup) == 100);
}

TEST_CASE("query_column empty pack yields empty tuple", "[table][query_column]") {
   table<> t;
   t.create_row<int>();
   const table<>::column_key col = t.insert_column();
   t.cell<int>(col) = 1;

   auto tup = t.query_column<>(col);
   static_assert(std::tuple_size_v<decltype(tup)> == 0);
   (void)tup;
}

TEST_CASE("query_column throws on invalid column key", "[table][query_column]") {
   table<> t;
   t.create_row<int>();
   (void)t.insert_column();

   const table<>::column_key bad {};
   REQUIRE_THROWS_AS(t.query_column<int>(bad), std::out_of_range);
}

TEST_CASE("query_column throws when a requested row type is missing", "[table][query_column]") {
   table<> t;
   t.create_row<int>();
   const table<>::column_key col = t.insert_column();
   t.cell<int>(col) = 1;

   REQUIRE_THROWS_AS((t.query_column<int, double>(col)), std::out_of_range);
}

TEST_CASE("get_row_view maps column_key to row cells and allows mutation",
          "[table][get_row_view]") {
   table<> t;
   t.create_row<int>();
   const table<>::column_key a = t.insert_column();
   const table<>::column_key b = t.insert_column();

   auto rv = t.get_row_view<int>();
   REQUIRE(rv.contains(a));
   REQUIRE(rv.contains(b));
   REQUIRE_FALSE(rv.contains(table<>::column_key {}));

   rv.at(a) = 7;
   rv.at(b) = 8;
   REQUIRE(t.cell<int>(a) == 7);
   REQUIRE(t.cell<int>(b) == 8);

   rv.at(a) = 100;
   REQUIRE(t.get_row<int>()[0] == 100);
   REQUIRE(rv.values().size() == 2);
}

TEST_CASE("get_row_view stays valid when columns are inserted or erased", "[table][get_row_view]") {
   table<> t;
   t.create_row<int>();
   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();
   t.cell<int>(k0) = 1;
   t.cell<int>(k1) = 2;

   auto rv = t.get_row_view<int>();
   REQUIRE(rv.values().size() == 2);

   REQUIRE(t.erase_column(k0));
   REQUIRE(rv.contains(k1));
   REQUIRE(rv.at(k1) == 2);
   REQUIRE(rv.values().size() == 1);

   const auto k2 = t.insert_column();
   t.cell<int>(k2) = 3;
   REQUIRE(rv.contains(k2));
   REQUIRE(rv.at(k2) == 3);
   REQUIRE(rv.values().size() == 2);
}

TEST_CASE("get_row_view stays valid when another row type is erased", "[table][get_row_view]") {
   table<> t;
   t.create_row<int>();
   t.create_row<double>();
   const auto k = t.insert_column();
   t.cell<int>(k) = 77;
   t.cell<double>(k) = 1.5;

   auto rv = t.get_row_view<int>();
   REQUIRE(t.erase_row<double>());
   REQUIRE(rv.at(k) == 77);
   REQUIRE(rv.values().size() == 1);
}

TEST_CASE("erase_column returns false for unknown key", "[table][erase_column]") {
   table<> t;
   t.create_row<int>();
   auto k = t.insert_column();
   REQUIRE_FALSE(t.erase_column(table<>::column_key {}));
}

TEST_CASE("erase_column removes column and keeps remaining keys aligned", "[table][erase_column]") {
   table<> t;
   t.create_row<int>();
   t.create_row<double>();

   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();
   const auto k2 = t.insert_column();

   t.cell<int>(k0) = 10;
   t.cell<int>(k1) = 20;
   t.cell<int>(k2) = 30;
   t.cell<double>(k0) = 1.0;
   t.cell<double>(k1) = 2.0;
   t.cell<double>(k2) = 3.0;

   REQUIRE(t.erase_column(k1));

   REQUIRE(t.cell<int>(k0) == 10);
   REQUIRE(t.cell<int>(k2) == 30);
   REQUIRE(t.cell<double>(k0) == 1.0);
   REQUIRE(t.cell<double>(k2) == 3.0);

   REQUIRE_THROWS_AS(t.cell<int>(k1), std::out_of_range);
   REQUIRE_THROWS_AS((t.query_column<int, double>(k1)), std::out_of_range);
}

TEST_CASE("erase_column first column moves successor data to dense index 0",
          "[table][erase_column]") {
   table<> t;
   t.create_row<int>();
   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();
   t.cell<int>(k0) = 100;
   t.cell<int>(k1) = 200;

   REQUIRE(t.erase_column(k0));

   REQUIRE(t.cell<int>(k1) == 200);
   REQUIRE(t.get_row<int>().size() == 1);
}

TEST_CASE("erase_column last column leaves prior columns unchanged", "[table][erase_column]") {
   table<> t;
   t.create_row<int>();
   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();
   t.cell<int>(k0) = 7;
   t.cell<int>(k1) = 8;

   REQUIRE(t.erase_column(k1));

   REQUIRE(t.cell<int>(k0) == 7);
   REQUIRE(t.get_row<int>().size() == 1);
}

TEST_CASE("erase_column with no rows only updates column mapping", "[table][erase_column]") {
   table<> t;
   const auto k = t.insert_column();
   REQUIRE(t.erase_column(k));
   const auto k2 = t.insert_column();
   t.create_row<int>();
   REQUIRE(t.get_row<int>().size() == 1);
}

TEST_CASE("column iterators expose insert_column keys in dense order", "[table][columns_iter]") {
   table<> t;
   t.create_row<int>();
   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();

   auto it = t.columns_begin<int>();
   REQUIRE((*it).first == k0);
   ++it;
   REQUIRE((*it).first == k1);
   ++it;
   REQUIRE(it == t.columns_end<int>());
}

TEST_CASE("columns_begin/end iterates all columns as query_column tuples",
          "[table][columns_iter]") {
   table<> t;
   t.create_row<int>();
   t.create_row<double>();
   const auto k0 = t.insert_column();
   const auto k1 = t.insert_column();
   t.cell<int>(k0) = 1;
   t.cell<double>(k0) = 1.5;
   t.cell<int>(k1) = 2;
   t.cell<double>(k1) = 2.5;

   auto it = t.columns_begin<double, int>();
   REQUIRE(it != t.columns_end<double, int>());
   {
      auto [col_key, tup] = *it;
      REQUIRE(col_key == k0);
      REQUIRE(std::get<0>(tup) == 1.5);
      REQUIRE(std::get<1>(tup) == 1);
      std::get<0>(tup) = -1.0;
   }
   ++it;
   REQUIRE(it != t.columns_end<double, int>());
   {
      auto [col_key, tup] = *it;
      REQUIRE(col_key == k1);
      REQUIRE(std::get<0>(tup) == 2.5);
      REQUIRE(std::get<1>(tup) == 2);
   }
   ++it;
   REQUIRE(it == t.columns_end<double, int>());
   REQUIRE(t.cell<double>(k0) == -1.0);
}

TEST_CASE("columns_begin const yields const cell references", "[table][columns_iter]") {
   table<> t;
   t.create_row<int>();
   const auto k = t.insert_column();
   t.cell<int>(k) = 42;
   const table<> &ct = t;
   auto it = ct.columns_begin<int>();
   const auto entry = *it;
   REQUIRE(entry.first == k);
   REQUIRE(std::get<0>(entry.second) == 42);
}

TEST_CASE("columns_begin empty range when no columns", "[table][columns_iter]") {
   table<> t;
   t.create_row<int>();
   REQUIRE(t.columns_begin<int>() == t.columns_end<int>());
}

// NOLINTEND
