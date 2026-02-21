#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

/**
 * @brief A simple and robust thread pool implementation.
 *
 * This class provides a thread pool that can be used to execute tasks concurrently.
 * It manages a collection of worker threads and a queue of tasks.
 * Users can enqueue tasks, and the thread pool will execute them using the available threads.
 */
class ThreadPool {
public:
    /**
     * @brief Constructs a ThreadPool with the number of threads equal to the hardware concurrency.
     * @param on_thread_start A function to be called by each thread when it starts.
     * @param on_thread_stop A function to be called by each thread when it stops.
     * @throws std::runtime_error if the number of hardware concurrency is not available.
     */
    ThreadPool(
        std::function<void()> on_thread_start = nullptr, 
        std::function<void()> on_thread_stop = nullptr
    );

    /**
     * @brief Constructs a ThreadPool with a specified number of threads.
     * @param num_threads The number of threads to create in the pool.
     * @param on_thread_start A function to be called by each thread when it starts.
     * @param on_thread_stop A function to be called by each thread when it stops.
     * @throws std::runtime_error if num_threads is 0.
     */
    ThreadPool(
        size_t num_threads, 
        std::function<void()> on_thread_start = nullptr, 
        std::function<void()> on_thread_stop = nullptr
    );
    
    /**
     * @brief Destructor that joins all threads.
     *
     * This will block until all tasks in the queue are completed.
     * After the destructor is called, no more tasks can be enqueued.
     */
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Enqueues a task to be executed by the thread pool.
     * @tparam Func The type of the function to be executed.
     * @tparam Args The types of the arguments to be passed to the function.
     * @param func The function to be executed.
     * @param args The arguments to be passed to the function.
     * @return A std::future that will hold the result of the function's execution.
     * @throws std::runtime_error if the thread pool is already terminated.
     */
    template<typename Func, typename... Args>
    std::future<std::invoke_result_t<Func, Args...>> Enqueue(Func&& func, Args&&... args);

    /**
     * @brief Applies a function to each element in a range in parallel.
     *
     * The range [first, last) is divided into chunks, and each chunk is processed by a separate task in the thread pool.
     * This function blocks until all tasks are completed.
     *
     * @tparam Iterator The type of the iterator.
     * @tparam Func The type of the function to be applied.
     * @param first An iterator to the beginning of the range.
     * @param last An iterator to the end of the range.
     * @param func The function to be applied to each element.
     */
    template<typename Iterator, typename Func>
    void ForEach(Iterator first, Iterator last, Func func, std::optional<size_t> num_chunks = std::nullopt);

    /**
     * @brief Applies a transform function to each element in a range and then reduces the results in parallel.
     *
     * The range [first, last) is divided into chunks. For each chunk, the transform function is applied to each element,
     * and the results are reduced to a single value. The results from all chunks are then reduced to a final result.
     * This function blocks until all tasks are completed.
     *
     * @tparam Iterator The type of the iterator.
     * @tparam T The type of the initial value and the result.
     * @tparam TransformFunc The type of the transform function.
     * @tparam ReduceFunc The type of the reduce function.
     * @param first An iterator to the beginning of the range.
     * @param last An iterator to the end of the range.
     * @param init The initial value for the reduction. It's crucial to pass the identity element of the reduction operation
     *             to avoid issues during the chunk-wise reduction process.
     * @param transform The function to be applied to each element.
     * @param reduce The function to combine the transformed results.
     * @return The result of the transform-reduce operation.
     */
    template<typename Iterator, typename T, typename TransformFunc, typename ReduceFunc>
    T TransformReduce(Iterator first, Iterator last, T init, TransformFunc transform, ReduceFunc reduce, std::optional<size_t> num_chunks = std::nullopt);

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::condition_variable cv_task_;
    std::mutex m_task_;

    size_t num_threads_;
    bool is_terminated_;
};

template<typename Func, typename... Args>
inline std::future<std::invoke_result_t<Func, Args...>> ThreadPool::Enqueue(Func&& func, Args&&... args) {
    static_assert(std::is_invocable_v<Func, Args...>, "Func must be callable with Args.");

    auto task = std::make_shared<std::packaged_task<std::invoke_result_t<Func, Args...>()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
    );

    std::future<std::invoke_result_t<Func, Args...>> task_future = task->get_future();
    {
        std::lock_guard<std::mutex> lock(m_task_);
        if (is_terminated_) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task](){ (*task)(); });
    }
    cv_task_.notify_one();

    return task_future;
}

template<typename Iterator, typename Func>
inline void ThreadPool::ForEach(Iterator first, Iterator last, Func func, std::optional<size_t> num_chunks) {
    static_assert(
        std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>,
        "Iterator must be at least an input iterator."
    );

    static_assert(
        std::is_invocable_v<Func, decltype(*first)>,
        "Func must be callable with dereferenced iterator type."
    );

    auto total_size = std::distance(first, last);
    if (total_size == 0) {
        return;
    }

    if (!num_chunks) {
        num_chunks = num_threads_;
    } 

    size_t chunk_size = (total_size + *num_chunks - 1) / *num_chunks;

    std::vector<std::future<void>> task_futures;
    task_futures.reserve(*num_chunks);

    Iterator begin = first;
    while (begin != last) {
        Iterator end = begin;
        std::advance(end, std::min(chunk_size, static_cast<size_t>(std::distance(begin, last))));

        task_futures.emplace_back(
            Enqueue([begin, end, func]() {
                for (auto it = begin; it != end; ++it) {
                    func(*it);
                }
            })
        );
        begin = end;
    }

    for (auto& task_future : task_futures) {
        task_future.get();
    }
}

template<typename Iterator, typename T, typename TransformFunc, typename ReduceFunc>
inline T ThreadPool::TransformReduce(Iterator first, Iterator last, T init, TransformFunc transform, ReduceFunc reduce, std::optional<size_t> num_chunks) {
    static_assert(
        std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>,
        "Iterator must be at least an input iterator."
    );

    static_assert(
        std::is_invocable_v<TransformFunc, decltype(*first)>,
        "TransformFunc must be callable with dereferenced iterator type."
    );

    using TransformResult = std::invoke_result_t<TransformFunc, decltype(*first)>;
    static_assert(
        std::is_invocable_v<ReduceFunc, T, TransformResult>,
        "ReduceFunc must be callable with accumulator T and TransformFunc result."
    );

    auto total_size = std::distance(first, last);
    if (total_size == 0) {
        return init;
    }

    if (!num_chunks) {
        num_chunks = num_threads_;
    }

    size_t chunk_size = (total_size + *num_chunks - 1) / *num_chunks;

    std::vector<std::future<T>> task_futures;
    task_futures.reserve(*num_chunks);

    Iterator begin = first;
    while (begin != last) {
        Iterator end = begin;
        std::advance(end, std::min(chunk_size, static_cast<size_t>(std::distance(begin, last))));
        task_futures.emplace_back(
            Enqueue([begin, end, init, transform, reduce]() -> T {
                T local_result = init;
                for (auto it = begin; it != end; ++it) {
                    local_result = reduce(local_result, transform(*it));
                }
                return local_result;
            })
        );
        begin = end;
    }

    T result = init;
    for (auto& task_future : task_futures) {
        result = reduce(result, task_future.get());
    }
    
    return result;
}
