#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

// ������ ��������
mutex mtx; // ����� ������ ���� ����� ����� ��� ��'����
condition_variable cv; // ����� ������ ��'���� ����� ������ �������
queue<int> thread1_messages, thread2_messages; // ����� ������ ������� �������� ������ 1 ������ 2
bool new_message_thread1 = false, new_message_thread2 = false; // ��� �� ����� ���� ������

// ������� �����
int message_delay_seconds = 1; // ���� ����� �� ����� ���� ����� �� ����� ����
int check_interval_seconds = 5; // ���� ����� ����� 3 ���� ����� ����
int max_initial_random_number = 10; // ����� ������ ������� ���������� ���������
int max_increment = 10; // ����� ������ ������ �������
int increment_value = 10; // ��� ������ ���� �� ����

// ������� ������ ���� �������
int generate_random_number(int previous_number, bool is_first) {
    random_device rd; // ����� ������ ����� ���������
    mt19937 gen(rd()); // ������ ������ ���������
    // ������� ������ ���������
    uniform_int_distribution<> dist(
        is_first ? 0 : previous_number + 1,
        is_first ? max_initial_random_number : previous_number + max_increment
    );
    return dist(gen); // ������ ���� �������
}

// ������� ������ 1
void threadFunction1() {
    int count = generate_random_number(0, true); // ����� �� ���� ������� ��� 0 �-10
    while (true) {
        this_thread::sleep_for(chrono::seconds(message_delay_seconds)); // ������
        unique_lock<mutex> lock(mtx); // ���� �� ������
        if (rand() % 2 == 0) { // ���� �� ���� ����� ����� ����
            count = generate_random_number(count, false); // ���� ���� ������� ���
            thread1_messages.push(count); // ����� �� ����� ���� �� ����� 1
            new_message_thread1 = true; // ����� ��� ����� ���� ���� �� ����� 1
            cout << "[Thread 1] Received number: " << count << endl; // ����� �� �����
            count += increment_value; // ����� �� ����� ����� ������ ����
            cv.notify_all(); // ����� �� �� ��'����
        }
        lock.unlock(); // ����� �� ������
    }
}

// ������� ������ 2
void threadFunction2() {
    int count = generate_random_number(0, true); // ����� �� ���� ������� ��� 0 �-10
    while (true) {
        this_thread::sleep_for(chrono::seconds(message_delay_seconds)); // ������
        unique_lock<mutex> lock(mtx); // ���� �� ������
        if (rand() % 2 == 0) { // ���� �� ���� ����� ����� ����
            count = generate_random_number(count, false); // ���� ���� ������� ���
            thread2_messages.push(count); // ����� �� ����� ���� �� ����� 2
            new_message_thread2 = true; // ����� ��� ����� ���� ���� �� ����� 2
            cout << "[Thread 2] Received number: " << count << endl; // ����� �� �����
            count += increment_value; // ����� �� ����� ����� ������ ����
            cv.notify_all(); // ����� �� �� ��'����
        }
        lock.unlock(); // ����� �� ������
    }
}

// ������� ������ 3
void threadFunction3() {
    while (true) {
        unique_lock<mutex> lock(mtx); // ���� �� ������
        cv.wait_for(lock, chrono::seconds(check_interval_seconds), [] {
            return new_message_thread1 || new_message_thread2;
            }); // ���� ������ �������� �� �� ���� ������

        if (!new_message_thread1 && !new_message_thread2) { // �� ��� ������ �����
            continue; // ���� ������
        }

        cout << "[Thread 3] Checking numbers..." << endl; // ����� ����� ������

        // ����� ��� ������� ������ �� ����� 1 ������ 2
        while (!thread1_messages.empty() && !thread2_messages.empty()) {
            int num1 = thread1_messages.front(); // ���� �� ����� ������
            int num2 = thread2_messages.front(); // ���� �� ����� ����

            cout << "[Thread 3] Comparing " << num1 << " (Thread 1) and " << num2 << " (Thread 2)" << endl; // ����� �� ������� �������

            if (num1 == num2) { // �� ������� ������
                cout << "[Thread 3] Found common number: " << num1 << endl; // ����� �� ����� ������
                thread1_messages.pop(); // ���� �� ����� ����� �� ����� 1
                thread2_messages.pop(); // ���� �� ����� ����� �� ����� 2
            }
            else if (num1 < num2) {
                thread1_messages.pop(); // ���� �� ����� ����� �� ����� 1
            }
            else {
                thread2_messages.pop(); // ���� �� ����� ����� �� ����� 2
            }
        }

        new_message_thread1 = false; // ����� ���� ������ ����� ���� �� ����� 1
        new_message_thread2 = false; // ����� ���� ������ ����� ���� �� ����� 2
    }
}

// ������� �����
int main() {
    // ����� �'����
    thread t1(threadFunction1);
    thread t2(threadFunction2);
    thread t3(threadFunction3);

    // ����� ����� ��'����
    t1.join();
    t2.join();
    t3.join();

    return 0;
}
