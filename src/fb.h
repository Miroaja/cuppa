#pragma once
#include <array>
#include <cassert>
#include <codecvt>
#include <cstdint>
#include <cstring>
#include <cuchar>
#include <iostream>
#include <limits.h>
#include <locale>
#include <ncurses.h>
#include <ostream>
#include <string>

template <int W, int H> struct fb {
  inline fb() : _buffer() {}
  inline void print() noexcept {
    std::cout << "\033[H\033[2J\033[3J\033[?7l";

    wrefresh(stdscr);
    int x, y;
    getmaxyx(stdscr, y, x);
    x /= 2;
    y /= 2;
    x -= W / 2;
    y -= H / 2;

    for (int i = 0; i < y; i++) {
      std::cout << "\n";
    }
    std::u32string holder = U"";
    std::u32string pad(std::max(x, 0), U' ');

    int c = 0;
    for (auto ch : _buffer) {
      holder += c % W == 0 ? pad : U"";
      holder += ch;
      holder += c++ % W == W - 1 ? U"\n" : U"";
    }

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::cout << converter.to_bytes(holder) << "\033[?7h";
    std::cout << std::flush;
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
