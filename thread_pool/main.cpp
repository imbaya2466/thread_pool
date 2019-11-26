
#include "thread_pool.h"
#include <iostream>
#include <vector>
#include <chrono>


int f(int i) {
  return i * i;
}

int main()
{
  ThreadPool pool(4,8,4,1000);
  std::vector< std::future<int> > results;

  for (int i = 0; i < 8; ++i) {
    results.emplace_back(
      pool.enqueue(f, i)
    );
  }

  for (auto&& result : results)
    std::cout << result.get() << ' ';   //std::future get未准备好时会阻塞
  std::cout << std::endl;

  return 0;
}



