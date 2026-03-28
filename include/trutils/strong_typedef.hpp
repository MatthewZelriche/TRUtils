#pragma once

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

  private:
   T _val {};
};

} // namespace tr