#pragma once

#include <cassert>
#include <deque>

#include "slot_map.hpp"
#include "sparse_map.hpp"
#include "type_erased_vector.hpp"

namespace tr {

template<typename ColumnKeyTag = DefaultTag>
class Table {
  private:
   struct RowKeyTag {};

  public:
   using RowKey = Key<RowKeyTag>;
   using ColumnKey = Key<ColumnKeyTag>;

   template<typename T>
   std::pair<RowKey, TypeErasedVector &> createRow() {
      auto row = TypeErasedVector::create<T>();
      row.resize(mColumnMapping.size());
      auto key = mRows.insert(TypeErasedVector::create<T>());
      auto ptr = mRows.get(key);
      // Quick sanity check to ensure the returned value isn't null, though this shouldn't be possible here
      assert(ptr != nullptr);
      return {key, ptr};
   }

   bool removeRow(RowKey key) { return mRows.remove(key); }

   template<typename T>
   std::span<T> getRow(RowKey key) {
      return mRows.get(key)->data();
   }

   ColumnKey createColumn() {
      auto key = mColumnMapping.insert();
      for (auto &row : mRows) { row.push_back(); }
      return key;
   }

   bool removeColumn(ColumnKey key) {
      if (!mColumnMapping.contains(key)) { return false; }
      auto idx = mColumnMapping.get(key);
      mColumnMapping.erase(key);
      for (auto &row : mRows) { row.pop_and_swap(idx); }
      return true;
   }

   template<typename T>
   T &rowEntryAtColumn(ColumnKey colKey, RowKey rowKey) {
      auto idx = mColumnMapping.get(colKey);
      auto rowPtr = mRows.get(rowKey);
      if (idx == ColumnKey::INVALID_IDX || rowPtr == nullptr) { throw std::runtime_error(""); }
      return rowPtr->at<T>(idx);
   }

  private:
   SparseMap<ColumnKey> mColumnMapping;
   SlotMap<RowKey, TypeErasedVector, std::deque> mRows {};
};

} // namespace tr