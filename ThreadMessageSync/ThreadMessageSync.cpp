#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

mutex mtx; // מנעול לניהול גישה למידע משותף בין הת'רדים
condition_variable cv_thread1; // משתנה שמאפשר ל-thread 1 לחכות
condition_variable cv_thread2; // משתנה שמאפשר ל-thread 2 לחכות
condition_variable cv_thread3; // משתנה שמאפשר ל-thread 3 לחכות
queue<int> thread1_messages, thread2_messages; // תורים לאחסון המספרים
bool new_message_thread1 = false, new_message_thread2 = false, new_message_thread3 = false; // האם יש הודעה חדשה בתורים

int number_to_send;
int last_received_thread1 = 0;
int last_received_thread2 = 0;

int message_delay_seconds = 1; // הזמן שיחכה כל תהליך לפני שליחה של הודעה חדשה
int check_interval_seconds = 5; // זמן השהייה בין כל בדיקת השוואה (5 שניות)

int generate_random_number(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// פונקציה לתהליך 1
void threadFunction1() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread1.wait(lock, [] { return new_message_thread1; }); // מחכה להודעה חדשה
        thread1_messages.push(number_to_send); // מוסיף את המספר לתור של תהליך 1
        //int received_count = thread1_messages.front();
        //thread1_messages.pop();
        cout << "[Thread 1] Received number: " << number_to_send << endl;
        //last_received_thread1 = received_count; // מעדכן את המספר האחרון
        new_message_thread1 = false; // מעדכן שאין הודעה חדשה
    }
}

// פונקציה לתהליך 2
void threadFunction2() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv_thread2.wait(lock, [] { return new_message_thread2; }); // מחכה להודעה חדשה
        thread2_messages.push(number_to_send); // מוסיף את המספר לתור של תהליך 2
        //int received_count = thread2_messages.front();
        //thread2_messages.pop();
        cout << "[Thread 2] Received number: " << number_to_send << endl;
        //last_received_thread2 = received_count; // מעדכן את המספר האחרון
        new_message_thread2 = false; // מעדכן שאין הודעה חדשה
    }
}

void threadFunction3() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(check_interval_seconds)); // מחכה 5 שניות

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
    thread t1(threadFunction1); // תהליך 1 פעיל לכל זמן הריצה
    thread t2(threadFunction2); // תהליך 2 פעיל לכל זמן הריצה
    thread t3(threadFunction3); // יוצרת תהליך 3

    int option = rand() % 3; // נבחר מספר בין 0 ל-2

    while (true) {
        {
            option = rand() % 3; // נבחר מספר בין 0 ל-2
            cout << "[Main] option: " << option << endl;
            unique_lock<mutex> lock(mtx);
            if (option == 0) {
                number_to_send = generate_random_number(last_received_thread1+1, last_received_thread1 + 5);
                last_received_thread1 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " send to thread1 " << endl;
                new_message_thread1 = true; // מעדכן שיש הודעה חדשה           
                cv_thread1.notify_one(); // מיידעת את thread 1
            }
            else if(option == 1){
                number_to_send = generate_random_number(last_received_thread2+1, last_received_thread2 + 5);
                last_received_thread2 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " send to thread2 " << endl;
                new_message_thread2 = true; // מעדכן שיש הודעה חדשה
                cv_thread2.notify_one(); // מיידעת את thread 2
            }
            else if (option == 2) {
                // אופציה שליחה לשני התהליכים
                if (last_received_thread1 < last_received_thread2) {
                    number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                }
                else if (last_received_thread2 <= last_received_thread1) {
                    number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                }
                last_received_thread1 = number_to_send;
                last_received_thread2 = number_to_send;
                new_message_thread1 = true; // מעדכן שיש הודעה חדשה           
                new_message_thread2 = true; // מעדכן שיש הודעה חדשה
                cv_thread1.notify_one(), cv_thread2.notify_one(); // מיידעת את thread 1 ו-thread 2


                cout << "[Main] Sending number: " << last_received_thread1 << " send to thread1 " << endl;
                cout << "[Main] Sending number: " << last_received_thread2 << " send to thread2 " << endl;
            }
        }
        this_thread::sleep_for(chrono::seconds(message_delay_seconds));
    }

    t1.join(); // צירוף התהליכים
    t2.join();
    t3.join();

    return 0; // סיום התוכנית
}
