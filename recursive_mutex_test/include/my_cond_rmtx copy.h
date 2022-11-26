/*
 * 작성자: 윤정도
 * 생성일: 11/26/2022 5:35:48 PM
 * =====================
 *
 */


#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <cassert>

class my_cond_rmtx
{
public:
  my_cond_rmtx()
  {
    locked_thread_id = 0;
    recursion = 0;
  }

  void lock()
  {
    // 잠금이 해제될때까지 wait
    const size_t cur_id = get_current_thread_id();

    std::unique_lock lg(this_mtx);
    if (cur_id == locked_thread_id)
    {
      ++recursion;
      return;
    }

    for (;;)
    {
      if (locked_thread_id == 0)
      {
        mtx.lock();
        recursion = 1;
        locked_thread_id = cur_id;
        break;
      }

      condvar.wait(lg);
    }
  }

  bool try_lock() {
    const size_t cur_id = get_current_thread_id();

    std::unique_lock lg(this_mtx);
    if (mtx.try_lock())
    {
      recursion = 1;
      locked_thread_id = cur_id;
      return true;
    }

    if (cur_id == locked_thread_id)
    {
      ++recursion;
      return true;
    }

    return false;
  }

  void unlock() {
    const size_t cur_id = get_current_thread_id();
    std::unique_lock lg(this_mtx);
    assert(cur_id == locked_thread_id);

    if ((--recursion) == 0)
    {
      mtx.unlock();
      locked_thread_id = 0;
      condvar.notify_one();
    }
  }
private:
  static size_t get_current_thread_id() {
    // https://en.cppreference.com/w/cpp/thread/thread/id/hash
    // thread::id를 정수형값으로 얻을 수 있도록 해줌 
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
  }

private:
  std::condition_variable condvar;
  std::mutex this_mtx;
  std::mutex mtx;

  size_t locked_thread_id;
  int recursion;
};



