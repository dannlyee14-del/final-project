#include "library.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#define NONE    "\e[0m"
#define WHITE_B "\e[47m"

Library::Library() {
    idx = 0; 
    exit = false; 
    statusMsg = "";
    currentUser = nullptr;
    
    ifstream in("./TXT/bookList.txt");
    string fn; char type;
    while (in >> fn >> type) {
        ifstream b("./TXT/" + fn); 
        if (!b.is_open()) continue;
        string s, t, a, c;
        while (getline(b, s)) {
            if (s.find("type:") != string::npos) { 
                c = s.substr(s.find(':')+1); 
                while(!c.empty() && c[0] == ' ') c.erase(0, 1);
                if(!c.empty() && c.back() == '\r') c.pop_back(); 
            }
            else if (s.find("Title:") != string::npos) { 
                t = s.substr(s.find(':')+1); 
                while(!t.empty() && t[0] == ' ') t.erase(0, 1);
                if(!t.empty() && t.back() == '\r') t.pop_back(); 
            }
            else if (s.find("Author:") != string::npos) { 
                a = s.substr(s.find(':')+1); 
                while(!a.empty() && a[0] == ' ') a.erase(0, 1);
                if(!a.empty() && a.back() == '\r') a.pop_back(); 
                break; 
            }
        }
        if (type == 'T') books.push_back(new TxtBook(fn, t, a, c));
        else if (type == 'F') books.push_back(new FigBook(fn, t, a, c));
        else if (type == 'M') books.push_back(new MthBook(fn, t, a, c));
        else if (type == 'A') books.push_back(new AniBook(fn, t, a, c));
        else if (type == 'C') books.push_back(new MorseBook(fn, t, a, c));
    }
    login();
}

Library::~Library() { 
    for (auto b : books) delete b; 
    if (currentUser) delete currentUser;
}

char Library::getKey() {
    char key; 
    system("stty raw -echo"); 
    key = getchar();
    if (key == '\x1b') {
        char next_char = getchar();
        if (next_char == '\x5b' || next_char == 'O') {
            char d = getchar(); 
            system("stty cooked echo");
            if (d == 'A') return 'U';
            if (d == 'B') return 'D';
            if (d == 'C') return 'R';
            if (d == 'D') return 'L';
            return ' ';
        }
    } else if (key == '\x0d' || key == '\x0a') { 
        system("stty cooked echo"); 
        return 'E';
    } else if (toupper(key) == 'A') { 
        system("stty cooked echo"); 
        return 'A';
    } else if (toupper(key) == 'M') { 
        system("stty cooked echo"); 
        return 'M';
    } else if (toupper(key) == 'Q') { 
        system("stty cooked echo"); 
        return 'Q';
    }
    system("stty cooked echo"); 
    return ' ';
}
void Library::operation(char op) {
    int total = books.size(); 
    int quit_idx = total + 1;
    statusMsg = ""; 

    if (op == 'Q') {
        exit = true;
        return;
    }

    if (op == 'A') addBook();
    else if (op == 'M') viewAccount();
    else if (op == 'E') {
        if (idx == 0) searchBook();
        else if (idx == quit_idx) exit = true;
        else { 
            system(CLEAR_CMD);
            cout << "=== " << books[idx-1]->getTitle() << " ===" << endl;
            cout << "1. Preview Book" << endl;
            cout << "2. Borrow Book" << endl;
            cout << "Choice: ";
            int choice;
            if (cin >> choice) {
                cin.ignore(1000, '\n');
                if (choice == 1) {
                    books[idx-1]->preview(); 
                    books[idx-1]->addBorrowCount(); 
                } else if (choice == 2) {
                    borrowBook(books[idx-1]);
                }
            } else {
                cin.clear();
                cin.ignore(1000, '\n');
            }
        }
    } else if (op == 'U') {
        if (idx == 0) {
            statusMsg = "This is the top of the page, you cannot move up.";
        } else if (idx <= 5) {
            idx = 0; 
        } else {
            idx -= 5;
        }
    } else if (op == 'D') {
        if (idx == quit_idx) {
            statusMsg = "This is the bottom of the page, you cannot move down.";
        } else if (idx == 0) {
            idx = 1; 
        } else if (idx > total - 5) {
            idx = quit_idx;
        } else {
            idx += 5;
        }
    } else if (op == 'L' && idx > 0) {
        idx--;
    } else if (op == 'R' && idx < quit_idx) {
        idx++;
    }
}

