#ifndef _LIBRARY_H_
#define _LIBRARY_H_
// ===== 跨平臺系統資源處理 =====
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#define CLEAR_CMD "cls"
#else
#include <unistd.h>
#define CLEAR_CMD "clear"
#endif
// =============================
#include "book.h"
#include <vector>
#include <ctime>
#include <string>

using namespace std;

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
    void login();
    void borrowBook(Book* b);
    void viewAccount();
protected:
    int idx;
    bool exit;
    vector<Book*> books;
    string statusMsg;
    User* currentUser;
    vector<string> globalBorrowedBooks;
};

#endif