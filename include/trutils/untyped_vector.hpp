#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "type_id.hpp"

namespace tr {

template<typename T>
concept trivially_destructible = std::is_trivially_destructible_v<T>;

/// @brief A non-templated vector that stores data in a type-erased manner.
///
/// Allows storing elements without needing to know the type at the call site
/// for most operations (like reserve, size, clear, etc.). Type-dependent
/// operations (like at, operator[], push_back) are templated methods that
/// verify the type at compile-time to prevent type errors.
class untyped_vector {
  public:
   /// @brief Constructor that initializes with a specific type
   explicit untyped_vector(const ty_info &type_info) :
       mTypeInfo(type_info),
       mAlignedSz((type_info.size + type_info.alignment - 1) & -type_info.alignment) {
      assert(type_info.alignment > 0);
   }

   /// @brief Destructor
   ~untyped_vector() = default;

   /// @brief Copy constructor
   untyped_vector(const untyped_vector &other) = default;

   /// @brief Move constructor
   untyped_vector(untyped_vector &&other) noexcept = default;

   /// @brief Copy assignment
   untyped_vector &operator=(const untyped_vector &other) = default;

   /// @brief Move assignment
   untyped_vector &operator=(untyped_vector &&other) noexcept = default;

   /// @brief Returns the number of elements stored
   size_t size() const { return mData.size() / mAlignedSz; }

   /// @brief Returns the allocated capacity in terms of elements
   size_t capacity() const { return mData.capacity() / mAlignedSz; }

   /// @brief Returns true if the vector is empty
   bool empty() const { return mData.empty(); }

   /// @brief Reserves capacity for at least the specified number of elements
   void reserve(size_t new_capacity) { mData.reserve(new_capacity * mAlignedSz); }

   /// @brief Clears all elements from the vector
   void clear() { mData.clear(); }

   /// @brief Removes the last element
   void pop_back() {
      if (!mData.empty()) { mData.resize(mData.size() - mAlignedSz); }
   }

   /// @brief Exchanges the contents of slots @p i and @p j without knowing the stored type.
   ///
   /// Trivially destructible elements are swapped by exchanging each byte of the two slots in place.
   /// @throws std::out_of_range if @p i or @p j is >= size().
   void swap(size_t i, size_t j) noexcept(false) {
      const size_t n = size();
      if (i >= n || j >= n) { throw std::out_of_range("untyped_vector::swap"); }
      if (i == j) { return; }
      std::byte *const a = element_ptr(i);
      std::byte *const b = element_ptr(j);
      for (size_t k = 0; k < mAlignedSz; ++k) { std::swap(a[k], b[k]); }
   }

   /// @brief Swaps the element at index with the last element and pops the last element.
   /// @throws std::out_of_range if index >= size().
   void swap_and_pop(size_t index) {
      const size_t n = size();
      if (index >= n) { throw std::out_of_range("untyped_vector::erase"); }
      if (index + 1 == n) {
         pop_back();
         return;
      }
      swap(index, n - 1);
      pop_back();
   }

   /// @brief Pushes a value onto the back of the vector
   template<typename T>
   void push_back(const T &value) {
      verify_type<T>();
      append_element<T>(value);
   }

   /// @brief Pushes an rvalue reference onto the back of the vector
   template<typename T>
   void push_back(T &&value) { //NOLINT The non-forwarding is intentional.
      verify_type<T>();
      append_element<T>(value);
   }

   /// @brief Adds space for an uninitialized element at the back
   ///
   /// The added element is zero-initialized. Use at<T>() to assign a value.
   /// This method does not require knowing the concrete type at call site.
   void push_back_uninit() { mData.resize(mData.size() + mAlignedSz); }

   /// @brief Resizes the container to contain count elements
   template<typename T>
   void resize(size_t count, const T &value = T {}) {
      verify_type<T>();

      const size_t old_n = size();
      // Same size? Do nothing.
      if (count == old_n) { return; }

      // Shrink the vector. Trivial destructibility means we can just resize the vector.
      if (count < old_n) {
         mData.resize(count * mAlignedSz);
         return;
      }

      // Grow the Vector
      mData.resize(count * mAlignedSz);
      for (size_t i = old_n; i < count; ++i) { write_element_at<T>(element_ptr(i), value); }
   }

