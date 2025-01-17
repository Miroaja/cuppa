#pragma once
#include "utf8.h"
#include "utf8/checked.h"
#include <array>
#include <cassert>
#include <codecvt>
#include <cstdint>
#include <cstring>
#include <cuchar>
#include <iostream>
#include <limits.h>
#include <locale>
#include <string>

template <int W, int H> struct fb {
  inline fb() : _buffer() {}
  inline void print() noexcept {
    std::cout << "\033[H\033[2J\033[3J\033[?7l";
    std::u32string holder = U"";
    int c = 0;
    for (auto ch : _buffer) {
      holder += ch;
      holder += c++ % W == W - 1 ? U"\n" : U"";
    }
    std::cout << utf8::utf32to8(holder) << "\033[?7h";
  }
  inline void set(int x, int y, char32_t c) {
    if (x < 0 || x >= W || y < 0 || y >= H) {
      return;
    }
    _buffer[x + W * y] = c;
  }
  inline void clear(char32_t c = U' ') {
    for (auto &ch : _buffer) {
      ch = c;
    }
  }

private:
  std::array<char32_t, W * H> _buffer;
};
