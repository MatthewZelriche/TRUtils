#pragma once

#include <iostream>
#include <source_location>
#include <type_traits>

namespace tr {

using ty_info = std::string_view;

template<typename T>
consteval ty_info getTypeID() {
   std::string_view result = std::source_location::current().function_name();
   size_t pos = 0;
   size_t end = result.size();
#ifdef _MSC_VER
   pos = result.find("id<") + 3;
   end = result.find(">", pos);
#elif defined(__GNUC__) && !defined(__clang__)
   pos = result.find("T = ") + 4;
   end = result.find(";", pos);
#elif __clang__
   pos = result.find("T = ") + 4;
   end = result.find("]", pos);
#else
   // Use full string size
#endif

   return result.substr(pos, end - pos);
}

} // namespace tr