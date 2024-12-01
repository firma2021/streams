#include <chrono>
#include <coroutine>
#include <iostream>
#include <thread>
using namespace std;

// 自定义 awaitable 对象
struct Awaiter
{
    bool await_ready() const noexcept { return false; } // 总是挂起
    void await_suspend(std::coroutine_handle<> h) const
    {
        std::thread(
            [h]()
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟异步操作
                h.resume();                                           // 恢复协程
            })
            .detach();
    }
    int await_resume() const noexcept
    {
        cout << this_thread::get_id() << endl;
        return 42; // 返回结果
    }
};

// 协程函数
struct MyCoroutine
{
    struct promise_type
    {
        MyCoroutine get_return_object() { return MyCoroutine {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        auto initial_suspend() { return std::suspend_always {}; }
        auto final_suspend() noexcept { return std::suspend_always {}; }
        void return_void() {}

        void unhandled_exception() {}
    };

    std::coroutine_handle<promise_type> handle;

    MyCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~MyCoroutine()
    {
        if (handle) { handle.destroy(); }
    }
};

// 协程示例
MyCoroutine example()
{
    std::cout << "Starting coroutine." << std::endl;
    int result = co_await Awaiter(); // 等待并获取结果
    std::cout << "Coroutine resumed with result: " << result << std::endl;
}

// 当新线程的 sleep_for 完成后，它会调用 h.resume();，这会恢复协程的执行。此时，协程的后续代码（std::cout << "Coroutine resumed with result: " << result << std::endl;）将在新线程中执行。

int main()
{
    cout << this_thread::get_id() << endl;
    MyCoroutine coro = example();
    coro.handle.resume();                                  // 开始协程
    std::this_thread::sleep_for(std::chrono::seconds(10)); // 主线程等待
    return 0;
}
