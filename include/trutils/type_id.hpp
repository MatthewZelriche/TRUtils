#pragma once

#include <cstdint>
#include <iostream>
#include <source_location>
#include <string_view>

namespace tr {

using ty_id = uint64_t;

struct ty_info {
   std::string_view name;
   size_t size;
   size_t alignment;
   ty_id id;
};

template<typename T>
consteval std::string_view get_unique_type_name() {
   std::string_view result = std::source_location::current().function_name();
   size_t pos = 0;
   size_t end = result.size();
#ifdef _MSC_VER
   pos = result.find("name<") + 5;
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

consteval ty_id hash_string(std::string_view str) {
   // FNV-1a hash (64-bit)
   uint64_t hash = 0xcbf29ce484222325ULL; // FNV offset basis
   for (char c : str) {
      hash ^= static_cast<uint64_t>(c);
      hash *= 0x100000001b3ULL; // FNV prime
   }
   return hash;
}

template<typename T>
consteval ty_id getTypeID() {
   return hash_string(get_unique_type_name<T>());
}

template<typename T>
consteval ty_info getTypeInfo() {
   ty_info info;
   info.name = get_unique_type_name<T>();
   info.size = sizeof(T);
   info.alignment = alignof(T);
   info.id = hash_string(info.name);
   return info;
}

} // namespace tr