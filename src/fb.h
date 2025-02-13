#pragma once
#include <array>
#include <cassert>
#include <codecvt>
#include <cstdint>
#include <cstring>
#include <cuchar>
#include <functional>
#include <iostream>
#include <limits.h>
#include <locale>
#include <ncurses.h>
#include <ostream>
#include <string>
#include <string_view>

template <int W, int H> struct fb {
  inline fb() : _buffer() {}
  inline void print() noexcept {
    int x, y;
    getmaxyx(stdscr, y, x);
    x = (x - W) / 2;
    y = (y - H) / 2;

    for (int row = 0; row < H; ++row) {
      std::wstring wstr(_buffer.begin() + row * W,
                        _buffer.begin() + (row + 1) * W);
      mvaddwstr(std::max(y + row, 0), std::max(x, 0), wstr.c_str());
    }
  }

  inline void set(int x, int y, wchar_t c) {
    if (x < 0 || x >= W || y < 0 || y >= H) {
      return;
    }
    _buffer[x + W * y] = c;
  }

  inline void clear(wchar_t c = U' ') {
    _buffer.fill(c);
    ::clear();
    refresh();
  }

private:
  std::array<wchar_t, W * H> _buffer;
};
