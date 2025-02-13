#pragma once
#include "fb.h"
#include <concepts>
#include <functional>
#include <stdint.h>
#include <vector>

template <typename T>
concept Particle = requires(T a) {
  { a.x } -> std::convertible_to<int>;
  { a.y } -> std::convertible_to<int>;
  { a.life } -> std::convertible_to<int>;
};

template <Particle P> struct ps {
  ps(int count, auto updater, auto charGetter, auto initializer, auto resetter)
      : _updateParticle(updater), _getChar(charGetter),
        _initializeParticle(initializer), _resetParticle(resetter) {
    for (int i = 0; i < count; i++) {
      _particles.push_back(_initializeParticle());
    }
  }

  std::vector<P> &particles() { return _particles; }

  void update(float dt) {
    for (auto &p : _particles) {
      if (p.life-- <= 0) {
        _resetParticle(p);
      }
      _updateParticle(p, dt);
    }
  }

  template <int W, int H> void print(fb<W, H> &f) {
    for (auto p : _particles) {
      f.set(p.x, p.y, _getChar(p));
    }
  }

private:
  std::function<void(P &, float)> _updateParticle;
  std::function<wchar_t(const P &)> _getChar;
  std::function<P(void)> _initializeParticle;
  std::function<void(P &)> _resetParticle;
  std::vector<P> _particles;
};
