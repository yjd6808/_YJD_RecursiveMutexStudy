/*
 * 작성자: 윤정도
 * 생성일: 11/26/2022 3:45:19 PM
 * =====================
 *
 */


#pragma once

#include <mutex>
#include <thread>
#include <cassert>

class my_hybrid_rmtx
{
public:
  my_hybrid_rmtx()
  {
    locked_thread_id = 0;
    recursion = 0;
  }

  void lock() 
  {   
    // 잠금이 해제될때까지 spin
    while (!try_lock());
  }

  bool try_lock() {
    const size_t cur_id = get_current_thread_id();
    size_t expected = 0;

    if (locked_thread_id.compare_exchange_strong(expected, cur_id))
    {
      mtx.lock();
      recursion = 1;
      return true;
    }

    if (expected == cur_id)
    {
      ++recursion;
      return true;
    }

    return false;
  }

  void unlock() {
    const size_t cur_id = get_current_thread_id();
    assert(locked_thread_id == cur_id);
    assert(recursion > 0);

    if ((--recursion) == 0)
    {
      locked_thread_id = 0;
      mtx.unlock();
    }
  }
private:
  static size_t get_current_thread_id() {
    // https://en.cppreference.com/w/cpp/thread/thread/id/hash
    // thread::id를 정수형값으로 얻을 수 있도록 해줌 
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
  }

private:
  std::atomic<size_t> locked_thread_id;
  std::mutex mtx;
  int recursion;
};



