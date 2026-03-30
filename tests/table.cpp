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
   table t;
   t.createRow<int>();
   t.createRow<double>();
   t.createRow<Foo>();

   const table::column_key col = t.insert_column();
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
   table t;
   t.createRow<int>();
   const table::column_key col = t.insert_column();
   t.cell<int>(col) = 100;

   const table &ct = t;
   auto tup = ct.query_column<int>(col);
   REQUIRE(std::get<0>(tup) == 100);
}

TEST_CASE("query_column empty pack yields empty tuple", "[table][query_column]") {
   table t;
   t.createRow<int>();
   const table::column_key col = t.insert_column();
   t.cell<int>(col) = 1;

   auto tup = t.query_column<>(col);
   static_assert(std::tuple_size_v<decltype(tup)> == 0);
   (void)tup;
}

TEST_CASE("query_column throws on invalid column key", "[table][query_column]") {
   table t;
   t.createRow<int>();
   (void)t.insert_column();

   const table::column_key bad {};
   REQUIRE_THROWS_AS(t.query_column<int>(bad), std::out_of_range);
}

TEST_CASE("query_column throws when a requested row type is missing", "[table][query_column]") {
   table t;
   t.createRow<int>();
   const table::column_key col = t.insert_column();
   t.cell<int>(col) = 1;

   REQUIRE_THROWS_AS((t.query_column<int, double>(col)), std::out_of_range);
}

TEST_CASE("row_view maps column_key to row cells and allows mutation", "[table][row_view]") {
   table t;
   t.createRow<int>();
   const table::column_key a = t.insert_column();
   const table::column_key b = t.insert_column();

   auto rv = t.row_view<int>();
   REQUIRE(rv.contains(a));
   REQUIRE(rv.contains(b));
   REQUIRE_FALSE(rv.contains(table::column_key {}));

   rv.at(a) = 7;
   rv.at(b) = 8;
   REQUIRE(t.cell<int>(a) == 7);
   REQUIRE(t.cell<int>(b) == 8);

   rv.at(a) = 100;
   REQUIRE(t.row<int>()[0] == 100);
   REQUIRE(rv.values().size() == 2);
}

TEST_CASE("row_view const aliases read-only cells", "[table][row_view]") {
   table t;
   t.createRow<int>();
   const table::column_key col = t.insert_column();
   t.cell<int>(col) = 42;

   const table &ct = t;
   auto rv = ct.row_view<int>();
   REQUIRE(rv.at(col) == 42);
}

// NOLINTEND
