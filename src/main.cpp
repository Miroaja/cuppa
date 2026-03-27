#include "ps.h"
#include "sync.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <ncurses.h>
#include <omp.h>
#include <random>
#include <stdexcept>
#include <tuple>

const wchar_t *cuppajoe = LR"(     _.---------------._          )"
                          LR"( ,-''XXXXXXXXXXXXXXXXXXX''-.      )"
                          LR"(|;XXXXXXXXXXXXXXXXXXXXXXXXX;|     )"
                          LR"(|;--.XXXXXXXXXXXXXXXXXXX.--;|     )"
                          LR"(|;   `-----------------`_.-----   )"
                          LR"(|;                   -''__     \  )"
                          LR"(|;                        \     \ )"
                          LR"(|;                         \     |)"
                          LR"(|;                         ;|    |)"
                          LR"(|;                         ;|    |)"
                          LR"(|;                         ;|    |)"
                          LR"(|;                         ;|    |)"
                          LR"(|;                         ;|    ⌡)"
                          LR"(|;                         /    / )"
                          LR"(|;                    <   /  __/  )"
                          LR"(|;                     \_/--^     )"
                          LR"(|;                         ;|     )"
                          LR"( `||.                   .||`      )"
                          LR"(     `;;;;;;;;;;;;;;;;;`          )";

struct Steam {
  float x, y;
  float dx, dy;
  float ix, iy;
  int life;
  float heat;
  int id;
};

constexpr int WIDTH = 34;
constexpr int HEIGHT = 60;

static int partCount;
static bool enableHeatEq = false;

struct SteamData {
  std::array<float, WIDTH * HEIGHT> thermalLayer;
  std::array<float, WIDTH * HEIGHT> dThermalLayer;
  std::array<std::pair<float, float>, WIDTH * HEIGHT> gThermalLayer;
  float t;
};

void reset(Steam &s);

Steam initialize() {
  static int id = 0;
  Steam s;
  s.id = id++;

  static std::random_device rd{};
  static std::mt19937 gen(rd());
  static std::normal_distribution<float> d(0, 2);
  s.x = d(gen) * 1.5f + WIDTH / 2.0f - 2.5f;
  s.y = 31.5 + d(gen) / 2;
  s.ix = s.x;
  s.iy = s.y;

  reset(s);
  return s;
}

ps<Steam, SteamData> *stm = nullptr;

void update(Steam &s, SteamData &d, float dt) {
  float gx = 0.0f, gy = 0.0f;
  auto &p = d.thermalLayer[std::clamp<int>(s.x, 0, WIDTH - 1) +
                           WIDTH * std::clamp<int>(s.y, 0, HEIGHT - 1)];
  std::tie(gx, gy) = d.gThermalLayer[s.x + WIDTH * s.y];
  s.heat += p *= 0.5f;

  s.dx += -s.dx * 0.1f * dt;
  s.dy += -s.dy * 0.1f * dt;

  s.dx += -gx * 7.0f * dt;
  s.dy += -gy * 7.0f * dt;

  s.dy += (-s.heat * 0.7f + 0.1f) * dt;
  s.dx += cosf(s.heat * 100000.0f) * s.heat * 1.1f * dt;
  s.dx += 0.5f * dt * cosf(d.t * 6.28318f / 10.0) * cosf(s.heat * 500000.0f) *
          s.heat;

  s.y += s.dy * dt;
  s.x += s.dx * dt;

  s.heat -= 2 * dt;

  if (s.x < 0 || s.x >= WIDTH || s.y < 0 || s.y >= (int)(HEIGHT / 2) + 6) {
    s.life = 0;
  }
  if (s.dy >= 2.0f) {
    s.life = 0;
  }

  if (s.x <= 1 || s.x >= WIDTH - 2 || s.y <= 1 || s.y >= HEIGHT - 2) {
    return;
  }
  auto &p2 = d.thermalLayer[std::clamp<int>(s.x, 0, WIDTH - 1) +
                            WIDTH * std::clamp<int>(s.y, 0, HEIGHT - 1)];
  p2 += s.heat *= 0.5f;
}

void updateData(SteamData &d, float dt) {
  if (!enableHeatEq) {
    return;
  }
#pragma omp parallel for
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    auto x = i % WIDTH, y = i / WIDTH;

    if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
      d.thermalLayer[x + WIDTH * y] = 0;
      d.dThermalLayer[x + WIDTH * y] = 0;
      continue;
    }
    d.thermalLayer[x + WIDTH * y] += d.dThermalLayer[x + WIDTH * y] * dt;
  }

#pragma omp parallel for
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    auto x = i % WIDTH, y = i / WIDTH;
    if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
      continue;
    }

    int left = x - 1;   // Left neighbor
    int right = x + 1;  // Right neighbor
    int top = y - 1;    // Top neighbor
    int bottom = y + 1; // Bottom neighbor

    float currentTemp = d.thermalLayer[x + WIDTH * y];
    auto gx = (d.thermalLayer[right + WIDTH * y] - 2 * currentTemp +
               d.thermalLayer[left + WIDTH * y]) /
              2.0f;

    auto gy = (d.thermalLayer[x + WIDTH * bottom] - 2 * currentTemp +
               d.thermalLayer[x + WIDTH * top]) /
              2.0f;

    d.gThermalLayer[x + WIDTH * y] = {gx, gy};
  }

#pragma omp parallel for
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    auto [gx, gy] = d.gThermalLayer[i];

    float delta = gx + gy;
    d.dThermalLayer[i] = (3.5f * delta);
    d.thermalLayer[i] *= 0.99f;
  }

  d.t += dt;
}

