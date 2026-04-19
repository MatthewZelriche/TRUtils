// NOLINTBEGIN
#include "trutils/untyped_vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

using namespace tr;

TEST_CASE("untyped_vector basic operations", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());

   REQUIRE(vec.empty());
   REQUIRE(vec.size() == 0);
   REQUIRE(vec.capacity() == 0);
   REQUIRE(vec.element_size() == sizeof(int));
   REQUIRE(vec.type_hash() == getTypeID<int>());

   vec.reserve(5);
   REQUIRE(vec.capacity() >= 5);

   vec.push_back<int>(10);
   vec.push_back<int>(20);
   vec.push_back<int>(30);

   REQUIRE_FALSE(vec.empty());
   REQUIRE(vec.size() == 3);
   REQUIRE(vec.front<int>() == 10);
   REQUIRE(vec.back<int>() == 30);
   REQUIRE(vec.at<int>(0) == 10);
   REQUIRE(vec.at<int>(1) == 20);
   REQUIRE(vec.at<int>(2) == 30);

   auto span = vec.data<int>();
   REQUIRE(span.size() == 3);
   REQUIRE(span[0] == 10);
   span[1] = 42;
   REQUIRE(vec.at<int>(1) == 42);

   vec.pop_back();
   REQUIRE(vec.size() == 2);
   REQUIRE(vec.back<int>() == 42);

   vec.clear();
   REQUIRE(vec.empty());
}

TEST_CASE("untyped_vector push_back_default and copy/move", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<long long>());
   REQUIRE(vec.size() == 0);

   vec.push_back_default();
   vec.push_back_default();
   REQUIRE(vec.size() == 2);

   vec.at<long long>(0) = 123456789012345LL;
   vec.at<long long>(1) = 98765432109876LL;

   REQUIRE(vec.front<long long>() == 123456789012345LL);
   REQUIRE(vec.back<long long>() == 98765432109876LL);

   untyped_vector vec_copy = vec;
   REQUIRE(vec_copy.size() == 2);
   REQUIRE(vec_copy.at<long long>(0) == 123456789012345LL);
   REQUIRE(vec_copy.at<long long>(1) == 98765432109876LL);

   untyped_vector vec_move = std::move(vec);
   REQUIRE(vec_move.size() == 2);
   REQUIRE(vec_move.at<long long>(0) == 123456789012345LL);
   REQUIRE(vec_move.at<long long>(1) == 98765432109876LL);
}

TEST_CASE("untyped_vector type mismatch and out-of-range errors", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back<int>(100);

   REQUIRE_THROWS_AS(vec.push_back<double>(0.5), std::runtime_error);
   REQUIRE_THROWS_AS(vec.at<double>(0), std::runtime_error);
   REQUIRE_THROWS_AS(vec.front<double>(), std::runtime_error);
   REQUIRE_THROWS_AS(vec.back<double>(), std::runtime_error);
   REQUIRE_THROWS_AS(vec.data<double>(), std::runtime_error);

   REQUIRE_THROWS_AS(vec.at<int>(1), std::out_of_range);
   REQUIRE_THROWS_AS(vec.at<int>(100), std::out_of_range);
   REQUIRE(vec.at<int>(0) == 100);
}

struct alignas(16) AlignedType {
   int value;
};

TEST_CASE("untyped_vector supports unusual alignment", "[untyped_vector][alignment]") {
   untyped_vector vec(getTypeInfo<AlignedType>());

   REQUIRE(vec.element_size() == sizeof(AlignedType));
   REQUIRE(vec.type_hash() == getTypeID<AlignedType>());

   vec.push_back<AlignedType>({42});
   vec.push_back<AlignedType>({99});

   REQUIRE(vec.size() == 2);
   REQUIRE(vec.at<AlignedType>(0).value == 42);
   REQUIRE(vec.at<AlignedType>(1).value == 99);

   vec.pop_back();
   REQUIRE(vec.size() == 1);
   REQUIRE(vec.back<AlignedType>().value == 42);

   vec.clear();
   REQUIRE(vec.empty());
}

TEST_CASE("untyped_vector resize grow default constructs new elements", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.resize<int>(4);
   REQUIRE(vec.size() == 4);
   for (size_t i = 0; i < 4; ++i) { REQUIRE(vec.at<int>(i) == 0); }

   vec.at<int>(0) = 100;
   vec.at<int>(3) = 200;
   vec.resize<int>(6);
   REQUIRE(vec.size() == 6);
   REQUIRE(vec.at<int>(0) == 100);
   REQUIRE(vec.at<int>(1) == 0);
   REQUIRE(vec.at<int>(3) == 200);
   REQUIRE(vec.at<int>(4) == 0);
   REQUIRE(vec.at<int>(5) == 0);
}

TEST_CASE("untyped_vector resize grow with explicit fill value", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back<int>(1);
   vec.push_back<int>(2);
   vec.resize<int>(5, 99);
   REQUIRE(vec.size() == 5);
   REQUIRE(vec.at<int>(0) == 1);
   REQUIRE(vec.at<int>(1) == 2);
   REQUIRE(vec.at<int>(2) == 99);
   REQUIRE(vec.at<int>(3) == 99);
   REQUIRE(vec.at<int>(4) == 99);
}

TEST_CASE("untyped_vector resize shrink truncates", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   for (int i = 0; i < 10; ++i) { vec.push_back<int>(i); }
   vec.resize<int>(3);
   REQUIRE(vec.size() == 3);
   REQUIRE(vec.at<int>(0) == 0);
   REQUIRE(vec.at<int>(1) == 1);
   REQUIRE(vec.at<int>(2) == 2);
}

TEST_CASE("untyped_vector resize same size is no-op", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back<int>(7);
   vec.push_back<int>(8);
   const size_t cap = vec.capacity();
   vec.resize<int>(2);
   REQUIRE(vec.size() == 2);
   REQUIRE(vec.at<int>(0) == 7);
   REQUIRE(vec.at<int>(1) == 8);
   REQUIRE(vec.capacity() == cap);
}

TEST_CASE("untyped_vector resize to zero clears", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back<int>(1);
   vec.resize<int>(0);
   REQUIRE(vec.empty());
}

TEST_CASE("untyped_vector resize type mismatch throws", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back<int>(1);
   REQUIRE_THROWS_AS(vec.resize<double>(3), std::runtime_error);
}

TEST_CASE("untyped_vector erase without element type at call site", "[untyped_vector][erase]") {
   untyped_vector vec(getTypeInfo<int>());
   vec.push_back_default();
   vec.push_back_default();
   vec.push_back_default();
   vec.at<int>(0) = 10;
   vec.at<int>(1) = 20;
   vec.at<int>(2) = 30;

   vec.swap_and_pop(2);
   REQUIRE(vec.size() == 2);
   REQUIRE(vec.at<int>(0) == 10);
   REQUIRE(vec.at<int>(1) == 20);

   vec.swap_and_pop(0);
   REQUIRE(vec.size() == 1);
   REQUIRE(vec.at<int>(0) == 20);

   vec.swap_and_pop(0);
   REQUIRE(vec.empty());

   REQUIRE_THROWS_AS(vec.swap_and_pop(0), std::out_of_range);
}

// NOLINTEND