#ifndef _LIBRARY_H_
#define _LIBRARY_H_
#include "book.h"
#include <vector>
#include <ctime>
#include <string>
#include <map>

using namespace std;

class User {
public:
    string name;
    int age;
    bool isGuest;
    bool isLocked;
    vector<string> borrowedBooks;
    vector<time_t> dueDates;
    vector<string> favoriteBooks;
    vector<string> readingHistory;
    vector<string> returnedBooks;
    int readingPoints;
    time_t banUntil;
    User(string n, int userAge, bool guest = false)
        : name(n), age(userAge), isGuest(guest), isLocked(false),
          readingPoints(0), banUntil(0) {}
};

class Library {
public:
    Library();
    ~Library();
    char getKey();
    void operation(char opCode);
    void coutMainPage();
    void coutBookIcon(int bookNum);
    void coutMainPageLegacy();
    void coutBookIconLegacy(int bookNum);
    bool getExit();
    void searchBook();
    void addBook();
    void showLeaderboard();
    void login();
    void borrowBook(Book* b);
    void viewAccount();
    void recommendBooks();
    void toggleFavorite(Book* b);
    void recordPreview(Book* b);
    string getReaderTitle() const;
    void chooseLanguage();
    bool readLineOrHome(string& value);
    bool readNumberOrHome(int& value);
    bool waitForHome();
    int chooseMenu(const string& title, const vector<string>& options);
    void rateBook(Book* b);
    void commentBook(Book* b);
    void showBookReviews(Book* b);
    void loadReviews();
    void saveReviewRecord(const string& filename, char kind, const string& value);
    void loadUserStates();
    void saveUserState(User* user);
    void loadBorrowRecords();
    void saveBorrowRecords();
    void saveReturnRecord(const string& filename, const string& userName,
                          time_t dueDate, int overdueMinutes, int banMinutes);
    int calculateOverdueMinutes(time_t dueDate) const;
    bool isUserBanned() const;
    string formatDate(time_t value) const;
    string formatClock(time_t value) const;
    vector<Book*> getVisibleBooks() const;
    bool isBorrowedByOtherUser(Book* b) const;
protected:
    int idx;
    bool exit;
    vector<Book*> books;
    string statusMsg;
    User* currentUser;
    map<string, User*> users;
    vector<string> globalBorrowedBooks;
    map<string, string> borrowedByUser;
    bool chineseInterface;
};

#endif
