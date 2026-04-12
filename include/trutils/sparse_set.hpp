#pragma once

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "panic.hpp"

namespace tr {

template<class T, template<typename...> class Template>
concept is_specialization_of = requires(std::remove_cvref_t<T> type) {
   // Can T be implicitly converted to Template<Args...>?
   []<typename... Args>(Template<Args...> const &) { return true; }(type);
};

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

  private:
   using ID = uint32_t;
   using Version = uint32_t;

   ID id {INVALID_IDX};
   Version version {0};

   static constexpr Version DISABLED_VERSION = std::numeric_limits<Version>::max();
   static constexpr ID INVALID_IDX = std::numeric_limits<ID>::max();
   static constexpr ID FREELIST_END_IDX = std::numeric_limits<ID>::max() - 1;
   static constexpr ID SPARSE_MAX_IDX = std::numeric_limits<ID>::max() - 2;

   // Various friend declarations to avoid exposing internals of this class to users
   template<typename KeyType>
   friend class SparseSet;
   friend std::hash<Key<Tag>>;

   // Equality op for hashing
   friend bool operator==(const Key<Tag> &key, const Key<Tag> &other) {
      return key.id == other.id && key.version == other.version;
   }
};

template<typename KeyType = Key<DefaultTag>>
class SparseSet {
   static_assert(tr::is_specialization_of<KeyType, Key>,
                 "KeyType must be a specialization of class Key");

  public:
   /// @throws if the maximum size of the set has been reached.
   [[nodiscard]] KeyType insert() {
      auto denseIdx = mDense.size();
      auto sparseIdx = allocateSparseEntry(static_cast<KeyType::ID>(denseIdx));
      mDense.push_back(sparseIdx);

      KeyType key;
      key.id = sparseIdx;
      key.version = mSparse[sparseIdx].version;
      return key;
   }

   /// @throws if the set does not contain the given key.
   KeyType::ID get(KeyType key) const {
      if (!contains(key)) { THROW(std::out_of_range, "sparse_set::get - key not found"); }

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

   inline bool contains(const KeyType &key) const {
      return key.id < mSparse.size() && mSparse[key.id].version == key.version;
   }

   inline void clear() {
      mDense.clear();
      mSparse.clear();
      mFreelistHead = KeyType::FREELIST_END_IDX;
   }

   inline size_t size() const { return mDense.size(); }
   inline bool empty() const { return mDense.empty(); }

   /// @brief Valid key for the element at dense_idx in dense iteration order.
   /// @throws std::out_of_range if dense_idx is out of range.
   [[nodiscard]] KeyType key_at_dense(size_t dense_idx) const {
      if (dense_idx >= mDense.size()) {
         THROW(std::out_of_range, "sparse_set::key_at_dense - index out of range");
      }
      const auto sparse_idx = mDense[dense_idx];
      KeyType key {};
      key.id = sparse_idx;
      key.version = mSparse[sparse_idx].version;
      return key;
   }

  private:
   /// @throws if the sparse array has reached the maximum possible number of entries
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
         THROW(std::runtime_error, "sparse_set - allocation failed: maximum size reached");
      }
      mSparse.push_back(SparseEntry {denseIdx, 0});
      return idx;
   }

   void freeSparseEntry(KeyType::ID sparseIdx) {
      mSparse[sparseIdx].denseIdx = KeyType::INVALID_IDX;
      mSparse[sparseIdx].version += 1;

      // Do not recycle if we max out this slot's version
      if (mSparse[sparseIdx].version != KeyType::DISABLED_VERSION) { freelistPush(sparseIdx); }
   }

   [[nodiscard]] KeyType::ID freelistPop() {
      auto oldHead = mFreelistHead;
      if (oldHead == KeyType::FREELIST_END_IDX) { return KeyType::INVALID_IDX; }
      mFreelistHead = freelistNext(oldHead);
      return oldHead;
   }

   KeyType::ID freelistNext(KeyType::ID freelistIdx) {
      auto result = KeyType::FREELIST_END_IDX;
      if (freelistIdx != KeyType::FREELIST_END_IDX) { result = mSparse[freelistIdx].denseIdx; }

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
      // Note/Todo: Will not work if we widen the types
      return std::hash<uint64_t> {}((uint64_t)key.id | ((uint64_t)key.version) << 32);
   }
};
// NOLINTEND

} // namespace std