void Library::coutMainPage() {
    system(CLEAR_CMD);
    for(int i = 0; i < 100; i++) cout << "=";
    cout << "\n\n" << setw(66) << "Welcome to NYCU library system !!!\n\n";
    
    if (idx == 0) cout << WHITE_B;
    cout << " Search";
    for(int i = 0; i < 93; i++) cout << " ";
    cout << NONE << "\n\n Books\n";

    for (int i = 0; i < (int)books.size(); i += 5) {
        coutBookIcon(i);
    }

    if (idx == (int)books.size() + 1) cout << WHITE_B;
    cout << " Quit";
    for(int i = 0; i < 95; i++) cout << " ";
    cout << NONE << "\n\n";
    
    if (!statusMsg.empty()) {
        cout << "\e[1;31m !! Warning: " << statusMsg << " !! \e[0m" << endl << endl;
    }

    cout << " Guide" << endl;
    cout << "  > move up    : [↑]" << endl;
    cout << "  > move down  : [↓]" << endl;
    cout << "  > move left  : [←]" << endl;
    cout << "  > move right : [→]" << endl;
    cout << "  > confirm    : [↲]" << endl;
    cout << "  > add book   : [A]" << endl;
    cout << "  > my account : [M]" << endl;
    cout << "  > exit system: [Q]" << endl;
    cout << endl;
    for(int i = 0; i < 100; i++) cout << "=";
    cout << endl;
}

void Library::coutBookIcon(int b) {
    for (int j = 0; j < 7; j++) {
        cout << "     ";
        for (int i = 0; i < 5; i++) {
            if (b + i < (int)books.size()) {
                string t = books[b+i]->getTitle();
                if (b + i == idx - 1) cout << WHITE_B;
                if (j == 0 || j == 4) cout << "###############";
                else if (j == 1 || j == 3) cout << "#             #";
                else if (j == 2) cout << "#      " << t[0] << "      #";
                else if (j == 5) cout << setw(15) << left << t.substr(0, 15);
                else if (j == 6) cout << setw(15) << left << (t.size() > 15 ? t.substr(15, 15) : "");
                cout << NONE;
            } else {
                cout << "               ";
            }
            cout << "    ";
        }
        cout << endl;
    }
}

void Library::searchBook() {
    system(CLEAR_CMD);
    cout << "********** Search book **********" << endl;
    cout << "Index by 1.Filename 2.Title 3.Author 4.Category 5.Content 6.Leaderboard" << endl;
    cout << "Choice: ";
    
    int choice;
    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(1000, '\n');
        return;
    }

    if (choice == 6) {
        vector<Book*> rankList = books;
        sort(rankList.begin(), rankList.end(), [](Book* a, Book* b) {
            return a->getBorrowCount() > b->getBorrowCount();
        });

        cout << "\n--- Popularity Leaderboard ---" << endl;
        cout << left << setw(5) << "Rank" << setw(30) << "Title" << "Borrows" << endl;
        cout << string(45, '-') << endl;
        for (int i = 0; i < (int)rankList.size(); i++) {
            cout << left << setw(5) << i + 1 
                 << setw(30) << rankList[i]->getTitle() 
                 << rankList[i]->getBorrowCount() << endl;
        }
        cout << "\nPress Enter to return...";
        cin.ignore(1000, '\n');
        getchar();
        return; 
    }

    string query;
    cout << "Keyword: ";
    cin >> query;

    cout << "\n--- Search Results ---" << endl;
    int foundCount = 0;

    for (auto b : books) {
        bool isMatch = false;
        
        if (choice == 1 && b->getFilename().find(query) != string::npos) isMatch = true;
        else if (choice == 2 && b->getTitle().find(query) != string::npos) isMatch = true;
        else if (choice == 3 && b->getAuthor().find(query) != string::npos) isMatch = true;
        else if (choice == 4 && b->getCategory().find(query) != string::npos) isMatch = true;
        else if (choice == 5 && b->searchContent(query)) isMatch = true;

        if (isMatch) {
            foundCount++;
            cout << foundCount << ". " << b->getTitle() 
                << " [" << b->getCategory() << "] by " << b->getAuthor() 
                << " (Borrows: " << b->getBorrowCount() << ")" << endl;
        }
    }

    if (foundCount == 0) {
        cout << "No books found matching your keyword." << endl;
    } else {
        cout << "\nTotal " << foundCount << " book(s) found." << endl;
    }

    cout << "\nPress Enter to return...";
    cin.ignore(1000, '\n');
    getchar();
}

void Library::addBook() {
    system(CLEAR_CMD); 
    string fn, ts, t, a; 
    cout << "Filename: "; cin >> fn; 
    cout << "Type (T/F/M/A/C): "; cin >> ts;
    cin.ignore(256, '\n');
    ifstream f("./TXT/" + fn);
    if (!f.is_open()) {
        cout << "Auto-creating...\nTitle: "; getline(cin, t); 
        cout << "Author: "; getline(cin, a);
        ofstream o("./TXT/" + fn); 
        o << "type: new\nTitle: " << t << "\nAuthor: " << a << "\nContent here.\n";
    }
    ofstream l("./TXT/bookList.txt", ios::app); 
    l << "\n" << fn << " " << (char)toupper(ts[0]);
    cout << "Done. Restart to apply.\nPress Enter..."; 
    getchar();
}

bool Library::getExit() { return exit; }

