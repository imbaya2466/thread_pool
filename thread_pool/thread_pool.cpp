#include "thread_pool.h"


// the constructor just launches some amount of workers
ThreadPool::ThreadPool(uint32_t core_threads, uint32_t max_threads, uint32_t max_queue, uint32_t time_alive_nowork_ms) :
    core_threads_(core_threads),
    max_threads_(max_threads),
    max_queue_(max_queue),
    time_alive_nowork_ms_(time_alive_nowork_ms),
    stop_(false)
{
  AddThread(core_threads_);
}

// the destructor joins all threads
ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
  for (std::thread& worker : workers_)
    worker.join();
}

bool
ThreadPool::SetFit()
{
  if (tasks_.size() < max_queue_) {
    return true;
  }
  if (workers_.size()<max_threads_) {
    AddThread(1);
    return true;
  }
  return false;
}

bool
ThreadPool::AddThread(uint32_t add_size)
{
  for (size_t i = 0; i < add_size; ++i) {
    workers_.emplace_back([this] {
      for (;;) {
        std::function<void()> task;

        {
          //TODO:当线程数量>core_threads_且闲时超过time_alive_nowork_ms结束线程
          std::unique_lock<std::mutex> lock(this->queue_mutex_);
          this->condition_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); });
          if (this->stop_ && this->tasks_.empty()) return;
          task = std::move(this->tasks_.front());
          this->tasks_.pop();
        }
        task();
      }
    });
  }
  return false;
}


// c interface
typedef void *thread_pool;

thread_pool
thpool_init(uint32_t core_threads, uint32_t max_threads, uint32_t max_queue, uint32_t time_alive_nowork_ms)
{
  thread_pool ret = new ThreadPool(core_threads, max_threads,max_queue,time_alive_nowork_ms);
  return ret;
}
bool
thpool_add_work(thread_pool t_pool, void (*function_p)(void *), void *arg_p)
{
  //TODO catch异常返回
  ((ThreadPool*) t_pool)->enqueue(function_p, arg_p);
  return true;
}
void
thpool_destroy(thread_pool t_pool)
{
  if (t_pool) {
    delete ((ThreadPool *)t_pool);
  }
}


