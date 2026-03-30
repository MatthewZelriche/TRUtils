#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>

#include "simple_flatmap.hpp"
#include "slot_map.hpp"
#include "type_id.hpp"
#include "untyped_vector.hpp"

namespace tr {

class table {
  public:
   struct column_tag {};
   using column_key = Key<column_tag>;
   using column_mapping = SparseSet<column_key>;

   template<typename T>
   void createRow() {
      if (containsRow<T>()) { throw std::runtime_error("Duplicate row type in table"); }
      auto typeInfo = getTypeInfo<T>();
      auto vector = untyped_vector(typeInfo);
      vector.resize<T>(mColumnMapping.size());
      mRows.insert(typeInfo.id, std::move(vector));
   }

   template<typename T>
   bool containsRow() const {
      return mRows.contains(getTypeID<T>());
   }

   template<typename T>
   bool erase_row() {
      return mRows.erase(getTypeID<T>());
   }

   /// Dense, unordered row storage - row_view is needed for key lookups.
   template<typename T>
   std::span<T> row() {
      return mRows.at(getTypeID<T>()).template data<T>();
   }

   template<typename T>
   std::span<const T> row() const {
      return mRows.at(getTypeID<T>()).template data<T>();
   }

   /// Provides a typed view over the row's storage, allowing key lookups.
   /// View is invalidated if if it outlives the table, or if rows or columns are erased.
   // Therefore you should typically not keep this view around for long.
   template<typename T>
   slot_map_view<column_key, T> row_view() {
      return {mColumnMapping, mRows.at(getTypeID<T>()).template data<T>()};
   }

   template<typename T>
   slot_map_view<column_key, const T> row_view() const {
      return {mColumnMapping, mRows.at(getTypeID<T>()).template data<T>()};
   }

   /// @brief Adds a column; extends every existing row by one default-initialized cell.
   /// @warning Newly installed column entries for this new column are zero-initialized.
   [[nodiscard]] column_key insert_column() {
      column_key key = mColumnMapping.insert();
      for (auto &entry : mRows) { entry.second.push_back_uninit(); }
      return key;
   }

   /// @brief Mutable access to the cell at key in row T.
   /// @throws std::out_of_range if key is not a live column or row T is missing.
   template<typename T>
   T &cell(column_key key) {
      const auto colIdx = static_cast<size_t>(mColumnMapping.get(key));
      return mRows.at(getTypeID<T>()).template at<T>(colIdx);
   }

   template<typename T>
   const T &cell(column_key key) const {
      const auto colIdx = static_cast<size_t>(mColumnMapping.get(key));
      return mRows.at(getTypeID<T>()).template at<T>(colIdx);
   }

   /// @brief For column key, returns a tuple of references to the requested row cells, in order.
   /// @throws std::out_of_range if the column key is invalid or a requested row type is not in the table.
   template<typename... RowTs>
   std::tuple<RowTs &...> query_column(column_key key) {
      const auto colIdx = static_cast<size_t>(mColumnMapping.get(key));
      return std::tuple<RowTs &...> {mRows.at(getTypeID<RowTs>()).template at<RowTs>(colIdx)...};
   }

   template<typename... RowTs>
   std::tuple<const RowTs &...> query_column(column_key key) const {
      const auto colIdx = static_cast<size_t>(mColumnMapping.get(key));
      return std::tuple<const RowTs &...> {
          mRows.at(getTypeID<RowTs>()).template at<RowTs>(colIdx)...};
   }

  private:
   simple_flatmap<ty_id, untyped_vector> mRows;
   column_mapping mColumnMapping;
};

} // namespace tr