void Library::showLeaderboard() {
    system(CLEAR_CMD);
    cout << "********** Popular Books Ranking **********" << endl;

    vector<Book*> sortedBooks = books;
    sort(sortedBooks.begin(), sortedBooks.end(), [](Book* a, Book* b) {
        return a->getBorrowCount() > b->getBorrowCount();
    });

    for (int i = 0; i < (int)sortedBooks.size(); i++) {
        cout << i + 1 << ". " << setw(20) << left << sortedBooks[i]->getTitle() 
             << " | Borrows: " << sortedBooks[i]->getBorrowCount() << endl;
    }

    cout << "\nPress Enter to return...";
    cin.ignore(1000, '\n');
    getchar();
}

void Library::login() {
    system(CLEAR_CMD);
    cout << "===========================================" << endl;
    cout << "               User Login System           " << endl;
    cout << "===========================================" << endl;
    cout << "Enter your name: ";
    string name;
    getline(cin, name);
    
    bool isGuest = false;
    if (name.empty() || name == "Guest") {
        name = "Guest";
        isGuest = true;
    }
    
    currentUser = new User(name, isGuest); 
    cout << "\nWelcome, " << currentUser->name << "! Press Enter to enter library...";
    getchar();
}

void Library::borrowBook(Book* b) {
    system(CLEAR_CMD);
    cout << "========== Borrow Book ==========" << endl;
    
    if (currentUser->isGuest) {
        cout << "\e[1;31mWarning: Guest accounts cannot borrow books!\e[0m" << endl;
        cout << "\nPress Enter to return...";
        getchar();
        return;
    }
    
    for (const string& title : globalBorrowedBooks) {
        if (title == b->getTitle()) {
            cout << "\e[1;31mWarning: This book is already borrowed!\e[0m" << endl;
            cout << "\nPress Enter to return...";
            getchar();
            return;
        }
    }
    
    currentUser->borrowedBooks.push_back(b->getTitle());
    globalBorrowedBooks.push_back(b->getTitle());
    
    time_t dueDate = time(0) + 14 * 24 * 60 * 60;
    currentUser->dueDates.push_back(dueDate);
    
    b->addBorrowCount(); 
    
    cout << "Successfully borrowed \"" << b->getTitle() << "\"!" << endl;
    
    char buf[80];
    struct tm* timeinfo = localtime(&dueDate);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    cout << "Due Date: " << buf << endl;
    
    cout << "\nPress Enter to return...";
    getchar();
}

void Library::viewAccount() {
    while (true) {
        system(CLEAR_CMD);
        cout << "========== My Account ==========" << endl;
        cout << "User Name: " << currentUser->name << endl;
        cout << "-------------------------------------------" << endl;
        cout << "Borrowed Books List:" << endl;
        
        if (currentUser->borrowedBooks.empty()) {
            cout << "No borrowed books." << endl;
        } else {
            for (int i = 0; i < (int)currentUser->borrowedBooks.size(); i++) {
                time_t dueDate = currentUser->dueDates[i];
                char buf[80];
                struct tm* timeinfo = localtime(&dueDate);
                strftime(buf, sizeof(buf), "%Y-%m-%d", timeinfo);
                cout << i + 1 << ". " << setw(30) << left << currentUser->borrowedBooks[i] 
                     << " | Due Date: " << buf << endl;
            }
        }
        
        cout << "-------------------------------------------" << endl;
        cout << "Options: 1. Return Book  2. Logout  3. Back to Menu" << endl;
        cout << "Choice: ";
        int choice;
        if (cin >> choice) {
            cin.ignore(1000, '\n');
            if (choice == 1) {
                if (currentUser->borrowedBooks.empty()) {
                    cout << "You have no books to return." << endl;
                    cout << "\nPress Enter to continue...";
                    getchar();
                    continue;
                }
                cout << "Enter the number of the book to return: ";
                int num;
                if (cin >> num) {
                    cin.ignore(1000, '\n');
                    if (num > 0 && num <= (int)currentUser->borrowedBooks.size()) {
                        string retTitle = currentUser->borrowedBooks[num - 1];
                        currentUser->borrowedBooks.erase(currentUser->borrowedBooks.begin() + (num - 1));
                        currentUser->dueDates.erase(currentUser->dueDates.begin() + (num - 1));
                        
                        auto it = find(globalBorrowedBooks.begin(), globalBorrowedBooks.end(), retTitle);
                        if (it != globalBorrowedBooks.end()) {
                            globalBorrowedBooks.erase(it);
                        }
                        
                        cout << "Successfully returned \"" << retTitle << "\"!" << endl;
                        cout << "\nPress Enter to continue...";
                        getchar();
                    } else {
                        cout << "Invalid number!" << endl;
                        cout << "\nPress Enter to continue...";
                        getchar();
                    }
                }
            } else if (choice == 2) {
                delete currentUser;
                login();
                break;
            } else if (choice == 3) {
                break;
            }
        } else {
            cin.clear();
            cin.ignore(1000, '\n');
        }
    }
}