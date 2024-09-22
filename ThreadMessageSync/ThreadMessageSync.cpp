#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

// מנעול לניהול גישה למידע משותף בין הת'רדים
mutex mtx;

// משתנים שמאפשרים לתהליכים לחכות להודעות חדשות
condition_variable cv_thread1;
condition_variable cv_thread2;
condition_variable cv_thread3;

// תורים לאחסון מספרים עבור thread 1 ו-thread 2
queue<int> thread1_messages, thread2_messages;

// דגלים שמציינים האם יש הודעה חדשה בתורים
bool new_message_thread1 = false, new_message_thread2 = false;

// המספר שישלח לתהליכים
int number_to_send;
// המספר האחרון שנשלח לכל תהליך
int last_received_thread1 = 0;
int last_received_thread2 = 0;

// הזמן שיחכה כל תהליך לפני שליחה של הודעה חדשה
int message_delay_seconds = 1;
// זמן ההשהיה בין כל בדיקת השוואה (5 שניות)
int check_interval_seconds = 5;

/**
 * פונקציה ליצירת מספר אקראי בין min ל-max.
 * @param min הערך הנמוך ביותר האפשרי.
 * @param max הערך הגבוה ביותר האפשרי.
 * @return מספר אקראי בין min ל-max.
 */
int generate_random_number(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

/**
 * פונקציה כללית לניהול ההודעות.
 * @param message_queue תור לאחסון ההודעות.
 * @param cv משתנה תנאי המאפשר לתהליך לחכות להודעה חדשה.
 * @param new_message_flag דגל שמציין אם יש הודעה חדשה.
 * @param thread_name שם התהליך (לצורכי הדפסה).
 */
void threadFunction(queue<int>& message_queue, condition_variable& cv, bool& new_message_flag, const string& thread_name) {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&new_message_flag] { return new_message_flag; }); // מחכה להודעה חדשה
        message_queue.push(number_to_send); // מוסיף את המספר לתור
        cout << "[" << thread_name << "] Received number: " << number_to_send << endl;
        new_message_flag = false; // מעדכן שאין הודעה חדשה
    }
}

/**
 * פונקציה לתהליך 3, המבצע בדיקות והשוואות בין המספרים בתורים.
 */
void threadFunction3() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(check_interval_seconds)); // מחכה 5 שניות

        unique_lock<mutex> lock(mtx);
        cout << "[Thread 3] Checking numbers..." << endl;
        cout << "[Thread 3] Thread 1 Queue Size: " << thread1_messages.size() << endl;
        cout << "[Thread 3] Thread 2 Queue Size: " << thread2_messages.size() << endl;

        // השוואה בין המספרים שנמצאים בתור של thread 1 ושל thread 2
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
 * פונקציה ראשית: יוצרת את התהליכים ומבצעת שליחת הודעות בתדירות קבועה.
 */
int main() {
    cout << "Starting program" << endl;

    // יצירת תהליכים
    thread t1(threadFunction, ref(thread1_messages), ref(cv_thread1), ref(new_message_thread1), "Thread 1"); // תהליך 1
    thread t2(threadFunction, ref(thread2_messages), ref(cv_thread2), ref(new_message_thread2), "Thread 2"); // תהליך 2
    thread t3(threadFunction3); // תהליך 3

    int option;

    while (true) {
        {
            option = rand() % 3; // נבחר מספר בין 0 ל-2
            cout << "[Main] option: " << option << endl;
            unique_lock<mutex> lock(mtx);

            if (option == 0) {
                // שליחת הודעה ל-thread 1
                number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                last_received_thread1 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread1 " << endl;
                new_message_thread1 = true; // מעדכן שיש הודעה חדשה
                cv_thread1.notify_one(); // מיידע את thread 1
            }
            else if (option == 1) {
                // שליחת הודעה ל-thread 2
                number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                last_received_thread2 = number_to_send;
                cout << "[Main] Sending number: " << number_to_send << " to thread2 " << endl;
                new_message_thread2 = true; // מעדכן שיש הודעה חדשה
                cv_thread2.notify_one(); // מיידע את thread 2
            }
            else if (option == 2) {
                // שליחת הודעה לשני התהליכים
                if (last_received_thread1 < last_received_thread2) {
                    number_to_send = generate_random_number(last_received_thread2 + 1, last_received_thread2 + 5);
                }
                else {
                    number_to_send = generate_random_number(last_received_thread1 + 1, last_received_thread1 + 5);
                }
                last_received_thread1 = number_to_send;
                last_received_thread2 = number_to_send;
                new_message_thread1 = true; // מעדכן שיש הודעה חדשה
                new_message_thread2 = true; // מעדכן שיש הודעה חדשה
                cv_thread1.notify_one();
                cv_thread2.notify_one(); // מיידע את thread 1 ו-thread 2

                cout << "[Main] Sending number: " << last_received_thread1 << " to both thread1 and thread2 " << endl;
            }
        }
        // השהייה לפני שליחת ההודעה הבאה
        this_thread::sleep_for(chrono::seconds(message_delay_seconds));
    }

    // צירוף התהליכים בעת סיום התוכנית
    t1.join();
    t2.join();
    t3.join();

    return 0; // סיום התוכנית
}