   /// @brief Returns a reference to the element at the specified index
   ///
   /// Throws std::runtime_error if the type doesn't match the stored type.
   /// Throws std::out_of_range if the index is out of bounds.
   template<typename T>
   T &at(size_t index) {
      verify_type<T>();
      if (index >= size()) { throw std::out_of_range("untyped_vector::at - index out of range"); }
      return *reinterpret_cast<T *>(element_ptr(index));
   }

   /// @brief Returns a const reference to the element at the specified index
   ///
   /// Throws std::runtime_error if the type doesn't match the stored type.
   /// Throws std::out_of_range if the index is out of bounds.
   template<typename T>
   const T &at(size_t index) const {
      verify_type<T>();
      if (index >= size()) { throw std::out_of_range("untyped_vector::at - index out of range"); }
      return *reinterpret_cast<const T *>(element_ptr(index));
   }

   /// @brief Returns a reference to the first element
   ///
   /// Throws std::runtime_error if the type doesn't match.
   template<typename T>
   T &front() {
      return at<T>(0);
   }

   /// @brief Returns a const reference to the first element
   ///
   /// Throws std::runtime_error if the type doesn't match.
   template<typename T>
   const T &front() const {
      return at<T>(0);
   }

   /// @brief Returns a reference to the last element
   ///
   /// Throws std::runtime_error if the type doesn't match.
   template<typename T>
   T &back() {
      return at<T>(size() - 1);
   }

   /// @brief Returns a const reference to the last element
   ///
   /// Throws std::runtime_error if the type doesn't match.
   template<typename T>
   const T &back() const {
      return at<T>(size() - 1);
   }

   /// @brief Returns the element size in bytes
   size_t element_size() const { return mTypeInfo.size; }

   /// @brief Returns the type hash of the stored elements
   uint64_t type_hash() const { return mTypeInfo.id; }

   /// @brief Returns a span of the elements for iteration
   ///
   /// Throws std::runtime_error if the type doesn't match the stored type.
   template<typename T>
   std::span<T> data() {
      verify_type<T>();
      return std::span<T>(reinterpret_cast<T *>(mData.data()), size());
   }

   /// @brief Returns a const span of the elements for iteration
   ///
   /// Throws std::runtime_error if the type doesn't match the stored type.
   template<typename T>
   std::span<const T> data() const {
      verify_type<T>();
      return std::span<const T>(reinterpret_cast<const T *>(mData.data()), size());
   }

  private:
   std::vector<std::byte> mData;
   ty_info mTypeInfo;
   size_t mAlignedSz;

   /// Access the ith element as a pointer to its raw bytes.
   std::byte *element_ptr(size_t i) { return mData.data() + (i * mAlignedSz); }
   const std::byte *element_ptr(size_t i) const { return mData.data() + (i * mAlignedSz); }

   /// @brief Helper to verify type matches the stored type
   /// Also performs compile-time checking to confirm that the type is trivially destructible.
   template<trivially_destructible T>
   void verify_type() const {
      if (mTypeInfo.id != getTypeID<T>()) {
         throw std::runtime_error("Type mismatch: expected '" + std::string(mTypeInfo.name) +
                                  "', got '" + std::string(get_unique_type_name<T>()) + "'");
      }
   }

   /// Copy value into one aligned slot at slot.
   template<trivially_destructible T>
   void write_element_at(std::byte *slot, const T &value) {
      const auto *src = reinterpret_cast<const std::byte *>(std::addressof(value));
      std::memcpy(slot, src, sizeof(T));
      const size_t padding = mAlignedSz - sizeof(T);
      if (padding > 0) { std::memset(slot + sizeof(T), 0, padding); }
   }

   template<trivially_destructible T>
   void append_element(const T &value) {
      mData.resize(mData.size() + mAlignedSz);
      write_element_at<T>(element_ptr(size() - 1), value);
   }
};

} // namespace tr
