#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

class Timer {
public:
    Timer(size_t time, const std::function<void(void)>& f);
    Timer();
    ~Timer();

private:
    void wait_then_call()
    {
        std::unique_lock<std::mutex> lck{mtx};
        cv.wait_for(lck, time);
        f();
    }
    std::mutex mtx;
    std::condition_variable cv{};
    std::chrono::milliseconds time;
    std::function <void(void)> f;
    std::thread wait_thread{[this]() {wait_then_call(); }};
};


