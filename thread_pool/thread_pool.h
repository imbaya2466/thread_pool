#pragma once

#ifdef __cplusplus

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
  ThreadPool(uint32_t core_threads, uint32_t max_threads, uint32_t max_queue, uint32_t time_alive_nowork_ms);
  template<class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
    ->std::future<typename std::result_of<F(Args...)>::type>;
  ~ThreadPool();
private:
  bool SetFit();
  bool AddThread(uint32_t add_size);
  // need to keep track of threads so we can join them
  std::vector< std::thread > workers_;
  // the task queue
  std::queue< std::function<void()> > tasks_;

  // synchronization
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  bool stop_;

  //manage value
  uint32_t core_threads_ = 4;
  uint32_t max_threads_ = 8;
  uint32_t max_queue_ = 4;
  uint32_t time_alive_nowork_ms_ = 0;

};


// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
  if (!SetFit()) {
    throw std::runtime_error("threads and queue max");
  }
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared< std::packaged_task<return_type()> >(
    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // don't allow enqueueing after stopping the pool
    if (stop_)
      throw std::runtime_error("enqueue on stopped ThreadPool");

    tasks_.emplace([task]() { (*task)(); });
  }
  condition_.notify_one();
  return res;
}
#else
typedef void *thread_pool;
thread_pool thpool_init(uint32_t core_threads, uint32_t max_threads, uint32_t max_queue,
                          uint32_t time_alive_nowork_ms);
bool thpool_add_work(thread_pool t_pool, void (*function_p)(void *), void *arg_p);
void thpool_destroy(thread_pool t_pool);

#endif


