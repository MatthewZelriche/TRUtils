#pragma once

#include "simple_flatmap.hpp"
#include "sparse_set.hpp"
#include "type_id.hpp"
#include "untyped_vector.hpp"

namespace tr {

class table {
   struct column_tag {};
   using column_key = Key<column_tag>;
   using column_mapping = SparseSet<column_key>;

  public:
   template<typename T>
   void createRow() {
      if (containsRow<T>()) { throw std::runtime_error(""); }
   }

   template<typename T>
   bool containsRow() const {}

   template<typename T>
   void erase_row() {}

   template<typename T>
   T *get_column_entry() {}

  private:
   simple_flatmap<ty_id, untyped_vector> mRows;
   column_mapping mColumnMapping;
};

} // namespace tr