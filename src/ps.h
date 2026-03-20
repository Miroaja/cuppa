#pragma once
#include "fb.h"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <execution>
#include <functional>
#include <ranges>
#include <stdint.h>
#include <vector>

template <typename T>
concept Particle = requires(T a) {
  { a.x } -> std::convertible_to<int>;
  { a.y } -> std::convertible_to<int>;
  { a.life } -> std::convertible_to<int>;
};

template <Particle P, typename Data> struct ps {
  inline ps(int count, auto updater, auto dUpdater, auto charGetter,
            auto initializer, auto dataInitializer, auto resetter)
      : _updateParticle(updater), _updateData(dUpdater), _getChar(charGetter),
        _initializeParticle(initializer), _initializeData(dataInitializer),
        _resetParticle(resetter) {
    data = _initializeData();
    for (int i = 0; i < count; i++) {
      _particles.push_back(_initializeParticle());
    }
  }

  std::vector<P> &particles() { return _particles; }

  Data data;

  inline void update(float dt) {
    _updateData(data, dt);

    for (size_t i = 0; i < _particles.size(); i++) {
      if (_particles[i].life-- <= 0) {
        _resetParticle(_particles[i]);
      }
      _updateParticle(_particles[i], data, dt);
    }
  }

  template <int W, int H> inline void print(fb<W, H> &f) {
    for (auto p : _particles) {
      f.set(p.x, p.y, _getChar(p));
    }
  }

private:
  std::function<void(P &, Data &, float)> _updateParticle;
  std::function<void(Data &, float)> _updateData;
  std::function<wchar_t(const P &)> _getChar;
  std::function<P(void)> _initializeParticle;
  std::function<Data(void)> _initializeData;
  std::function<void(P &)> _resetParticle;
  std::vector<P> _particles;
};
