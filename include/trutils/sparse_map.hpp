#pragma once

#include <optional>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "utility_concepts.hpp"

namespace tr {

template<class T>
concept TagType = std::is_empty_v<T>;

//! @brief The default tag for a Slotmap key, if the user does not provide a custom Key.
class DefaultTag {};

/*! @class Key
 *  @brief The key type used for uniquely identifying values in a slotmap.
 *
 * Key is a strongly-typed wrapper around an underlying, implementation-defined integral type
 * which stores the actual identifying information. This strong typing allows users to define 
 * custom key typing by tagging the Key struct with a custom TagType for compile-time type checking
 * safety. The underlying implementation of Key stores both an identifier, and a version, to
 * prevent use-after-free errors. The class is intentionally opaque, as the internals are entirely
 * implementation-defined and subject to change without warning.
 * 
 * @tparam Tag A tag type that provides strong typing for user-defined keys. For example,
 *         Key<struct Foo> defines a new, strongly typed Key that is different from Key<struct Bar>
*/
template<TagType Tag>
class Key {
  public:
   using ID = uint32_t;
   ID getID() const { return id; }
   bool valid() const { return id != INVALID_IDX; }
   bool operator!() const { return valid(); }

  private:
   using ID = uint32_t;
   using Version = uint32_t;

   ID id {INVALID_IDX};
   Version version {0};

   static constexpr ID DISABLED_VERSION = std::numeric_limits<ID>::max();
   static constexpr ID INVALID_IDX = std::numeric_limits<ID>::max();
   static constexpr ID FREELIST_END_IDX = std::numeric_limits<ID>::max() - 1;
   static constexpr ID SPARSE_MAX_IDX = std::numeric_limits<ID>::max() - 2;

   // Various friend declarations to avoid exposing internals of this class to users
   template<typename KeyType>
   friend class SparseMap;
   friend std::hash<Key<Tag>>;

   // Equality op for hashing
   friend bool operator==(const Key<Tag> &key, const Key<Tag> &other) {
      return key.id == other.id && key.version == other.version;
   }
};

template<typename KeyType = Key<DefaultTag>>
class SparseMap {
   static_assert(tr::is_specialization_of<KeyType, Key>,
                 "KeyType must be a specialization of class Key");

  public:
   [[nodiscard]] KeyType insert() {
      auto denseIdx = mDense.size();
      auto sparseIdx = allocateSparseEntry(static_cast<KeyType::ID>(denseIdx));
      if (sparseIdx == KeyType::INVALID_IDX) [[unlikely]] {
         throw std::runtime_error("");
      }
      mDense.push_back(sparseIdx);

      KeyType key;
      key.id = sparseIdx;
      key.version = mSparse[sparseIdx].version;
      return key;
   }

   KeyType::ID get(KeyType key) {
      if (!contains(key)) {
         return KeyType::INVALID_IDX;
      }

      return mSparse[key.id].denseIdx;
   }

   bool erase(const KeyType &key) {
      if (!contains(key)) { return false; }
      auto sparseIdxToUpdate = mDense.back();

      // Swap and pop from dense array
      auto denseIdx = mSparse[key.id].denseIdx;
      if (denseIdx != mDense.size() - 1) {
         // If we aren't the last/only element, swap and pop
         auto oldDenseSize = mDense.size();
         std::swap(mDense[denseIdx], mDense[oldDenseSize - 1]);
         mDense.pop_back();

         // Update bookkeeping, but only if we actually swapped an element
         mSparse[sparseIdxToUpdate].denseIdx = denseIdx;
      } else {
         mDense.pop_back();
      }

      freeSparseEntry(key.id);
      return true;
   }

   bool contains(const KeyType &key) const {
      return key.id < mSparse.size() && mSparse[key.id].version == key.version;
   }

   void clear() {
      mDense.clear();
      mSparse.clear();
      mFreelistHead = KeyType::FREELIST_END_IDX;
   }

   size_t size() const { return mDense.size(); }

  private:
   [[nodiscard]] KeyType::ID allocateSparseEntry(KeyType::ID denseIdx) {
      auto idx = freelistPop();
      if (idx != KeyType::INVALID_IDX) {
         // Reusing an entry from the freelist: update its dense index
         // to reflect where the new element will live in the dense array.
         mSparse[idx].denseIdx = denseIdx;
         return idx;
      }

      // Nothing in freelist, grow sparse array
      idx = static_cast<KeyType::ID>(mSparse.size());
      if (idx == KeyType::SPARSE_MAX_IDX) [[unlikely]] {
         return KeyType::INVALID_IDX;
      }
      mSparse.push_back(SparseEntry {denseIdx, 0});
      return idx;
   }

   void freeSparseEntry(KeyType::ID sparseIdx) {
      mSparse[sparseIdx].denseIdx = KeyType::INVALID_IDX;
      mSparse[sparseIdx].version += 1;

      // Do not recycle if we max out this slot's version
      if (mSparse[sparseIdx].version != KeyType::DISABLED_VERSION) {
         freelistPush(sparseIdx);
      }
   }

   [[nodiscard]] KeyType::ID freelistPop() {
      auto oldHead = mFreelistHead;
      if (oldHead == KeyType::FREELIST_END_IDX) {
         return KeyType::INVALID_IDX;
      }
      mFreelistHead = freelistNext(oldHead);
      return oldHead;
   }

   KeyType::ID freelistNext(KeyType::ID freelistIdx) {
      auto result = KeyType::FREELIST_END_IDX;
      if (freelistIdx != KeyType::FREELIST_END_IDX) {
         result = mSparse[freelistIdx].denseIdx;
      }

      return result;
   }

   void freelistPush(KeyType::ID sparseIdx) {
      auto oldHead = mFreelistHead;
      mFreelistHead = sparseIdx;
      mSparse[sparseIdx].denseIdx = oldHead;
   }

   KeyType::ID mFreelistHead {KeyType::FREELIST_END_IDX};
   std::vector<typename KeyType::ID> mDense {};

   struct SparseEntry {
      union {
         KeyType::ID denseIdx;
         KeyType::ID freelistNext;
      };
      KeyType::Version version;
   };

   std::vector<SparseEntry> mSparse {};
};

} // namespace tr

// NOLINTBEGIN
namespace std {
template<typename T>
struct hash<tr::Key<T>> {
   std::size_t operator()(const tr::Key<T> &key) const noexcept {
      return std::hash<uint64_t> {}((uint64_t)key.id | ((uint64_t)key.version) << 32);
   }
};
// NOLINTEND

} // namespace std