#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>

using namespace std;

// משתנים גלובליים
mutex mtx; // מנעול לניהול גישה למידע משותף בין הת'רדים
condition_variable cv; // משתנה שמאפשר לת'רדים לחכות לתנאים מסוימים
queue<int> thread1_messages, thread2_messages; // תורים לאחסון המספרים המתקבלים מתהליך 1 ותהליך 2
bool new_message_thread1 = false, new_message_thread2 = false; // האם יש הודעה חדשה בתורים

// פרמטרים שונים
int message_delay_seconds = 1; // הזמן שיחכה כל תהליך לפני שליחה של הודעה חדשה
int check_interval_seconds = 5; // הזמן שיחכה תהליך 3 לפני בדיקה מחדש
int max_initial_random_number = 10; // הגבול העליון למספרים הרנדומליים הראשוניים
int max_increment = 10; // הגבול העליון להגדלת המספרים
int increment_value = 10; // ערך ההגדלה עבור כל מספר

// פונקציה ליצירת מספר רנדומלי
int generate_random_number(int previous_number, bool is_first) {
    random_device rd; // מכשיר ליצירת ערכים רנדומליים
    mt19937 gen(rd()); // גנרטור מספרים רנדומליים
    // התפלגות מספרים רנדומליים
    uniform_int_distribution<> dist(
        is_first ? 0 : previous_number + 1,
        is_first ? max_initial_random_number : previous_number + max_increment
    );
    return dist(gen); // מחזירה מספר רנדומלי
}

// פונקציה לתהליך 1
void threadFunction1() {
    int count = generate_random_number(0, true); // מתחיל עם מספר רנדומלי בין 0 ל-10
    while (true) {
        this_thread::sleep_for(chrono::seconds(message_delay_seconds)); // השהייה
        unique_lock<mutex> lock(mtx); // לוקח את המנעול
        if (rand() % 2 == 0) { // בוחן אם צריך לשלוח הודעה חדשה
            count = generate_random_number(count, false); // יוצר מספר רנדומלי חדש
            thread1_messages.push(count); // מוסיף את המספר לתור של תהליך 1
            new_message_thread1 = true; // מעדכן שיש הודעה חדשה בתור של תהליך 1
            cout << "[Thread 1] Received number: " << count << endl; // מדפיס את המספר
            count += increment_value; // מגדיל את המספר לצורך ההודעה הבאה
            cv.notify_all(); // מיידע את כל הת'רדים
        }
        lock.unlock(); // משחרר את המנעול
    }
}

// פונקציה לתהליך 2
void threadFunction2() {
    int count = generate_random_number(0, true); // מתחיל עם מספר רנדומלי בין 0 ל-10
    while (true) {
        this_thread::sleep_for(chrono::seconds(message_delay_seconds)); // השהייה
        unique_lock<mutex> lock(mtx); // לוקח את המנעול
        if (rand() % 2 == 0) { // בוחן אם צריך לשלוח הודעה חדשה
            count = generate_random_number(count, false); // יוצר מספר רנדומלי חדש
            thread2_messages.push(count); // מוסיף את המספר לתור של תהליך 2
            new_message_thread2 = true; // מעדכן שיש הודעה חדשה בתור של תהליך 2
            cout << "[Thread 2] Received number: " << count << endl; // מדפיס את המספר
            count += increment_value; // מגדיל את המספר לצורך ההודעה הבאה
            cv.notify_all(); // מיידע את כל הת'רדים
        }
        lock.unlock(); // משחרר את המנעול
    }
}

// פונקציה לתהליך 3
void threadFunction3() {
    while (true) {
        unique_lock<mutex> lock(mtx); // לוקח את המנעול
        cv.wait_for(lock, chrono::seconds(check_interval_seconds), [] {
            return new_message_thread1 || new_message_thread2;
            }); // מחכה לתנאים שיתקיימו או עד הזמן שהוקצב

        if (!new_message_thread1 && !new_message_thread2) { // אם אין הודעות חדשות
            continue; // חוזר להתחלה
        }

        cout << "[Thread 3] Checking numbers..." << endl; // מתחיל לבדוק מספרים

        // משווה בין המספרים בתורים של תהליך 1 ותהליך 2
        while (!thread1_messages.empty() && !thread2_messages.empty()) {
            int num1 = thread1_messages.front(); // מקבל את המספר הראשון
            int num2 = thread2_messages.front(); // מקבל את המספר השני

            cout << "[Thread 3] Comparing " << num1 << " (Thread 1) and " << num2 << " (Thread 2)" << endl; // מדפיס את המספרים להשוואה

            if (num1 == num2) { // אם המספרים תואמים
                cout << "[Thread 3] Found common number: " << num1 << endl; // מדפיס את המספר המשותף
                thread1_messages.pop(); // מסיר את המספר מהתור של תהליך 1
                thread2_messages.pop(); // מסיר את המספר מהתור של תהליך 2
            }
            else if (num1 < num2) {
                thread1_messages.pop(); // מסיר את המספר מהתור של תהליך 1
            }
            else {
                thread2_messages.pop(); // מסיר את המספר מהתור של תהליך 2
            }
        }

        new_message_thread1 = false; // מעדכן שאין הודעות חדשות בתור של תהליך 1
        new_message_thread2 = false; // מעדכן שאין הודעות חדשות בתור של תהליך 2
    }
}

// פונקציה ראשית
int main() {
    // יצירת ת'רדים
    thread t1(threadFunction1);
    thread t2(threadFunction2);
    thread t3(threadFunction3);

    // המתנה לסיום הת'רדים
    t1.join();
    t2.join();
    t3.join();

    return 0;
}
