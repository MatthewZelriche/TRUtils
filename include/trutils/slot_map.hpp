#pragma once

#include "sparse_map.hpp"

namespace tr {

template<typename Key, typename Value, template<typename Value> class Storage = std::vector>
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

   Value *get(Key key) {
      if (!mMapping.contains(key)) { return nullptr; }
      return &mStorage.at(mMapping.get(key));
   }

   const Value *get(Key key) const {
      if (!mMapping.contains(key)) { return nullptr; }
      return &mStorage.at(mMapping.get(key));
   }

   Value remove(Key key) {
      if (!mMapping.contains(key)) { throw std::runtime_error(""); }

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

  private:
   SparseMap<Key> mMapping;
   Storage<Value> mStorage {};
};

} // namespace tr