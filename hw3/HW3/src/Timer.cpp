#include "Timer.h"

Timer::Timer() {
    start_time = clock();
    elapsed_time = 0;
}

void Timer::start() {
    start_time = clock();
}

void Timer::stop() {
    elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
}

void Timer::stop_acc() {
    elapsed_time += (double)(clock() - start_time) / CLOCKS_PER_SEC;
}

bool Timer::is_timeout(double t) {
    stop();
    return elapsed_time > t;
}

double Timer::get_elapsed_time() {
    return elapsed_time;
}