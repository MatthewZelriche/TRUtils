#pragma once

#include <cassert>
#include <deque>
#include <span>
#include <stdexcept>

#include "slot_map.hpp"
#include "sparse_map.hpp"
#include "type_erased_vector.hpp"

namespace tr {

template<TagType KeyTag, typename Value>
class TypedRowView {
  public:
   using KeyType = Key<KeyTag>;
   TypedRowView() = default;
   TypedRowView(const SparseMap<KeyType> *mapping, TypeErasedVector *storage) :
       mMapping(mapping), mStorage(storage) {}

   const Value &at(KeyType key) const {
      if (!mMapping->contains(key)) { throw std::runtime_error(""); }
      return mStorage->at_unchecked<Value>(mMapping->get(key));
   }

   Value &at(KeyType key) {
      if (!mMapping->contains(key)) { throw std::runtime_error(""); }
      return mStorage->at_unchecked<Value>(mMapping->get(key));
   }

   std::span<const Value> data() const { return mStorage->data_unchecked<Value>(); }

   std::span<Value> data() { return mStorage->data_unchecked<Value>(); }

  private:
   const SparseMap<KeyType> *mMapping {nullptr};
   TypeErasedVector *mStorage {nullptr};
};

template<TagType ColumnKeyTag = DefaultTag>
class Table {
  private:
   struct RowKeyTag {};

  public:
   using RowKey = Key<RowKeyTag>;
   using ColumnKey = Key<ColumnKeyTag>;

   template<typename T>
   RowKey createRow() {
      auto row = TypeErasedVector::create<T>();
      row.resize(mColumnMapping.size());
      auto key = mRows.insert(TypeErasedVector::create<T>());
      auto ptr = mRows.get(key);
      // Quick sanity check to ensure the returned value isn't null, though this shouldn't be possible here
      assert(ptr != nullptr);
      return key;
   }

   bool removeRow(RowKey key) { return mRows.remove(key); }

   template<typename T>
   auto getRowView(RowKey key) {
      auto *row = mRows.get(key);
      if (!row->holds_type<T>()) { throw std::runtime_error(" "); }
      return TypedRowView<ColumnKeyTag, T>(&mColumnMapping, row);
   }

   template<typename T>
   std::span<T> getRowData(RowKey key) {
      return mRows.get(key)->data();
   }

   ColumnKey createColumn() {
      auto key = mColumnMapping.insert();
      for (auto &row : mRows) { row.push_back_uninit(); }
      return key;
   }

   bool removeColumn(ColumnKey key) {
      if (!mColumnMapping.contains(key)) { return false; }
      auto idx = mColumnMapping.get(key);
      mColumnMapping.erase(key);
      for (auto &row : mRows) { row.pop_and_swap(idx); }
      return true;
   }

   using KeyIterator = typename SparseMap<ColumnKey>::template SparseMapKeyIterator<ColumnKey>;
   KeyIterator keys() const { return KeyIterator(mColumnMapping); }

  private:
   SparseMap<ColumnKey> mColumnMapping;
   SlotMap<RowKey, TypeErasedVector, std::deque> mRows {};
};

} // namespace tr