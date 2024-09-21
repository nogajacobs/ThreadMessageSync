#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

mutex mtx; // ����� ������ ���� ����� ����� ��� ��'����
condition_variable cv_thread1; // ����� ������ �-thread 1 �����
condition_variable cv_thread2; // ����� ������ �-thread 2 �����
condition_variable cv_thread3; // ����� ������ �-thread 3 �����
queue<int> thread1_messages, thread2_messages; // ����� ������ �������
bool new_message_thread1 = false, new_message_thread2 = false, new_message_thread3 = false; // ��� �� ����� ���� ������

int number_to_send;
int last_received_thread1 = 0;
int last_received_thread2 = 0;

int message_delay_seconds = 1; // ���� ����� �� ����� ���� ����� �� ����� ����
int check_interval_seconds = 5; // ��� ������ ��� �� ����� ������ (5 �����)

int generate_random_number(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// ������� ������ 1
void threadFunction1() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread1.wait(lock, [] { return new_message_thread1; }); // ���� ������ ����
        thread1_messages.push(number_to_send); // ����� �� ����� ���� �� ����� 1
        //int received_count = thread1_messages.front();
        //thread1_messages.pop();
        cout << "[Thread 1] Received number: " << number_to_send << endl;
        //last_received_thread1 = received_count; // ����� �� ����� ������
        new_message_thread1 = false; // ����� ���� ����� ����
    }
}

// ������� ������ 2
void threadFunction2() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread2.wait(lock, [] { return new_message_thread2; }); // ���� ������ ����
        thread2_messages.push(number_to_send); // ����� �� ����� ���� �� ����� 2
        //int received_count = thread2_messages.front();
        //thread2_messages.pop();
        cout << "[Thread 2] Received number: " << number_to_send << endl;
        //last_received_thread2 = received_count; // ����� �� ����� ������
        new_message_thread2 = false; // ����� ���� ����� ����
    }
}

void threadFunction3() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(check_interval_seconds)); // ���� 5 �����

        unique_lock<mutex> lock(mtx);
        cout << "[Thread 3] Checking numbers..." << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;

        while (!thread1_messages.empty() && !thread2_messages.empty()) {
            int num1 = thread1_messages.front();
            int num2 = thread2_messages.front();
            cout << "[Thread 3] Thread1_messages.front: " << num1 << endl;
            cout << "[Thread 3] Thread1_messages.front: " << num2 << endl;

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
        cout << "[Thread 3] After checking  numbers...  " << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;
    }
}

int main() {
    cout << "Starting program" << endl;
    thread t1(threadFunction1); // ����� 1 ���� ��� ��� �����
    thread t2(threadFunction2); // ����� 2 ���� ��� ��� �����
    thread t3(threadFunction3); // ����� ����� 3

    int option = rand() % 3; // ���� ���� ��� 0 �-2

    while (true) {
        {
            option = rand() % 3; // ���� ���� ��� 0 �-2
            cout << "[Main] option: " << option << endl;
            unique_lock<mutex> lock(mtx);
            if (option == 0) {
                number_to_send = generate_random_number(last_received_thread1+1, last_received_thread1 + 5);
                last_received_thread1 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " send to thread1 " << endl;
                new_message_thread1 = true; // ����� ��� ����� ����           
                cv_thread1.notify_one(); // ������ �� thread 1
            }
            else if(option == 1){
                number_to_send = generate_random_number(last_received_thread2+1, last_received_thread2 + 5);
                last_received_thread2 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " send to thread2 " << endl;
                new_message_thread2 = true; // ����� ��� ����� ����
                cv_thread2.notify_one(); // ������ �� thread 2
            }
            else if (option == 2) {
                // ������ ����� ���� ��������
                if (last_received_thread1 < last_received_thread2) {
                    number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                }
                else if (last_received_thread2 <= last_received_thread1) {
                    number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                }
                last_received_thread1 = number_to_send;
                last_received_thread2 = number_to_send;
                new_message_thread1 = true; // ����� ��� ����� ����           
                new_message_thread2 = true; // ����� ��� ����� ����
                cv_thread1.notify_one(), cv_thread2.notify_one(); // ������ �� thread 1 �-thread 2


                cout << "[Main] Sending number: " << last_received_thread1 << " send to thread1 " << endl;
                cout << "[Main] Sending number: " << last_received_thread2 << " send to thread2 " << endl;
            }
        }
        this_thread::sleep_for(chrono::seconds(message_delay_seconds));
    }

    t1.join(); // ����� ��������
    t2.join();
    t3.join();

    return 0; // ���� �������
}
