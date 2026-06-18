#pragma once

#include <helios/async/details/profile.hpp>

#include <taskflow/taskflow.hpp>

#include <chrono>
#include <future>

namespace helios::async {

/**
 * @class Future
 * @brief Wrapper around `tf::Future` for handling asynchronous task results.
 * @details Provides methods to wait for and retrieve results from asynchronous
 * operations. Move-only type that represents the eventual result of an async
 * computation.
 * @note All member functions are thread-safe.
 * @tparam T The type of value the future will hold
 */
template <typename T>
class Future {
public:
  Future() = default;
  Future(const Future&) = delete;
  Future(Future&& other) noexcept = default;
  ~Future() = default;

  Future& operator=(const Future&) = delete;
  Future& operator=(Future&& other) noexcept = default;

  /**
   * @brief Blocks until the result is available and returns it.
   * @details This method blocks the calling thread. Can only be called once.
   * @return The result of the asynchronous operation
   */
  T Get();

  /**
   * @brief Attempts to cancel the associated task.
   * @details May not succeed if the task has already started or completed.
   * @return True if cancellation was successful, false otherwise
   */
  bool Cancel();

  /**
   * @brief Blocks until the result is available.
   * @details Unlike `Get()`, this doesn't retrieve the result and can be called
   * multiple times.
   */
  void Wait();

  /**
   * @brief Waits for the result to become available for the specified duration.
   * @tparam Rep Duration representation type
   * @tparam Period Duration period type
   * @param rel_time Maximum time to wait
   * @return Status indicating whether the result is ready, timeout occurred, or
   * task was deferred
   */
  template <typename Rep, typename Period>
    requires requires { typename std::chrono::duration<Rep, Period>; }
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& rel_time) const;

  /**
   * @brief Waits for the result to become available until the specified time
   * point.
   * @tparam Clock Clock type
   * @tparam Duration Duration type
   * @param abs_time Absolute time point to wait until
   * @return Status indicating whether the result is ready, timeout occurred, or
   * task was deferred
   */
  template <typename Clock, typename Duration>
    requires requires { typename std::chrono::time_point<Clock, Duration>; }
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& abs_time) const;

  /**
   * @brief Checks if the future has a shared state.
   * @details A future becomes invalid after Get() is called or if it was
   * default-constructed.
   * @return True if the future is valid (has an associated task), false
   * otherwise
   */
  [[nodiscard]] bool Valid() const { return future_.valid(); }

private:
  explicit Future(tf::Future<T> future) : future_(std::move(future)) {}

  mutable tf::Future<T> future_;

  friend class Executor;
  friend class SubTaskGraph;
};

template <typename T>
inline T Future<T>::Get() {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Future::Get");
  return future_.get();
}

template <typename T>
inline bool Future<T>::Cancel() {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Future::Cancel");
  return future_.cancel();
}

template <typename T>
inline void Future<T>::Wait() {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Future::Wait");
  future_.wait();
}

template <typename T>
template <typename Rep, typename Period>
  requires requires { typename std::chrono::duration<Rep, Period>; }
inline std::future_status Future<T>::WaitFor(
    const std::chrono::duration<Rep, Period>& rel_time) const {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Future::WaitFor");
  return future_.wait_for(rel_time);
}

template <typename T>
template <typename Clock, typename Duration>
  requires requires { typename std::chrono::time_point<Clock, Duration>; }
inline std::future_status Future<T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& abs_time) const {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Future::WaitUntil");
  return future_.wait_until(abs_time);
}

}  // namespace helios::async
