#pragma once

#include <functional>
#include <utility>

namespace tr {

template<typename Tag, typename T>
class strong_typedef {
  public:
   using type = T;
   explicit strong_typedef() = default;
   explicit strong_typedef(const T &val) : _val(val) {}
   explicit strong_typedef(T &&val) : _val(std::move(val)) {}

   explicit operator T &() { return _val; }
   explicit operator const T &() const { return _val; }

   friend bool operator==(const strong_typedef &lhs, const strong_typedef &rhs) {
      return lhs._val == rhs._val;
   }

   friend auto operator<=>(const strong_typedef &lhs, const strong_typedef &rhs) {
      return lhs._val <=> rhs._val;
   }

  private:
   T _val {};
};

} // namespace tr

// NOLINTBEGIN
namespace std {
template<typename Tag, typename T>
struct hash<tr::strong_typedef<Tag, T>> {
   std::size_t operator()(const tr::strong_typedef<Tag, T> &x) const {
      return std::hash<T> {}(static_cast<const T &>(x));
   }
};
} // namespace std
// NOLINTEND
