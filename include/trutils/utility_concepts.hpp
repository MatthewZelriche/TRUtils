#pragma once

#include <type_traits>

namespace tr {

template<class T, template<typename...> class Template>
concept is_specialization_of = requires(std::remove_cvref_t<T> type) {
   // Can T be implicitly converted to Template<Args...>?
   []<typename... Args>(Template<Args...> const &) { return true; }(type);
};

} // namespace tr