#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

// Mutex for managing access to shared data between threads
mutex mtx;

// Condition variables - each allows the corresponding thread to wait for a message
condition_variable cv_thread1;
condition_variable cv_thread2;
condition_variable cv_thread3;

// Queues for storing messages (numbers) for thread 1 and thread 2
queue<int> thread1_messages, thread2_messages;

// Flags indicating whether there is a new message in each queue
bool new_message_thread1 = false;
bool new_message_thread2 = false;
bool new_message_thread3 = false;

// The number to send to the threads
int number_to_send;

// Variables to store the last number sent to thread 1 and thread 2
int last_received_thread1 = 0;
int last_received_thread2 = 0;

// Time the thread will wait before sending a new message
int message_delay_seconds = 1;

// Time between each check of thread 3 (5 seconds)
int check_interval_seconds = 5;

/**
 * Function to generate a random number between min and max.
 * @param min the lowest possible value.
 * @param max the highest possible value.
 * @return a random number between min and max.
 */
int generate_random_number(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

/**
 * Function for thread 1: waits for a message, receives a number, and stores it in the queue.
 */
void threadFunction1() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread1.wait(lock, [] { return new_message_thread1; }); // Wait for a new message
        thread1_messages.push(number_to_send); // Add the number to thread 1's queue
        cout << "[Thread 1] Received number: " << number_to_send << endl;
        new_message_thread1 = false; // Update to indicate there is no new message
    }
}

/**
 * Function for thread 2: waits for a message, receives a number, and stores it in the queue.
 */
void threadFunction2() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread2.wait(lock, [] { return new_message_thread2; }); // Wait for a new message
        thread2_messages.push(number_to_send); // Add the number to thread 2's queue
        cout << "[Thread 2] Received number: " << number_to_send << endl;
        new_message_thread2 = false; // Update to indicate there is no new message
    }
}

/**
 * Function for thread 3: checks the queues of thread 1 and thread 2 every 5 seconds and compares the numbers.
 */
void threadFunction3() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(check_interval_seconds)); // Wait for 5 seconds

        unique_lock<mutex> lock(mtx);
        cout << "[Thread 3] Checking numbers..." << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;

        // Compare the numbers in thread 1's queue and thread 2's queue
        while (!thread1_messages.empty() && !thread2_messages.empty()) {
            int num1 = thread1_messages.front();
            int num2 = thread2_messages.front();
            cout << "[Thread 3] Thread1_messages.front: " << num1 << endl;
            cout << "[Thread 3] Thread2_messages.front: " << num2 << endl;

            cout << "[Thread 3] Comparing " << num1 << " (Thread 1) and " << num2 << " (Thread 2)" << endl;

            if (num1 == num2) {
                // If the numbers are equal, remove them from both queues
                cout << "[Thread 3] Found common number: " << num1 << endl;
                thread1_messages.pop();
                thread2_messages.pop();
            }
            else if (num1 < num2) {
                // If the number in thread 1's queue is smaller, remove it from this queue
                thread1_messages.pop();
                cout << "[Thread 3] Removing " << num1 << " from Thread 1 queue" << endl;
            }
            else {
                // If the number in thread 2's queue is smaller, remove it from this queue
                thread2_messages.pop();
                cout << "[Thread 3] Removing " << num2 << " from Thread 2 queue" << endl;
            }
        }

        cout << "[Thread 3] After checking numbers..." << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;
    }
}

/**
 * Main function: creates all the threads and sends messages at regular intervals.
 */
int main() {
    cout << "Starting program" << endl;

    // Create threads
    thread t1(threadFunction1); // thread 1 active for the entire run time
    thread t2(threadFunction2); // thread 2 active for the entire run time
    thread t3(threadFunction3); // creates thread 3 for checking

    int option = rand() % 3; // choose a number between 0 and 2

    while (true) {
        {
            option = rand() % 3; // choose a number between 0 and 2
            cout << "[Main] option: " << option << endl;
            unique_lock<mutex> lock(mtx);

            if (option == 0) {
                // Send a message to thread 1
                number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                last_received_thread1 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread 1" << endl;
                new_message_thread1 = true; // update to indicate there is a new message
                cv_thread1.notify_one(); // notify thread 1
            }
            else if (option == 1) {
                // Send a message to thread 2
                number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                last_received_thread2 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread 2" << endl;
                new_message_thread2 = true; // update to indicate there is a new message
                cv_thread2.notify_one(); // notify thread 2
            }
            else if (option == 2) {
                // Send a message to both threads simultaneously
                if (last_received_thread1 < last_received_thread2) {
                    number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                }
                else if (last_received_thread2 <= last_received_thread1) {
                    number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                }

                last_received_thread1 = number_to_send;
                last_received_thread2 = number_to_send;
                new_message_thread1 = true; // update to indicate there is a new message
                new_message_thread2 = true; // update to indicate there is a new message
                cv_thread1.notify_one();
                cv_thread2.notify_one(); // notify thread 1 and thread 2

                cout << "[Main] Sending number: " << last_received_thread1 << " to thread 1" << endl;
                cout << "[Main] Sending number: " << last_received_thread2 << " to thread 2" << endl;
            }
        }
        // Delay before sending the next message
        this_thread::sleep_for(chrono::seconds(message_delay_seconds));
    }

    // Join threads at program termination
    t1.join();
    t2.join();
    t3.join();

    return 0; // End of the program
}
