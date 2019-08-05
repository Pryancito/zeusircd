#include "Timer.h"

Timer::Timer(size_t time, const std::function<void(void)>& f) : time{std::chrono::milliseconds{time}}, f{f} {}
Timer::Timer() {}
Timer::~Timer() { }
