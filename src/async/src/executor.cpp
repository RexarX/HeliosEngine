#include <pch.hpp>

#include <helios/assert.hpp>
#include <helios/async/details/profile.hpp>
#include <helios/async/executor.hpp>
#include <impls.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace helios::async {

Executor::Executor() = default;
Executor::Executor(size_t worker_thread_count) : pimpl_(worker_thread_count) {}
Executor::~Executor() = default;

auto Executor::Run(TaskGraph& graph) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run(details::TaskflowAccess::Get(graph)));
}

auto Executor::Run(TaskGraph&& graph) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run(details::TaskflowAccess::Get(std::move(graph))));
}

auto Executor::Run(TaskGraph& graph, UniqueFunction<void()> callable)
    -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run(details::TaskflowAccess::Get(graph),
                           details::AdaptCallable(std::move(callable))));
}

auto Executor::Run(TaskGraph&& graph, UniqueFunction<void()> callable)
    -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run(details::TaskflowAccess::Get(std::move(graph)),
                           details::AdaptCallable(std::move(callable))));
}

auto Executor::RunN(TaskGraph& graph, size_t count) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_n(details::TaskflowAccess::Get(graph), count));
}

auto Executor::RunN(TaskGraph&& graph, size_t count) -> Future<void> {
  return details::TaskflowAccess::Make(pimpl_->executor.run_n(
      details::TaskflowAccess::Get(std::move(graph)), count));
}

auto Executor::RunN(TaskGraph& graph, size_t count,
                    UniqueFunction<void()> callable) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_n(details::TaskflowAccess::Get(graph), count,
                             details::AdaptCallable(std::move(callable))));
}

auto Executor::RunN(TaskGraph&& graph, size_t count,
                    UniqueFunction<void()> callable) -> Future<void> {
  return details::TaskflowAccess::Make(pimpl_->executor.run_n(
      details::TaskflowAccess::Get(std::move(graph)), count,
      details::AdaptCallable(std::move(callable))));
}

auto Executor::RunUntil(TaskGraph& graph, UniqueFunction<bool()> predicate)
    -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_until(details::TaskflowAccess::Get(graph),
                                 details::AdaptCallable(std::move(predicate))));
}

auto Executor::RunUntil(TaskGraph&& graph, UniqueFunction<bool()> predicate)
    -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_until(details::TaskflowAccess::Get(std::move(graph)),
                                 details::AdaptCallable(std::move(predicate))));
}

auto Executor::RunUntil(TaskGraph& graph, UniqueFunction<bool()> predicate,
                        UniqueFunction<void()> callable) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_until(details::TaskflowAccess::Get(graph),
                                 details::AdaptCallable(std::move(predicate)),
                                 details::AdaptCallable(std::move(callable))));
}

auto Executor::RunUntil(TaskGraph&& graph, UniqueFunction<bool()> predicate,
                        UniqueFunction<void()> callable) -> Future<void> {
  return details::TaskflowAccess::Make(
      pimpl_->executor.run_until(details::TaskflowAccess::Get(std::move(graph)),
                                 details::AdaptCallable(std::move(predicate)),
                                 details::AdaptCallable(std::move(callable))));
}

auto Executor::Async(UniqueFunction<void()> callable) -> std::future<void> {
  return AsyncTyped<void>(std::move(callable));
}

auto Executor::Async(std::string name, UniqueFunction<void()> callable)
    -> std::future<void> {
  return AsyncTyped<void>(std::move(name), std::move(callable));
}

void Executor::SilentAsync(UniqueFunction<void()> callable) {
  pimpl_->executor.silent_async(details::AdaptCallable(std::move(callable)));
}

void Executor::SilentAsync(std::string name, UniqueFunction<void()> callable) {
  tf::TaskParams params;
  params.name = std::move(name);
  pimpl_->executor.silent_async(params,
                                details::AdaptCallable(std::move(callable)));
}

auto Executor::DependentAsync(UniqueFunction<void()> callable,
                              std::span<const AsyncTask> dependencies)
    -> std::pair<AsyncTask, std::future<void>> {
  return DependentAsyncTyped<void>(std::move(callable), dependencies);
}

AsyncTask Executor::SilentDependentAsync(
    UniqueFunction<void()> callable, std::span<const AsyncTask> dependencies) {
  auto tf_deps = details::ToTfAsyncTasks(dependencies);
  return details::TaskflowAccess::Make(pimpl_->executor.silent_dependent_async(
      details::AdaptCallable(std::move(callable)), tf_deps.begin(),
      tf_deps.end()));
}

void Executor::WaitForAll() {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Executor::WaitForAll");
  pimpl_->executor.wait_for_all();
}

void Executor::CoRun(TaskGraph& graph) {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Executor::CoRun");
  HELIOS_ASSERT(IsWorkerThread(), "Must be called from a worker thread!");
  pimpl_->executor.corun(details::TaskflowAccess::Get(graph));
}

void Executor::CoRunUntil(UniqueFunction<bool()> predicate) {
  HELIOS_ASYNC_PROFILE_SCOPE_N("helios::async::Executor::CoRunUntil");
  HELIOS_ASSERT(IsWorkerThread(), "Must be called from a worker thread!");
  pimpl_->executor.corun_until(details::AdaptCallable(std::move(predicate)));
}

int Executor::CurrentWorkerId() const {
  return pimpl_->executor.this_worker_id();
}

size_t Executor::WorkerCount() const noexcept {
  return pimpl_->executor.num_workers();
}

size_t Executor::IdleWorkerCount() const noexcept {
  return pimpl_->executor.num_waiters();
}

size_t Executor::QueueCount() const noexcept {
  return pimpl_->executor.num_queues();
}

size_t Executor::RunningTopologyCount() const {
  return pimpl_->executor.num_topologies();
}

template <typename R>
auto Executor::AsyncTyped(UniqueFunction<R()> callable) -> std::future<R> {
  return pimpl_->executor.async(details::AdaptCallable(std::move(callable)));
}

template <typename R>
auto Executor::AsyncTyped(std::string name, UniqueFunction<R()> callable)
    -> std::future<R> {
  tf::TaskParams params;
  params.name = std::move(name);
  return pimpl_->executor.async(params,
                                details::AdaptCallable(std::move(callable)));
}

template <typename R>
auto Executor::DependentAsyncTyped(UniqueFunction<R()> callable,
                                   std::span<const AsyncTask> dependencies)
    -> std::pair<AsyncTask, std::future<R>> {
  auto tf_deps = details::ToTfAsyncTasks(dependencies);
  auto [task, future] = pimpl_->executor.dependent_async(
      details::AdaptCallable(std::move(callable)), tf_deps.begin(),
      tf_deps.end());
  return {details::TaskflowAccess::Make(std::move(task)), std::move(future)};
}

template auto Executor::AsyncTyped<void>(UniqueFunction<void()>)
    -> std::future<void>;
template auto Executor::AsyncTyped<int>(UniqueFunction<int()>)
    -> std::future<int>;
template auto Executor::AsyncTyped<void>(std::string, UniqueFunction<void()>)
    -> std::future<void>;
template auto Executor::AsyncTyped<int>(std::string, UniqueFunction<int()>)
    -> std::future<int>;
template auto Executor::DependentAsyncTyped<void>(UniqueFunction<void()>,
                                                  std::span<const AsyncTask>)
    -> std::pair<AsyncTask, std::future<void>>;
template auto Executor::DependentAsyncTyped<int>(UniqueFunction<int()>,
                                                 std::span<const AsyncTask>)
    -> std::pair<AsyncTask, std::future<int>>;

}  // namespace helios::async
