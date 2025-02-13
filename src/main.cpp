#include "ps.h"
#include "sync.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ncurses.h>
#include <random>

const char32_t *cuppajoe = UR"(  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░       
░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░      
░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░      
░░  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒  ░░░    
░░      ░░░░░░░░░░░░      ░  ░   
░░                        ░  ░   
  ░░                    ░░   ░   
  ░░                    ░░   ░   
   ░                  ░░░░░░░    
   ░                ░░░░░        
    ░░           ░░░░░░          
      ░░░░░░░░░░░░░░░░           )";

struct Steam {
  float x, y;
  float dx, dy;
  int life;
  float heat;
  int id;
};

void reset(Steam &s);

Steam initialize() {
  static int id = 0;
  Steam s;
  s.id = id++;
  reset(s);
  return s;
}

ps<Steam> *stm = nullptr;
void update(Steam &s, float dt) {
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  std::normal_distribution<float> d(0, s.heat);
  s.dy += (-s.heat * 0.7f + 0.1f) * dt;
  s.dx += d(gen) * 1.1f * dt;

  s.dx += -s.dx * 0.01f;
  s.dy += -s.dy * 0.01f;

  s.y += s.dy * dt;
  s.x += s.dx * dt;

  if (stm) {
    for (Steam &ss : stm->particles()) {
      if (ss.id != s.id) {
        if (fmax(fabs(ss.x - s.x), fabs(ss.y - s.y)) < 1.5f) {
          if (fabs(s.heat - ss.heat) < 0.2f) {
            float xLoss = (s.dx + ss.dx) / 2.0f * dt;
            float yLoss = (s.dy + ss.dy) / 2.0f * dt;
            ss.dx += xLoss;
            s.dx -= xLoss;
            ss.dy += yLoss;
            s.dy -= yLoss;
          }
        }
      }
    }
  }
  s.heat -= 2 * dt;

  if (s.x < 0 || s.x >= 34 || s.y < 0 || s.y >= 19) {
    s.life = 0;
  }
}

wchar_t getChar(const Steam &s) {
  constexpr std::array<wchar_t, 13> nnn = {
      U'.', U'.', U'.', U'-', U'⡈', U'⡪', U'⣒',
      U'⣕', U'⣫', U'⣿', U'░', U'▒', U'▓',
  };
  return nnn[std::clamp<int>(s.heat, 0, nnn.size() - 1)];
}

void reset(Steam &s) {
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  static std::normal_distribution<float> d(0, 3);
  s.x = d(gen) + 13;
  s.dx = 0;
  s.dy = 0;
  s.y = 18 + d(gen) / 5;
  s.life = 200 + d(gen) * 15;
  s.heat = std::fabs(d(gen)) * 2;
}

using namespace std::chrono_literals;
int main(void) {
  setlocale(LC_ALL, "en_US.UTF-8");
  fb<34, 30> buf;
  ps<Steam> steam(512, update, getChar, initialize, reset);
  stm = &steam;
  float dt = 1.0f;
  float t = 0.0f;
  for (;;) {
    auto start = std::chrono::high_resolution_clock::now();

    steam.update(dt);
    std::sort(steam.particles().begin(), steam.particles().end(),
              [](const Steam &a, const Steam &b) { return a.heat < b.heat; });

    buf.clear(U' ');

    for (int x = 32; x >= 0; x--) {
      for (int y = 0; y < 12; y++) {
        buf.set(x, y + 16, cuppajoe[x + 34 * y]);
      }
    }

    steam.print(buf);

    buf.print();

    //
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    syncTo(33ms, elapsed);
    printf("\n\ntime: %f", t);
    dt = (std::chrono::duration_cast<std::chrono::duration<float>>(
              std::chrono::high_resolution_clock::now() - start))
             .count();
    t += dt;
  }
}
