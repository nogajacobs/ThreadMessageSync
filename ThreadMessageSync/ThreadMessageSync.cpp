#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

std::mutex mtx;
std::condition_variable cv;
std::vector<int> thread1_messages, thread2_messages;
bool new_message_thread1 = false, new_message_thread2 = false;

void threadFunction1() {
    int count = 1;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::unique_lock<std::mutex> lock(mtx);
        thread1_messages.push_back(count);
        new_message_thread1 = true;
        count++;
        lock.unlock();

        cv.notify_all();
    }
}

void threadFunction2() {
    int count = 1;
    while (true) {
        std::this_thread::
