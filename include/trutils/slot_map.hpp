#pragma once

#include <cassert>
#include <span>
#include <stdexcept>
#include <utility>

#include "sparse_set.hpp"

namespace tr {

template<typename Key, typename Value, template<typename SVal> class Storage = std::vector>
class SlotMap {
  public:
   [[nodiscard]] Key insert(Value &&value) {
      auto key = mMapping.insert();
      mStorage.push_back(std::move(value));
      return key;
   }

   [[nodiscard]] Key insert(const Value &value) {
      auto key = mMapping.insert();
      mStorage.push_back(value);
      return key;
   }

   bool contains(Key key) const { return mMapping.contains(key); }

   /// @throws If the slotmap does not contain an entry associated with key.
   Value &get(Key key) { return mStorage.at(mMapping.get(key)); }

   /// @throws If the slotmap does not contain an entry associated with key.
   const Value &get(Key key) const { return mStorage.at(mMapping.get(key)); }

   /// @throws If the slotmap does not contain an entry associated with key.
   Value remove(Key key) {
      if (!mMapping.contains(key)) { throw std::runtime_error("Key not found in slotmap"); }

      auto storageIdx = mMapping.get(key);
      mMapping.erase(key);

      // Swap and pop the data
      auto oldDataSize = mStorage.size();
      if (oldDataSize > 1) { std::swap(mStorage[storageIdx], mStorage[oldDataSize - 1]); }
      auto val = std::move(mStorage[oldDataSize - 1]);
      mStorage.pop_back();
      return val;
   }

   size_t size() const { return mStorage.size(); }

   void clear() {
      mStorage.clear();
      mMapping.clear();
   }

   Storage<Value>::iterator begin() { return mStorage.begin(); }
   Storage<Value>::iterator end() { return mStorage.end(); }
   Storage<Value>::const_iterator begin() const { return mStorage.begin(); }
   Storage<Value>::const_iterator end() const { return mStorage.end(); }

   std::span<Value> data() { return std::span(mStorage); }
   std::span<const Value> data() const { return std::span(mStorage); }

  private:
   SparseSet<Key> mMapping;
   Storage<Value> mStorage {};
};

/// Non-owning view over a SlotMap's storage.
/// Lookup and mutation of
/// existing entries are allowed; nothing here inserts, erases, or reallocates. The view must not
/// outlive the data it aliases.
template<typename KeyType, typename Value>
class slot_map_view {
  public:
   using key_type = KeyType;
   using value_type = Value;

   slot_map_view() = delete;
   slot_map_view(const SparseSet<key_type> &keys, std::span<Value> values) :
       mKeys(keys), mValues(values) {}

   bool empty() const { return mValues.empty(); }

   size_t size() const { return mValues.size(); }

   bool contains(key_type key) const { return mKeys.contains(key); }

   /// @throws std::out_of_range if @p key is not in the set or the dense index is out of range.
   Value &at(key_type key) {
      const size_t idx = index_of(key);
      return mValues[idx];
   }

   /// @throws std::out_of_range if @p key is not in the set or the dense index is out of range.
   const Value &at(key_type key) const {
      const size_t idx = index_of(key);
      return mValues[idx];
   }

   std::span<Value> values() { return mValues; }
   std::span<const Value> values() const { return mValues; }

  private:
   size_t index_of(key_type key) const {
      const auto idx = static_cast<size_t>(mKeys.get(key));
      assert(idx < mValues.size());
      return idx;
   }

   const SparseSet<key_type> &mKeys {};
   std::span<Value> mValues {};
};

} // namespace tr