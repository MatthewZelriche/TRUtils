#pragma once

#include <cstdint>
#include <source_location>
#include <string_view>

namespace tr {

using ty_id = uint64_t;

struct ty_info {
   std::string_view name;
   size_t size {};
   size_t alignment {};
   ty_id id {};
};

template<typename T>
consteval std::string_view get_unique_type_name() {
   constexpr std::string_view result = std::source_location::current().function_name();
#ifdef _MSC_VER
   constexpr std::string_view nameStr = "name<";
   constexpr size_t pos = result.find(nameStr) + nameStr.size();
   constexpr size_t end = result.find('>', pos);
#elif defined(__GNUC__) && !defined(__clang__)
   constexpr size_t pos = result.find("T = ") + 4;
   constexpr size_t end = result.find(";", pos);
#elif defined(__clang__)
   constexpr size_t pos = result.find("T = ") + 4;
   constexpr size_t end = result.find("]", pos);
#else
   // Use full string size
#endif

   if constexpr (pos == std::string_view::npos || end == std::string_view::npos) { return result; }
   return result.substr(pos, end - pos);
}

consteval ty_id hash_string(std::string_view str) {
   // FNV-1a hash (64-bit)
   constexpr uint64_t fnvOffsetBasis = 0xcbf29ce484222325ULL;
   constexpr uint64_t fnvPrime = 0x100000001b3ULL;
   uint64_t hash = fnvOffsetBasis; // FNV offset basis
   for (char c : str) {
      hash ^= static_cast<uint64_t>(c);
      hash *= fnvPrime; // FNV prime
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