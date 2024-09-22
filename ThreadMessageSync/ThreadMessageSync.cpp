#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

// Mutex for managing access to shared information between threads
mutex mtx;

// Condition variables allowing threads to wait for new messages
condition_variable cv_thread1;
condition_variable cv_thread2;
condition_variable cv_thread3;

// Queues for storing messages (numbers) for thread 1 and thread 2
queue<int> thread1_messages, thread2_messages;

// Flags indicating whether there is a new message in each queue
bool new_message_thread1 = false, new_message_thread2 = false;

// The number to send to the threads
int number_to_send;
// The last number sent to each thread
int last_received_thread1 = 0;
int last_received_thread2 = 0;

// The time each thread will wait before sending a new message
int message_delay_seconds = 1;
// The delay time between each comparison check (5 seconds)
int check_interval_seconds = 5;

/**
 * Function to generate a random number between min and max.
 * @param min the minimum possible value.
 * @param max the maximum possible value.
 * @return a random number between min and max.
 */
int generate_random_number(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

/**
 * General function for managing messages.
 * @param message_queue the queue for storing messages.
 * @param cv condition variable allowing the thread to wait for a new message.
 * @param new_message_flag flag indicating whether there is a new message.
 * @param thread_name the name of the thread (for printing purposes).
 */
void threadFunction(queue<int>& message_queue, condition_variable& cv, bool& new_message_flag, const string& thread_name) {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&new_message_flag] { return new_message_flag; }); // Wait for a new message
        message_queue.push(number_to_send); // Add the number to the queue
        cout << "[" << thread_name << "] Received number: " << number_to_send << endl;
        new_message_flag = false; // Update that there is no new message
    }
}

/**
 * Function for thread 3, which performs checks and comparisons between the numbers in the queues.
 */
void threadFunction3() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(check_interval_seconds)); // Wait for 5 seconds

        unique_lock<mutex> lock(mtx);
        cout << "[Thread 3] Checking numbers..." << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;

        // Compare numbers in thread 1 and thread 2's queues
        while (!thread1_messages.empty() && !thread2_messages.empty()) {
            int num1 = thread1_messages.front();
            int num2 = thread2_messages.front();
            cout << "[Thread 3] Thread1_messages.front: " << num1 << endl;
            cout << "[Thread 3] Thread2_messages.front: " << num2 << endl;

            cout << "[Thread 3] Comparing " << num1 << " (Thread 1) and " << num2 << " (Thread 2)" << endl;

            if (num1 == num2) {
                cout << "[Thread 3] Found common number: " << num1 << endl;
                thread1_messages.pop();
                thread2_messages.pop();
            }
            else if (num1 < num2) {
                thread1_messages.pop();
                cout << "[Thread 3] Removing " << num1 << " from Thread 1 queue" << endl;
            }
            else {
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
 * Main function: creates the threads and sends messages at a constant frequency.
 */
int main() {
    cout << "Starting program" << endl;

    // Create threads
    thread t1(threadFunction, ref(thread1_messages), ref(cv_thread1), ref(new_message_thread1), "Thread 1"); // Thread 1
    thread t2(threadFunction, ref(thread2_messages), ref(cv_thread2), ref(new_message_thread2), "Thread 2"); // Thread 2
    thread t3(threadFunction3); // Thread 3

    int option;

    while (true) {
        {
            option = rand() % 3; // Choose a number between 0 and 2
            cout << "[Main] option: " << option << endl;
            unique_lock<mutex> lock(mtx);

            if (option == 0) {
                // Send a message to thread 1
                number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                last_received_thread1 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread1 " << endl;
                new_message_thread1 = true; // Update that there is a new message
                cv_thread1.notify_one(); // Notify thread 1
            }
            else if (option == 1) {
                // Send a message to thread 2
                number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                last_received_thread2 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread2 " << endl;
                new_message_thread2 = true; // Update that there is a new message
                cv_thread2.notify_one(); // Notify thread 2
            }
            else if (option == 2) {
                // Send a message to both threads
                if (last_received_thread1 < last_received_thread2) {
                    number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                }
                else {
                    number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                }
                last_received_thread1 = number_to_send;
                last_received_thread2 = number_to_send;
                new_message_thread1 = true; // Update that there is a new message
                new_message_thread2 = true; // Update that there is a new message
                cv_thread1.notify_one();
                cv_thread2.notify_one(); // Notify both thread 1 and thread 2

                cout << "[Main] Sending number: " << last_received_thread1 << " to both thread1 and thread2 " << endl;
            }
        }
        // Delay before sending the next message
        this_thread::sleep_for(chrono::seconds(message_delay_seconds));
    }

    // Join the threads upon program termination
    t1.join();
    t2.join();
    t3.join();

    return 0; // End of program
}
