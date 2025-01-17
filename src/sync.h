#pragma once
#include <chrono>
#include <thread>

template <typename Rep, typename Period>
void syncTo(std::chrono::duration<Rep, Period> to,
            std::chrono::duration<Rep, Period> period) {
  using namespace std::chrono_literals;
  auto sleep = to - period;
  if (sleep < 0ms) {
    return;
  }
  std::this_thread::sleep_for(sleep);
}
