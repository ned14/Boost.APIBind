/* test_spinlock_tribool.cpp
Unit testing for spinlocks
(C) 2013-2016 Niall Douglas http://www.nedproductions.biz/


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#define NANOSECONDS_PER_CPU_CYCLE (1000000000000ULL / 3700000000ULL)

#define _CRT_SECURE_NO_WARNINGS 1

#include "../include/boost/test/unit_test.hpp"
#include "../include/spinlock.hpp"
#include "../include/tribool.hpp"
#include "timing.h"

#include <algorithm>
#include <stdio.h>

BOOST_AUTO_TEST_SUITE(all)

BOOST_AUTO_TEST_CASE(works / spinlock, "Tests that the spinlock works as intended")
{
  boost_lite::configurable_spinlock::spinlock<bool> lock;
  BOOST_REQUIRE(lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock();

  std::lock_guard<decltype(lock)> h(lock);
  BOOST_REQUIRE(!lock.try_lock());
}

BOOST_AUTO_TEST_CASE(works / spinlock / threaded, "Tests that the spinlock works as intended under threads")
{
  boost_lite::configurable_spinlock::spinlock<bool> lock;
  boost_lite::configurable_spinlock::atomic<size_t> gate(0);
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
  for(size_t i = 0; i < 1000; i++)
  {
    gate.store(threads);
    size_t locked = 0;
#pragma omp parallel for reduction(+ : locked)
    for(int n = 0; n < (int) threads; n++)
    {
      --gate;
      while(gate)
        ;
      locked += lock.try_lock();
    }
    BOOST_REQUIRE(locked == 1U);
    lock.unlock();
  }
}

#if 0
BOOST_AUTO_TEST_CASE(works / spinlock / transacted, "Tests that the spinlock works as intended under transactions")
{
  boost_lite::configurable_spinlock::spinlock<bool> lock;
  boost_lite::configurable_spinlock::atomic<size_t> gate(0);
  size_t locked = 0;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
#pragma omp parallel for
  for(int i = 0; i < (int) (1000 * threads); i++)
  {
    BOOST_BEGIN_TRANSACT_LOCK(lock) { ++locked; }
    BOOST_END_TRANSACT_LOCK(lock)
  }
  BOOST_REQUIRE(locked == 1000 * threads);
}
#endif

#if 0
template <bool tristate, class T> struct do_lock
{
  void operator()(T &lock) { lock.lock(); }
};
template <class T> struct do_lock<true, T>
{
  void operator()(T &lock)
  {
    int e = 0;
    lock.lock(e);
  }
};

template <class locktype> double CalculatePerformance(bool use_transact)
{
  locktype lock;
  boost_lite::configurable_spinlock::atomic<size_t> gate(0);
  struct
  {
    size_t value;
    char padding[64 - sizeof(size_t)];
  } count[64];
  memset(&count, 0, sizeof(count));
  usCount start, end;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
  // printf("There are %u threads in this CPU\n", (unsigned) threads);
  start = GetUsCount();
#pragma omp parallel for
  for(int thread = 0; thread < threads; thread++)
  {
    --gate;
    while(gate)
      ;
    for(size_t n = 0; n < 10000000; n++)
    {
      if(use_transact)
      {
        BOOST_BEGIN_TRANSACT_LOCK(lock) { ++count[thread].value; }
        BOOST_END_TRANSACT_LOCK(lock)
      }
      else
      {
        do_lock<std::is_same<typename locktype::value_type, int>::value, locktype>()(lock);
        ++count[thread].value;
        lock.unlock();
      }
    }
  }
  end = GetUsCount();
  size_t increments = 0;
  for(size_t thread = 0; thread < threads; thread++)
  {
    BOOST_REQUIRE(count[thread].value == 10000000);
    increments += count[thread].value;
  }
  return increments / ((end - start) / 1000000000000.0);
}

BOOST_AUTO_TEST_CASE(performance / spinlock / binary, "Tests the performance of binary spinlocks")
{
  printf("\n=== Binary spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<bool> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / binary / transaction, "Tests the performance of binary spinlock transactions")
{
  printf("\n=== Transacted binary spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<bool> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / tristate, "Tests the performance of tristate spinlocks")
{
  printf("\n=== Tristate spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<int> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / tristate / transaction, "Tests the performance of tristate spinlock transactions")
{
  printf("\n=== Transacted tristate spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<int> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / pointer, "Tests the performance of pointer spinlocks")
{
  printf("\n=== Pointer spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<boost_lite::configurable_spinlock::lockable_ptr<int>> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(false));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / pointer / transaction, "Tests the performance of pointer spinlock transactions")
{
  printf("\n=== Transacted pointer spinlock performance ===\n");
  typedef boost_lite::configurable_spinlock::spinlock<boost_lite::configurable_spinlock::lockable_ptr<int>> locktype;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(true));
}

static double CalculateMallocPerformance(size_t size, bool use_transact)
{
  boost_lite::configurable_spinlock::spinlock<bool> lock;
  boost_lite::configurable_spinlock::atomic<size_t> gate(0);
  usCount start, end;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
  // printf("There are %u threads in this CPU\n", (unsigned) threads);
  start = GetUsCount();
#pragma omp parallel for
  for(int n = 0; n < 10000000 * threads; n++)
  {
    void *p;
    if(use_transact)
    {
      BOOST_BEGIN_TRANSACT_LOCK(lock) { p = malloc(size); }
      BOOST_END_TRANSACT_LOCK(lock)
    }
    else
    {
      std::lock_guard<decltype(lock)> g(lock);
      p = malloc(size);
    }
    if(use_transact)
    {
      BOOST_BEGIN_TRANSACT_LOCK(lock) { free(p); }
      BOOST_END_TRANSACT_LOCK(lock)
    }
    else
    {
      std::lock_guard<decltype(lock)> g(lock);
      free(p);
    }
  }
  end = GetUsCount();
  BOOST_REQUIRE(true);
  //  printf("size=%u\n", (unsigned) map.size());
  return threads * 10000000 / ((end - start) / 1000000000000.0);
}

BOOST_AUTO_TEST_CASE(performance / malloc / transact / small, "Tests the transact performance of multiple threads using small memory allocations")
{
  printf("\n=== Small malloc transact performance ===\n");
  printf("1. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
  printf("2. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
  printf("3. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
}

BOOST_AUTO_TEST_CASE(performance / malloc / transact / large, "Tests the transact performance of multiple threads using large memory allocations")
{
  printf("\n=== Large malloc transact performance ===\n");
  printf("1. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
  printf("2. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
  printf("3. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
}
#endif


BOOST_AUTO_TEST_CASE(works / tribool, "Tests that the tribool works as intended")
{
  using tribool::tribool;
  auto t(tribool::true_), f(tribool::false_), o(tribool::other), u(tribool::unknown);
  BOOST_CHECK(true_(t));
  BOOST_CHECK(false_(f));
  BOOST_CHECK(other(o));
  BOOST_CHECK(t != f);
  BOOST_CHECK(t != o);
  BOOST_CHECK(f != o);
  BOOST_CHECK(o == u);
  BOOST_CHECK(~t == f);
  BOOST_CHECK(~f == t);
  BOOST_CHECK(~u == u);
  BOOST_CHECK((f & f) == f);
  BOOST_CHECK((f & t) == f);
  BOOST_CHECK((f & u) == f);
  BOOST_CHECK((u & u) == u);
  BOOST_CHECK((u & t) == u);
  BOOST_CHECK((t & t) == t);
  BOOST_CHECK((f | f) == f);
  BOOST_CHECK((f | u) == u);
  BOOST_CHECK((f | t) == t);
  BOOST_CHECK((u | u) == u);
  BOOST_CHECK((u | t) == t);
  BOOST_CHECK((t | t) == t);
  BOOST_CHECK(std::min(f, u) == f);
  BOOST_CHECK(std::min(f, t) == f);
  BOOST_CHECK(std::min(u, t) == u);
  BOOST_CHECK(std::min(t, t) == t);
  BOOST_CHECK(std::max(f, u) == u);
  BOOST_CHECK(std::max(f, t) == t);
  std::cout << "bool false is " << false << ", bool true is " << true << std::endl;
  std::cout << "tribool false is " << f << ", tribool unknown is " << u << ", tribool true is " << t << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()