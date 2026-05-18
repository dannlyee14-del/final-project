#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include "book.h"
#include <vector>
#include <ctime>

// 使用者帳號與借閱紀錄加分功能
class User {
public:
    string name;
    bool isGuest;
    bool isLocked;
    vector<string> borrowedBooks;
    vector<time_t> dueDates;
    User(string n, bool guest = false) : name(n), isGuest(guest), isLocked(false) {}
};

class Library {
public:
    Library();
    ~Library();
    char getKey();
    void operation(char opCode);
    void coutMainPage();
    void coutBookIcon(int bookNum);
    bool getExit();
    void searchBook();
    void addBook();
    void showLeaderboard();
protected:
    int idx;
    bool exit;
    vector<Book*> books;
    string statusMsg; // 確保有這一行，用來顯示邊界警告
};

#endif