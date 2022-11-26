/*
 * 작성자: 윤정도
 * 생성일: 11/26/2022 9:38:06 PM
 * =====================
 *
 */


#pragma once

#include <atomic>
#include <mutex>
#include <cassert>

// [구현 사항]
// 1. 같은 쓰레드에서 락을 중첩으로 획득하는 것을 허용한다.
// 2. 중첩시 몇번 중첩되었는지 카운트를 하도록한다.
// 3. 잠금된 상태에서 다른 쓰레드가 잠금을 시도할려고 할 때는 잠금이 풀릴 때까지 기다려야한다.
// 이게 진짜 제대로 구현한 버전
class my_ultimate_rmtx
{
public:
  my_ultimate_rmtx()
  {
    locked_thread_id = 0;
    recursion = 0;
  }

  void lock()
  {
    const size_t cur_id = get_current_thread_id();

    if (locked_thread_id != cur_id)
    {
      mtx.lock();

      locked_thread_id = cur_id;
      recursion = 1;
      return;
    }

    ++recursion;
  }

  bool try_lock()
  {
    const size_t cur_id = get_current_thread_id();

    if (mtx.try_lock())
    {
      locked_thread_id = cur_id;
      recursion = 1;
      return true;
    }

    if (locked_thread_id == cur_id)
    {
      ++recursion;
      return true;
    }

    return false;
  }

  void unlock()
  {
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
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
  }
private:

  std::mutex mtx;
  size_t locked_thread_id;
  int recursion;
};