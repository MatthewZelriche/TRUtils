#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace tr {

template<typename T>
struct ty_info {
   static constexpr inline char typeID = 0;
};

// @brief A Type-Erased implementation of a contiguous growable array container.
class TypeErasedVector {
  public:
   // *** Static Constructor, Destructor, Copy & Move Constructors and Operators ***

   //! @brief Constructs a new TypeErasedVector capable of holding elements of type T.
   template<typename T>
   static TypeErasedVector create() {
      static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
      auto self = TypeErasedVector(alignof(T), sizeof(T), &ty_info<T>::typeID);
      return self;
   }
   ~TypeErasedVector() { operator delete(mDataBuf, typeAlignment()); }
   TypeErasedVector(const TypeErasedVector &other) :
       mTypeID(other.mTypeID), mElementAlignment(other.mElementAlignment),
       mElementSize(other.mElementSize), mElementCount(other.mElementCount),
       mElementCapacity(other.mElementCapacity) {
      mDataBuf = (uint8_t *)operator new(other.mElementCapacity * other.mElementSize,
                                         other.typeAlignment());
      std::memcpy(mDataBuf, other.mDataBuf, mElementSize * mElementCount);
   }
   TypeErasedVector &operator=(const TypeErasedVector &other) {
      if (this->mDataBuf == other.mDataBuf) { return *this; }
      // Check to make sure both contain the same type
      if (other.mTypeID != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      auto *newBuf = (uint8_t *)operator new(other.mElementCapacity * other.mElementSize,
                                             other.typeAlignment());
      std::memcpy(newBuf, other.mDataBuf, other.mElementSize * other.mElementCount);
      operator delete(mDataBuf, typeAlignment());
      // Don't need to update mTypeID, mElementcapacity, mElementSize, they are const assuming same typeID
      mDataBuf = newBuf;
      mElementCount = other.mElementCount;
      mElementCapacity = other.mElementCapacity;
      return *this;
   }
   TypeErasedVector(TypeErasedVector &&other) noexcept :
       mTypeID(other.mTypeID), mElementAlignment(other.mElementAlignment),
       mElementSize(other.mElementSize) {
      mDataBuf = other.mDataBuf;
      mElementCount = other.mElementCount;
      mElementCapacity = other.mElementCapacity;
      other.mDataBuf = nullptr;
      other.mElementCapacity = 0;
      other.mElementCount = 0;
   }
   TypeErasedVector &operator=(TypeErasedVector &&other) {
      // Check for self move. We don't have an assignment operator currently, but the allocated
      // memory addresses are enough to avoid memory errors due to a self-move.
      if (this->mDataBuf == other.mDataBuf) { return *this; }
      // Check to make sure both contain the same type
      if (other.mTypeID != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      operator delete(mDataBuf, typeAlignment());
      mDataBuf = other.mDataBuf;
      mElementCount = other.mElementCount;
      mElementCapacity = other.mElementCapacity;
      other.mDataBuf = nullptr;
      other.mElementCapacity = 0;
      other.mElementCount = 0;
      return *this;
   }

   // *** Type-Erased methods. These methods do not need to know the concrete type ***

   //! @brief Checks if the container is empty.
   bool empty() const { return mElementCount == 0; }
   //! @brief Returns the number of elements stored in the container
   size_t size() const { return mElementCount; }
   //! @brief Returns the number of elements stored in the container
   size_t capacity() const { return mElementCapacity; }
   //! @brief Returns the number of elements that the container has currently allocated space for.
   void clear() { mElementCount = 0; }

   /*! @brief Appends memory for T at the end of the container
   *
   * @warning If T is a non-trivial data type, it is the caller's responsibility to construct an 
   * instance of the type in the newly allocated position, eg with placement new
   */
   void *push_back_uninit() {
      if (shouldGrow()) { reserve(calculateCapacity()); }
      mElementCount++;
      return mem_at_unchecked(size() - 1);
   }

   /*! @brief Removes the last element in the container
   *
   * @throws if empty() is true. Even though we aren't returning a value, popping an empty container
   * is almost certainly not what the caller intended to do.
   */
   void pop_back() {
      if (empty()) { throw std::runtime_error("Attempted to pop an empty container"); }
      mElementCount -= 1;
   }

   /*! @brief Removes the element at pos by swapping it with the last element, and then popping it.
   *
   * @throws if empty() is true. Even though we aren't returning a value, popping an empty container
   * is almost certainly not what the caller intended to do.
   */
   void swap_and_pop(size_t pos) {
      if (empty()) { throw std::runtime_error("Attempted to pop an empty container"); }
      if (pos >= size()) { throw std::runtime_error("Array index out of bounds"); }
      const uint8_t *lastElement = mem_at_unchecked(size() - 1);
      uint8_t *posElement = mem_at_unchecked(pos);
      std::memcpy(posElement, lastElement, mElementSize);
      mElementCount -= 1;
   }

   /*! @brief Increases the capacity (number of elements that can be stored without re-allocation)
   *          of the container. 
   *
   * If newCap is less than or equal to the current capacity, this method does nothing. 
   */
   void reserve(size_t newCap) {
      if (newCap <= mElementCapacity) { return; }
      auto *newBuf = (uint8_t *)operator new(newCap * mElementSize, typeAlignment());
      if (mDataBuf != nullptr) { std::memcpy(newBuf, mDataBuf, mElementSize * mElementCount); }
      operator delete(mDataBuf, typeAlignment());
      mDataBuf = newBuf;
      mElementCapacity = newCap;
   }

   /*! @brief Resizes the container to contain exactly count elements. 
   *
   * If size() is already count, does nothing. Otherwise, shrinks or extends from the end of the 
   * container, as appropriate. 
   * 
   * @warning If T is a non-trivial data type, it is the caller's responsibility to construct an
   * instance of the type in all of the newly extended positions, eg with placement new
   */
   void resize(size_t count) {
      if (count == mElementCount) { return; }

      if (count < mElementCount) {
         mElementCount = count;
      } else {
         while (mElementCapacity < count) { reserve(calculateCapacity()); }
         mElementCount = count;
      }
   }

   // *** Methods that require the concrete type via a template parameter ***

   /*! @brief Retrieves the element of type T at pos in the container 
   *
   * @throws If pos is out of bounds
   * @throws If T is not the correct type of the stored type
   */
   template<typename T>
   T &at(size_t pos) {
      if (pos >= size()) { throw std::runtime_error("Array index out of bounds"); }
      if (&ty_info<T>::typeID != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      return *reinterpret_cast<T *>(mem_at_unchecked(pos));
   }

   template<typename T>
   const T &at(size_t pos) const {
      if (pos >= size()) { throw std::runtime_error("Array index out of bounds"); }
      if (&ty_info<T>::typeID != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      return *reinterpret_cast<const T *>(mem_at_unchecked(pos));
   }

   /*! @brief Retrieves a view of the contiguous stored values of type T
   *
   * @throws If T is not the correct type of the stored type
   */
   template<typename T>
   std::span<T> data() {
      if (&ty_info<T>::typeID != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      return std::span<T>(reinterpret_cast<T *>(mDataBuf), size());
   }

   template<typename T>
   std::span<const T> data() const {
      if (&ty_info<T>::id != mTypeID) { throw std::runtime_error("Type safety check failed"); }
      return std::span<const T>(reinterpret_cast<const T *>(mDataBuf), size());
   }

  private:
   using type_id = const void *;
   TypeErasedVector(size_t elementAlignment, size_t elementSize, type_id typeID) :
       mTypeID(typeID), mElementAlignment(elementAlignment), mElementSize(elementSize) {}

   size_t calculateCapacity() const { return mElementCapacity == 0 ? 1 : mElementCapacity * 2; }
   bool shouldGrow() const { return mElementCount >= mElementCapacity; }
   std::align_val_t typeAlignment() const {
      return static_cast<std::align_val_t>(mElementAlignment);
   }
   uint8_t *mem_at_unchecked(size_t pos) const { return mDataBuf + (mElementSize * pos); }

   uint8_t *mDataBuf {nullptr};
   const type_id mTypeID;
   const size_t mElementAlignment;
   const size_t mElementSize;
   size_t mElementCount {0};
   size_t mElementCapacity {0};
};

} // namespace tr