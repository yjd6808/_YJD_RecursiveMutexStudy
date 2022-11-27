/*
 * 작성자: 윤정도
 * =====================
 *
 */

#pragma warning(push)
  #pragma warning(disable: 26495) // variable is uninitialized. Always initialize a member variable
  #include <benchmark/benchmark.h>
#pragma warning(pop)

#include <functional>
#include <iostream>

#include "my_hybrid_rmtx.h"
#include "my_cond_rmtx.h"
#include "my_ultimate_rmtx.h"

#define MY_HYBRID_RMTX        0
#define MY_COND_RMTX          1
#define MY_ULTIMATE_RMTX      2
#define STD_RMTX              3

struct rtmx_wrapper_interface
{
  virtual void lock() = 0;
  virtual void unlock() = 0;
};

// 표준 락과 내가 구현한 락의 공통 인터페이스
template <typename t_rmtx>
struct rtmx_wrapper : rtmx_wrapper_interface
{
  rtmx_wrapper(t_rmtx& r) { rmtx_ptr = std::addressof(r); }
  void lock() override { rmtx_ptr->lock(); }
  void unlock() override { rmtx_ptr->unlock(); }
private:
  t_rmtx* rmtx_ptr;
};

std::recursive_mutex g_std_rmtx;
my_hybrid_rmtx g_my_hybrid_rmtx;
my_cond_rmtx g_my_cond_rmtx;
my_ultimate_rmtx g_my_ultimate_rmtx;

rtmx_wrapper g_wrapper_my_hybrid_rmtx(g_my_hybrid_rmtx);
rtmx_wrapper g_wrapper_my_cond_rmtx(g_my_cond_rmtx);
rtmx_wrapper g_wrapper_my_ultimate_rmtx(g_my_ultimate_rmtx);
rtmx_wrapper g_wrapper_std_rmtx(g_std_rmtx);

volatile int shared_value = 0;


static void increaser(rtmx_wrapper_interface& rmtx_wrapper, int64_t loop, int64_t recursive) {
  for (int64_t i = 0; i < loop; ++i) {
    for (int j = 0; j < recursive; j++)
      rmtx_wrapper.lock();
    ++shared_value;
    for (int j = 0; j < recursive; j++)
      rmtx_wrapper.unlock();
  }
}

static void decreaser(rtmx_wrapper_interface& rmtx_wrapper, int64_t loop, int64_t recursive) {
  for (int64_t i = 0; i < loop; ++i) {
    for (int j = 0; j < recursive; j++)
      rmtx_wrapper.lock();
    --shared_value;
    for (int j = 0; j < recursive; j++)
      rmtx_wrapper.unlock();
  }
}


// ==============================================================================
// 벤치마크 관련 로직
// ==============================================================================


static void BM_rmtx_teardown(const benchmark::State& state) {
  if (shared_value != 0) {
    std::cout << "테스트 실패\n";
  }

  shared_value = 0;
}

static void BM_rmtx(benchmark::State& state)
{
  const int64_t rmtx_type = state.range(0);
  const int64_t loop_count = state.range(1);
  const int64_t recursive_count = state.range(2);
  const std::function method = state.thread_index() < 4 ? increaser : decreaser;

  rtmx_wrapper_interface* r = nullptr;

  switch (rmtx_type)
  {
  case MY_HYBRID_RMTX:    r = dynamic_cast<rtmx_wrapper_interface*>(&g_wrapper_my_hybrid_rmtx);   break;
  case MY_COND_RMTX:      r = dynamic_cast<rtmx_wrapper_interface*>(&g_wrapper_my_cond_rmtx);     break;
  case MY_ULTIMATE_RMTX:  r = dynamic_cast<rtmx_wrapper_interface*>(&g_wrapper_my_ultimate_rmtx); break;
  case STD_RMTX:          r = dynamic_cast<rtmx_wrapper_interface*>(&g_wrapper_std_rmtx);         break;
  }

  for (auto _ : state)
    method(*r, loop_count, recursive_count);
}



BENCHMARK(BM_rmtx)
->Teardown(BM_rmtx_teardown)
->ThreadRange(8, 8)
->Unit(benchmark::kMillisecond)
->ArgsProduct(
{
  { MY_HYBRID_RMTX, MY_COND_RMTX, MY_ULTIMATE_RMTX, STD_RMTX },
  { 1'000'000 },     // 반복 횟수
  { 1, 2, 10 }       // 잠금 횟수
});