wchar_t getChar(const Steam &s) {
  constexpr std::array<wchar_t, 14> nnn = {
      U' ', U'\'', U',', U'.', U':', U'⡈', U'⡪',
      U'⣒', U'⣕',  U'⣫', U'⣿', U'░', U'▒', U'▓',
  };
  return nnn[std::clamp<int>(std::tanh(s.heat / 4.5) * 12, 0, nnn.size() - 1)];
}

void reset(Steam &s) {
  s.x = s.ix;
  s.y = s.iy;
  s.dx = 0;
  s.dy = -10;
  s.life = 300;

  if (stm) {
    auto &p =
        stm->data.thermalLayer[std::clamp<int>(s.x, 0, WIDTH - 1) +
                               WIDTH * std::clamp<int>(s.y, 0, HEIGHT - 1)];
    s.heat = p;
    p = 4.0 * 512.0 / (float)partCount + 6.0;
    s.x += cosf(100000.0f * stm->data.t);
    s.y += cosf(5103000.0f * stm->data.t);
  }
}

static std::atomic_bool isDone = false;
void stdinWatcher() {
  [[maybe_unused]] std::string dummy;
  while (std::getline(std::cin, dummy)) {
  }
  isDone = true;
}

using namespace std::chrono_literals;
int main(int argc, const char **argv) {
  partCount = 512;
  if (argc >= 2) {
    std::string arg = argv[1];
    if (arg == "-t" && argc != 3) {
      enableHeatEq = true;
    } else {

      try {
        std::size_t pos;
        partCount = std::stoi(arg, &pos);
        if (pos < arg.size()) {
          std::cerr << "Trailing characters after number: " << arg << '\n';
          exit(1);
        }
        if (partCount <= 0) {
          throw std::invalid_argument("Negative or zero number");
        }
      } catch (std::invalid_argument const &ex) {
        std::cerr << "Invalid number: " << arg << '\n';
        exit(1);
      } catch (std::out_of_range const &ex) {
        std::cerr << "Number out of range: " << arg << '\n';
        exit(1);
      }
    }
  } else if (argc == 3) {
    std::string arg = argv[2];
    if (arg == "-t") {
      enableHeatEq = true;
    }
  } else if (argc >= 3) {
    std::cerr << "Too many arguments!";
    exit(1);
  }

  // omp_set_dynamic(1);
  omp_set_num_threads(std::max(1, omp_get_max_threads()));
  std::thread readerThread(stdinWatcher);

  setlocale(LC_ALL, "");
  initscr();
  curs_set(0);
  timeout(0);
  start_color();
  fb<34, 60> buf;
  ps<Steam, SteamData> steam(
      partCount, update, updateData, getChar, initialize,
      []() { return SteamData{.thermalLayer = {1.0f}, .dThermalLayer = {0}}; },
      reset);
  stm = &steam;
  float dt = 1.0f;

  std::array<float, WIDTH * HEIGHT> tField = {};
  while (!isDone) {
    if (getch() == KEY_RESIZE) {
      ::clear();
    }

    auto start = std::chrono::high_resolution_clock::now();

    steam.update(dt);
    std::sort(steam.particles().begin(), steam.particles().end(),
              [](const Steam &a, const Steam &b) { return a.heat < b.heat; });

    buf.clear(U' ');

    for (int x = WIDTH; x >= 0; x--) {
      for (int y = 0; y < 2; y++) {
        if (cuppajoe[x + WIDTH * y] == U'X') {
          buf.set(x, y + HEIGHT / 2, U'░');
          continue;
        }
        buf.set(x, y + HEIGHT / 2, cuppajoe[x + WIDTH * y]);
      }
    }
    tField.fill(0.0f);
    for (const auto &p : steam.particles()) {
      int x = p.x;
      int y = p.y;
      if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        continue;
      }
      tField[x + WIDTH * y] = std::max(tField[x + WIDTH * y], p.heat);
    }
    for (int x = 0; x < WIDTH; x++) {
      for (int y = 0; y < HEIGHT; y++) {
        auto c = getChar({.heat = tField[x + WIDTH * y]});
        if (c != U' ') {
          buf.set(x, y, c);
        }
      }
    }
    for (int x = WIDTH; x >= 0; x--) {
      for (int y = 2; y < 19; y++) {
        if (cuppajoe[x + WIDTH * y] == U'X') {
          if (getChar(
                  {.heat = tField[std::clamp<int>(x, 0, WIDTH) +
                                  WIDTH * std::clamp<int>((y + HEIGHT / 2), 0,
                                                          HEIGHT - 1)]}) ==
              U' ') {
            buf.set(x, y + HEIGHT / 2, U'░');
          }
          continue;
        }
        buf.set(x, y + HEIGHT / 2, cuppajoe[x + WIDTH * y]);
      }
    }

    buf.print();
    int x = getmaxx(stdscr) / 2 - 4;
    int yMax = getmaxy(stdscr);
    int y = (yMax - 60) / 2 + 43;

    attron(A_ITALIC);
    mvaddwstr(std::clamp(y, 0, yMax - 2), std::max(x, 0), L"Enjoy :)");
    attroff(A_ITALIC);
    refresh();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    syncTo(16ms, elapsed);
    dt = (std::chrono::duration_cast<std::chrono::duration<float>>(
              std::chrono::high_resolution_clock::now() - start))
             .count();
    dt = std::min<float>(dt, 1.0f / 60.0f);
  }
  endwin();

  readerThread.join();
}
