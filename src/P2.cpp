///
/// @file  P2.cpp
/// @brief P2(x, a) is the 2nd partial sieve function.
///        P2(x, a) counts the numbers <= x that have exactly 2 prime
///        factors each exceeding the a-th prime. This implementation
///        uses the primesieve library for quickly iterating over
///        primes using next_prime() and prev_prime() which greatly
///        simplifies the implementation.
///
///        This implementation is based on the paper:
///        Tomás Oliveira e Silva, Computing pi(x): the combinatorial
///        method, Revista do DETUA, vol. 4, no. 6, March 2006,
///        pp. 759-768.
///
/// Copyright (C) 2021 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primecount-internal.hpp>
#include <primesieve.hpp>
#include <int128_t.hpp>
#include <min.hpp>
#include <imath.hpp>
#include <LoadBalancerP2.hpp>
#include <print.hpp>

#include <stdint.h>
#include <algorithm>
#include <cassert>

using namespace std;
using namespace primecount;

namespace {

/// Count primes inside [prime, stop]
int64_t count_primes(primesieve::iterator& it,
                     int64_t& prime,
                     int64_t stop)
{
  int64_t count = 0;
  int64_t p = prime;

  for (; p <= stop; count++)
    p = it.next_prime();

  prime = p;
  return count;
}

/// Thread sieves [low, high[
template <typename T>
T P2_thread(T x,
           int64_t y,
           int64_t low,
           int64_t high)
{
  assert(low > 0);
  assert(low < high);
  int64_t sqrtx = isqrt(x);
  int64_t start = max(y, min(x / high, sqrtx));
  int64_t stop = min(x / low, sqrtx);
  primesieve::iterator rit(stop + 1, start);
  int64_t prime = rit.prev_prime();

  if (prime <= start)
    return 0;

  // The first iteration requires computing pi(x / prime)
  // using the prime counting function.
  int threads = 1;
  int64_t xp = (int64_t)(x / prime);
  int64_t pi_xp = pi_noprint(xp, threads);
  T sum = pi_xp;

  // All other iterations compute pi(x / prime)
  // using a prime sieve.
  primesieve::iterator it(xp, high);
  int64_t next = it.next_prime();

  // \sum_{i = pi[start]+1}^{pi[stop]} pi(x / primes[i])
  for (prime = rit.prev_prime(); prime > start; prime = rit.prev_prime())
  {
    xp = (int64_t)(x / prime);
    pi_xp += count_primes(it, next, xp);
    sum += pi_xp;
  }

  return sum;
}

/// P2(x, y) counts the numbers <= x that have exactly 2
/// prime factors each exceeding the a-th prime.
/// Run-time: O(z log log z)
///
template <typename T>
T P2_OpenMP(T x,
            int64_t y,
            int threads,
            bool is_print)
{
  static_assert(is_signed<T>::value,
                "T must be signed integer type");

  if (x < 4)
    return 0;

  int64_t sqrtx = isqrt(x);
  T a = pi_noprint(y, threads);
  T b = pi_noprint(sqrtx, threads);

  if (a >= b)
    return 0;

  // \sum_{i=a+1}^{b} -(i - 1)
  T sum = (a - 2) * (a + 1) / 2 - (b - 2) * (b + 1) / 2;

  int64_t z = (int64_t)(x / max(y, 1));
  LoadBalancerP2 loadBalancer(x, z, threads, is_print);
  threads = loadBalancer.get_threads();

  // for (low = sqrt(x); low < z; low += dist)
  #pragma omp parallel num_threads(threads) reduction(+:sum)
  {
    int64_t low, high;
    while (loadBalancer.get_work(low, high))
      sum += P2_thread(x, y, low, high);
  }

  return sum;
}

} // namespace

namespace primecount {

int64_t P2(int64_t x,
           int64_t y,
           int threads,
           bool is_print)
{
  if (is_print)
  {
    print("");
    print("=== P2(x, y) ===");
    print_vars(x, y, threads);
  }

  double time = get_time();
  int64_t sum = P2_OpenMP(x, y, threads, is_print);

  if (is_print)
    print("P2", sum, time);

  return sum;
}

#ifdef HAVE_INT128_T

int128_t P2(int128_t x,
            int64_t y,
            int threads,
            bool is_print)
{
  if (is_print)
  {
    print("");
    print("=== P2(x, y) ===");
    print_vars(x, y, threads);
  }

  double time = get_time();
  int128_t sum = P2_OpenMP(x, y, threads, is_print);

  if (is_print)
    print("P2", sum, time);

  return sum;
}

#endif

} // namespace
