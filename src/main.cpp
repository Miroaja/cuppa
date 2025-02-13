#include "ps.h"
#include "sync.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ncurses.h>
#include <random>

const wchar_t *cuppajoe = LR"(  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░       
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
  [[maybe_unused]] static float t = 0.0f;
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  std::normal_distribution<float> d(0, s.heat);

  s.dy += (-s.heat * 0.7f + 0.1f) * dt;
  s.dx += d(gen) * 1.1f * dt;

  s.dx += -s.dx * 0.01f;
  s.dy += -s.dy * 0.01f;

  s.dx += 1.0f * dt * cosf(t * 6.28318f / 10.0) * d(gen);

  s.y += s.dy * dt;
  s.x += s.dx * dt;

  if (stm) {
    for (Steam &ss : stm->particles()) {
      if (ss.id != s.id) {
        if (fmax(fabs(ss.x - s.x), fabs(ss.y - s.y)) < 3.0f) {

          float xLoss = (s.dx + ss.dx) / 2.0f * dt * 0.3f;
          float yLoss = (s.dy + ss.dy) / 2.0f * dt * 0.3f;
          float hLoss = (s.heat + ss.heat) / 2.0f * dt * 0.3f;
          ss.dx += xLoss;
          s.dx -= xLoss;
          ss.dy += yLoss;
          s.dy -= yLoss;
          s.heat -= hLoss;
          ss.heat += hLoss;
        }
      }
    }
  }
  s.heat -= 2 * dt;

  if (s.x < 0 || s.x >= 34 || s.y < 0 || s.y >= 33) {
    s.life = 0;
  }
  if (s.dy >= 5.0f && s.heat <= 1.0f) {
    s.life = 0;
  }
  t += dt;
}

wchar_t getChar(const Steam &s) {
  constexpr std::array<wchar_t, 13> nnn = {
      U'\'', U',', U'.', U':', U'⡈', U'⡪', U'⣒',
      U'⣕',  U'⣫', U'⣿', U'░', U'▒', U'▓',
  };
  return nnn[std::clamp<int>(std::tanh(s.heat / 4.5) * 12, 0, nnn.size() - 1)];
}

void reset(Steam &s) {
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  static std::normal_distribution<float> d(0, 3);
  s.x = d(gen) * 1.5f + 13;
  s.dx = 0;
  s.dy = 0;
  s.y = 32 + d(gen) / 5;
  s.life = 300 + d(gen) * 15;
  s.heat = 9.0f;
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
  int partCount = 512;
  if (argc == 2) {
    std::string arg = argv[1];
    try {
      std::size_t pos;
      partCount = std::stoi(arg, &pos);
      if (pos < arg.size()) {
        std::cerr << "Trailing characters after number: " << arg << '\n';
      }
    } catch (std::invalid_argument const &ex) {
      std::cerr << "Invalid number: " << arg << '\n';
      exit(1);
    } catch (std::out_of_range const &ex) {
      std::cerr << "Number out of range: " << arg << '\n';
      exit(1);
    }
  } else if (argc > 2) {
    std::cerr << "Too many arguments!";
    exit(1);
  }

  std::thread readerThread(stdinWatcher);

  setlocale(LC_ALL, "");
  initscr();
  curs_set(0);
  start_color();
  fb<34, 60> buf;
  ps<Steam> steam(partCount, update, getChar, initialize, reset);
  stm = &steam;
  float dt = 1.0f;

  while (!isDone) {
    auto start = std::chrono::high_resolution_clock::now();

    steam.update(dt);
    std::sort(steam.particles().begin(), steam.particles().end(),
              [](const Steam &a, const Steam &b) { return a.heat < b.heat; });

    buf.clear(U' ');

    for (int x = 32; x >= 0; x--) {
      for (int y = 0; y < 12; y++) {
        buf.set(x, y + 30, cuppajoe[x + 34 * y]);
      }
    }

    steam.print(buf);
    buf.print();
    int x = (getmaxx(stdscr) - 34) / 2;
    int yMax = getmaxy(stdscr);
    int y = (yMax - 60) / 2 + 43;

    attron(A_ITALIC);
    mvaddwstr(std::clamp(y, 0, yMax - 2), std::max(x, 0),
              L"    Enjoy a good cuppa :)");
    attroff(A_ITALIC);
    refresh();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    syncTo(33ms, elapsed);
    dt = (std::chrono::duration_cast<std::chrono::duration<float>>(
              std::chrono::high_resolution_clock::now() - start))
             .count();
  }
  endwin();

  readerThread.join();
}
