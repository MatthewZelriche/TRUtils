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

TEST_CASE("untyped_vector push_back_uninit and copy/move", "[untyped_vector]") {
   untyped_vector vec(getTypeInfo<long long>());
   REQUIRE(vec.size() == 0);

   vec.push_back_uninit();
   vec.push_back_uninit();
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

// NOLINTEND