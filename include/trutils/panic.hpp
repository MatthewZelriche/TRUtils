#pragma once

#include <format>
#include <iostream>

#ifdef __cpp_exceptions
#define THROW(except_ty, ...) throw except_ty(std::format(__VA_ARGS__));
#else
#define THROW(except_ty, ...) tr::panic(__VA_ARGS__);
#endif

namespace tr {

template<typename... Args>
void panic(Args &&...args) {
   std::cerr << "PANIC: " << std::format(std::forward<Args>(args)...) << std::endl;
   std::abort();
}

} // namespace tr