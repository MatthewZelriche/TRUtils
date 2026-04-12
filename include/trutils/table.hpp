#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "simple_flatmap.hpp"
#include "sparse_set.hpp"
#include "type_id.hpp"
#include "untyped_vector.hpp"

namespace tr {

class table {
  public:
   struct column_tag {};
   using column_key = Key<column_tag>;
   using column_mapping = SparseSet<column_key>;

   /// Non-owning view of one row: @ref column_key maps to cells in that row's dense buffer.
   /// From a non-const @ref table use @c row_view<T>; from a const table use
   /// @c row_view<const T>.
   /// Invalid if this row type is erased or the table is destroyed; column insert/erase and other
   /// rows do not invalidate the view.
   template<typename Cell>
   class row_view {
     public:
      using key_type = column_key;
      using value_type = Cell;
      using T = std::remove_const_t<Cell>;

      row_view() = delete;
      ~row_view() = default;
      row_view(const row_view &) = default;
      row_view(row_view &&) noexcept = default;
      row_view &operator=(const row_view &) = default;
      row_view &operator=(row_view &&) noexcept = default;

      row_view(const column_mapping &columns, untyped_vector &row) : mColumns(columns), mRow(row) {}

      bool empty() const { return mRow.size() == 0; }

      size_t size() const { return mRow.size(); }

      bool contains(column_key key) const { return mColumns.contains(key); }

      /// @throws std::out_of_range if @p key is not a live column or index is out of range.
      T &at(column_key key) {
         const size_t idx = index_of(key);
         return mRow.data<T>()[idx];
      }

      /// @throws std::out_of_range if @p key is not a live column or index is out of range.
      const T &at(column_key key) const {
         const size_t idx = index_of(key);
         return mRow.data<T>()[idx];
      }

      std::span<T> values() { return mRow.data<T>(); }
      std::span<const T> values() const { return mRow.data<T>(); }

     private:
      size_t index_of(column_key key) const { return static_cast<size_t>(mColumns.get(key)); }

      const column_mapping &mColumns;
      untyped_vector &mRow;
   };

   template<typename T>
   row_view<T> create_row() {
      if (contains_row<T>()) {
         THROW(std::invalid_argument, "table::create_row - duplicate row type");
      }
      auto typeInfo = getTypeInfo<T>();
      auto vector = untyped_vector(typeInfo);
      vector.resize<T>(mColumnMapping.size());
      mRows.insert(typeInfo.id, std::move(vector));
      return get_row_view<T>();
   }

   template<typename T>
   bool contains_row() const {
      return mRows.contains(getTypeID<T>());
   }

   template<typename T>
   bool erase_row() {
      return mRows.erase(getTypeID<T>());
   }

   /// Dense, unordered row storage — use row_view for key lookups.
   template<typename T>
   std::span<T> get_row() {
      return mRows.at(getTypeID<T>()).template data<T>();
   }

   template<typename T>
   std::span<const T> get_row() const {
      return mRows.at(getTypeID<T>()).template data<T>();
   }

   template<typename T>
   row_view<T> get_row_view() {
      return row_view<T>(mColumnMapping, mRows.at(getTypeID<T>()));
   }

   /// @brief Adds a column; extends every existing row by one default-initialized cell.
   /// @warning Newly installed column entries for this new column are zero-initialized.
   [[nodiscard]] column_key insert_column() {
      column_key key = mColumnMapping.insert();
      for (auto &entry : mRows) { entry.second.push_back_uninit(); }
      return key;
   }

   bool erase_column(column_key key) {
      if (!mColumnMapping.contains(key)) { return false; }

      const auto colIdx = static_cast<size_t>(mColumnMapping.get(key));
      for (auto &entry : mRows) { entry.second.swap_and_pop(colIdx); }
      mColumnMapping.erase(key);
      return true;
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
