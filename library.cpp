#include "library.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

#define NONE        "\033[0m"
#define WHITE_B     "\033[47m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define CYAN        "\033[36m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define YELLOW      "\033[33m"
#define GREEN       "\033[32m"
#define RED         "\033[31m"
#define SELECTED    "\033[1;30;46m"

namespace {
const int SCREEN_WIDTH = 100;

void printThemeLine(char fill = '=') {
    cout << CYAN << string(SCREEN_WIDTH, fill) << NONE << endl;
}

void printThemeCentered(const string& text, const char* color = BOLD) {
    int padding = max(0, (SCREEN_WIDTH - static_cast<int>(text.size())) / 2);
    cout << color << string(padding, ' ') << text << NONE << endl;
}

bool isCjkCodePoint(unsigned int cp) {
    return (cp >= 0x2E80 && cp <= 0x9FFF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFF00 && cp <= 0xFFEF);
}

bool readUtf8CodePoint(const string& text, size_t& pos, string& bytes, unsigned int& cp) {
    if (pos >= text.size()) return false;
    unsigned char first = static_cast<unsigned char>(text[pos]);
    size_t len = 1;
    cp = first;
    if ((first & 0xE0) == 0xC0) {
        len = 2;
        cp = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        len = 3;
        cp = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        len = 4;
        cp = first & 0x07;
    }
    if (pos + len > text.size()) len = 1;
    bytes = text.substr(pos, len);
    if (len > 1) {
        for (size_t i = 1; i < len; i++) {
            unsigned char ch = static_cast<unsigned char>(text[pos + i]);
            if ((ch & 0xC0) != 0x80) {
                len = 1;
                bytes = text.substr(pos, len);
                cp = first;
                break;
            }
            cp = (cp << 6) | (ch & 0x3F);
        }
    }
    pos += len;
    return true;
}

string fitCell(const string& text, int width) {
    string result;
    int used = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        string bytes;
        unsigned int cp = 0;
        size_t before = pos;
        if (!readUtf8CodePoint(text, pos, bytes, cp)) break;
        int charWidth = isCjkCodePoint(cp) ? 2 : 1;
        if (used + charWidth > width) {
            pos = before;
            break;
        }
        result += bytes;
        used += charWidth;
    }
    if (used < width) result += string(width - used, ' ');
    return result;
}

string displayUserName(User* user, bool chineseInterface) {
    if (user == nullptr) return "";
    if (user->isGuest) return chineseInterface ? "訪客" : "Guest";
    return user->name;
}

}

Library::Library() {
    idx = 0; 
    exit = false; 
    statusMsg = "";
    currentUser = nullptr;
    chineseInterface = true;
    
    ifstream in("./Database/bookList.txt");
    string fn; char type;
    while (in >> fn >> type) {
        ifstream b("./Bookshelf/" + fn); 
        if (!b.is_open()) continue;
        string s, t, a, c;
        bool adultOnly = false;
        while (getline(b, s)) {
            if (s.find("type:") != string::npos) { 
                c = s.substr(s.find(':')+1); 
                while(!c.empty() && c[0] == ' ') c.erase(0, 1);
                if(!c.empty() && c.back() == '\r') c.pop_back(); 
            }
            else if (s.find("Age:") != string::npos || s.find("age:") != string::npos) {
                string ageLimit = s.substr(s.find(':') + 1);
                while(!ageLimit.empty() && ageLimit[0] == ' ') ageLimit.erase(0, 1);
                transform(ageLimit.begin(), ageLimit.end(), ageLimit.begin(),
                          [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
                adultOnly = ageLimit.find("18") != string::npos ||
                            ageLimit.find("adult") != string::npos;
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
            }
        }
        Book* newBook = nullptr;
        if (type == 'T') newBook = new TxtBook(fn, t, a, c);
        else if (type == 'F') newBook = new FigBook(fn, t, a, c);
        else if (type == 'M') newBook = new MthBook(fn, t, a, c);
        else if (type == 'A') newBook = new AniBook(fn, t, a, c);
        else if (type == 'C') newBook = new MorseBook(fn, t, a, c);
        if (newBook != nullptr) {
            newBook->setAdultOnly(adultOnly);
            books.push_back(newBook);
        }
    }
    for (Book* book : books) book->setChinese(chineseInterface);
    loadReviews();
    loadUserStates();
    loadBorrowRecords();
    login();
}

Library::~Library() { 
    for (auto b : books) delete b; 
    for (auto& pair : users) delete pair.second;
}

bool Library::readLineOrHome(string& value) {
    value.clear();
    system("stty raw -echo");

    while (true) {
        char key = getchar();
        if (key == '\x1b') {
            char second = getchar();
            if (second == '[' || second == 'O') {
                char third = getchar();
                if (third == 'F') {
                    system("stty cooked echo");
                    cout << endl;
                    return false;
                }
            }
        } else if (key == '\r' || key == '\n') {
            system("stty cooked echo");
            cout << endl;
            return true;
        } else if (key == 127 || key == '\b') {
            if (!value.empty()) {
                value.pop_back();
                cout << "\b \b" << flush;
            }
        } else if (isprint(static_cast<unsigned char>(key)) || key < 0) {
            value += key;
            cout << key << flush;
        }
    }
}

bool Library::readNumberOrHome(int& value) {
    string input;
    if (!readLineOrHome(input)) return false;
    try {
        size_t used = 0;
        int parsed = stoi(input, &used);
        if (used != input.size()) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool Library::waitForHome() {
    cout << (chineseInterface ? "\n[End] 返回主頁，或按 Enter 繼續" :
                               "\n[End] Back to Home, or press Enter to continue") << flush;
    string ignored;
    return readLineOrHome(ignored);
}

string normalizeTxtFilename(string filename) {
    const string extension = ".txt";
    if (filename.size() >= extension.size() &&
        filename.substr(filename.size() - extension.size()) == extension) {
        return filename;
    }
    return filename + extension;
}

void ensureUserDataDirectory() {
    mkdir("./UserData", 0755);
}

int Library::chooseMenu(const string& title, const vector<string>& options) {
    if (options.empty()) return 0;
    int selected = 0;

    while (true) {
        system("clear");
        cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
             << NONE << "\n\n";
        cout << title << "\n" << string(45, '-') << endl;
        for (int i = 0; i < (int)options.size(); i++) {
            if (i == selected) cout << SELECTED;
            cout << " " << i + 1 << ". " << options[i] << " ";
            cout << NONE << endl;
        }
        cout << (chineseInterface ? "\n[↑/↓] 移動  [Enter] 確認" :
                                    "\n[↑/↓] Move  [Enter] Confirm") << endl;

        system("stty raw -echo");
        char key = getchar();
        if (key == '\x1b') {
            char second = getchar();
            if (second == '[' || second == 'O') {
                char third = getchar();
                system("stty cooked echo");
                if (third == 'A' && selected > 0) selected--;
                else if (third == 'B' && selected < (int)options.size() - 1) selected++;
                else if (third == 'F') return 0;
            } else {
                system("stty cooked echo");
            }
        } else if (key == '\r' || key == '\n') {
            system("stty cooked echo");
            return selected + 1;
        } else if (isdigit(static_cast<unsigned char>(key))) {
            system("stty cooked echo");
            int value = key - '0';
            if (value >= 1 && value <= (int)options.size()) return value;
        } else {
            system("stty cooked echo");
        }
    }
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
            if (d == 'C') return '>';
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
    } else if (toupper(key) == 'R') {
        system("stty cooked echo");
        return 'R';
    } else if (toupper(key) == 'Q') { 
        system("stty cooked echo"); 
        return 'Q';
    } else if (key == '~') {
        system("stty cooked echo");
        return '~';
    }
    system("stty cooked echo"); 
    return ' ';
}

void Library::operation(char op) {
    vector<Book*> visibleBooks = getVisibleBooks();
    int total = visibleBooks.size();
    int quit_idx = total + 1;
    statusMsg = ""; 

    if (op == 'Q') {
        exit = true;
        return;
    }
    if (op == '~') {
        if (currentUser != nullptr && !currentUser->isGuest) {
            currentUser->banUntil = 0;
            saveUserState(currentUser);
            statusMsg = chineseInterface ? "停權已解除。" : "Ban cleared.";
        }
        return;
    }

    if (op == 'A') addBook();
    else if (op == 'M') viewAccount();
    else if (op == 'R') recommendBooks();
    else if (op == 'E') {
        if (idx == 0) searchBook();
        else if (idx == quit_idx) exit = true;
        else { 
            Book* selectedBook = visibleBooks[idx - 1];
            vector<string> options = chineseInterface ?
                vector<string>{"預覽書籍", "借閱書籍", "加入／移除收藏", "書籍評分",
                               "撰寫評論", "查看評分與評論"} :
                vector<string>{"Preview Book", "Borrow Book", "Add / Remove Favorite",
                               "Rate Book", "Write Comment", "View Ratings and Comments"};
            int choice = chooseMenu("=== " + selectedBook->getTitle() + " ===", options);
            if (choice == 1) {
                selectedBook->preview();
                recordPreview(selectedBook);
            } else if (choice == 2) {
                borrowBook(selectedBook);
            } else if (choice == 3) {
                toggleFavorite(selectedBook);
            } else if (choice == 4) {
                rateBook(selectedBook);
            } else if (choice == 5) {
                commentBook(selectedBook);
            } else if (choice == 6) {
                showBookReviews(selectedBook);
            }
        }
    } else if (op == 'U') {
        if (idx == 0) {
            statusMsg = chineseInterface ? "已到達頁面頂端。" :
                                           "This is the top of the page, you cannot move up.";
        } else if (idx <= 5) {
            idx = 0; 
        } else {
            idx -= 5;
        }
    } else if (op == 'D') {
        if (idx == quit_idx) {
            statusMsg = chineseInterface ? "已到達頁面底端。" :
                                           "This is the bottom of the page, you cannot move down.";
        } else if (idx == 0) {
            idx = 1; 
        } else if (idx > total - 5) {
            idx = quit_idx;
        } else {
            idx += 5;
        }
    } else if (op == 'L' && idx > 0) {
        idx--;
    } else if (op == '>' && idx < quit_idx) {
        idx++;
    }
}

void Library::coutMainPageLegacy() {
    system("clear");
    for(int i = 0; i < 100; i++) cout << "=";
    cout << "\n\n" << setw(66)
         << (chineseInterface ? "歡迎使用 NYCU 圖書館系統！\n\n" :
                                "Welcome to NYCU library system !!!\n\n");
    
    if (idx == 0) cout << WHITE_B;
    cout << (chineseInterface ? " 搜尋" : " Search");
    for(int i = 0; i < 93; i++) cout << " ";
    cout << NONE << (chineseInterface ? "\n\n 書籍\n" : "\n\n Books\n");

    for (int i = 0; i < (int)books.size(); i += 5) {
        coutBookIcon(i);
    }

    if (idx == (int)books.size() + 1) cout << WHITE_B;
    cout << (chineseInterface ? " 離開" : " Quit");
    for(int i = 0; i < 95; i++) cout << " ";
    cout << NONE << "\n\n";
    
    if (!statusMsg.empty()) {
        cout << "\e[1;31m !! " << (chineseInterface ? "提醒：" : "Warning: ")
             << statusMsg << " !! \e[0m" << endl << endl;
    }

    if (chineseInterface) {
        cout << " 操作指南" << endl;
        cout << "  > 向上移動：[↑]" << endl;
        cout << "  > 向下移動：[↓]" << endl;
        cout << "  > 向左移動：[←]" << endl;
        cout << "  > 向右移動：[→]" << endl;
        cout << "  > 確認選擇：[Enter]" << endl;
        cout << "  > 新增書籍：[A]" << endl;
        cout << "  > 我的帳號／語言：[M]" << endl;
        cout << "  > 閱讀羅盤：[R]" << endl;
        cout << "  > 離開系統：[Q]" << endl;
    } else {
        cout << " Guide" << endl;
        cout << "  > move up    : [↑]" << endl;
        cout << "  > move down  : [↓]" << endl;
        cout << "  > move left  : [←]" << endl;
        cout << "  > move right : [→]" << endl;
        cout << "  > confirm    : [Enter]" << endl;
        cout << "  > add book   : [A]" << endl;
        cout << "  > my account/language: [M]" << endl;
        cout << "  > reading compass: [R]" << endl;
        cout << "  > exit system: [Q]" << endl;
    }
    cout << endl;
    for(int i = 0; i < 100; i++) cout << "=";
    cout << endl;
}

void Library::coutBookIconLegacy(int b) {
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

void Library::coutMainPage() {
    system("clear");
    vector<Book*> visibleBooks = getVisibleBooks();
    if (idx > (int)visibleBooks.size() + 1) idx = visibleBooks.size() + 1;
    printThemeLine('=');
    printThemeCentered(chineseInterface ? "NYCU 數位圖書館" : "NYCU DIGITAL LIBRARY", BOLD CYAN);
    printThemeCentered(chineseInterface ? "智慧閱讀，探索無限" : "Smart Reading, Infinite Discovery", DIM);
    printThemeLine('-');

    cout << "  " << GREEN << (chineseInterface ? "使用者" : "USER") << NONE << "  " << displayUserName(currentUser, chineseInterface)
         << "    " << CYAN << (chineseInterface ? "年齡" : "AGE") << NONE << "  " << currentUser->age
         << "    " << YELLOW << (chineseInterface ? "積分" : "POINTS") << NONE << "  " << currentUser->readingPoints
         << "    " << RED << (chineseInterface ? "停權" : "BAN") << NONE << "  "
         << (isUserBanned() ? (chineseInterface ? "封禁到 " : "Until ") + formatClock(currentUser->banUntil)
                            : (chineseInterface ? "無" : "None"))
         << "    " << MAGENTA << (chineseInterface ? "稱號" : "LEVEL") << NONE << "  " << getReaderTitle() << endl;
    printThemeLine('-');

    cout << "  ";
    if (idx == 0) cout << SELECTED;
    cout << "  [ / ] " << (chineseInterface ? "搜尋書籍" : "SEARCH BOOKS")
         << string(78, ' ') << NONE << "\n\n";

    cout << "  " << BOLD BLUE << (chineseInterface ? "可借閱書籍" : "AVAILABLE BOOKS") << NONE
         << "  " << DIM << visibleBooks.size() << (chineseInterface ? " 本" : " titles") << NONE;
    cout << "\n\n";

    for (int i = 0; i < (int)visibleBooks.size(); i += 5) {
        coutBookIcon(i);
    }

    cout << "\n  ";
    if (idx == (int)visibleBooks.size() + 1) cout << SELECTED;
    cout << "  [ X ] " << (chineseInterface ? "離開圖書館" : "EXIT LIBRARY")
         << string(77, ' ') << NONE << "\n\n";

    if (!statusMsg.empty()) {
        cout << "  " << RED << BOLD << "[!] " << (chineseInterface ? "提醒  " : "NOTICE  ")
             << statusMsg << NONE << "\n\n";
    }

    printThemeLine('-');
    if (chineseInterface) {
        cout << "  " << BOLD << "快捷鍵" << NONE
             << "   [方向鍵] 移動   [Enter] 選擇   "
             << CYAN << "[A]" << NONE << " 新增書籍   "
             << MAGENTA << "[M]" << NONE << " 帳號/語言   "
             << YELLOW << "[R]" << NONE << " 推薦書籍   "
             << RED << "[Q]" << NONE << " 離開" << endl;
    } else {
        cout << "  " << BOLD << "QUICK KEYS" << NONE
             << "   [Arrows] Navigate   [Enter] Select   "
             << CYAN << "[A]" << NONE << " Add   "
             << MAGENTA << "[M]" << NONE << " Account/Language   "
             << YELLOW << "[R]" << NONE << " Compass   "
             << RED << "[Q]" << NONE << " Exit" << endl;
    }
    printThemeLine('=');
}

void Library::coutBookIcon(int b) {
    vector<Book*> visibleBooks = getVisibleBooks();
    const int cardWidth = 15;
    for (int j = 0; j < 7; j++) {
        cout << "     ";
        for (int i = 0; i < 5; i++) {
            int bookIndex = b + i;
            if (bookIndex < (int)visibleBooks.size()) {
                Book* book = visibleBooks[bookIndex];
                string t = book->getTitle();
                bool selected = bookIndex == idx - 1;
                if (selected) cout << SELECTED;
                else cout << BLUE;

                if (j == 0 || j == 4) cout << "###############";
                else if (j == 1 || j == 3) cout << "#             #";
                else if (j == 2) {
                    char initial = t.empty() ? '?' : t[0];
                    cout << "#      " << initial << "      #";
                } else if (j == 5) {
                    cout << fitCell(t, cardWidth);
                } else if (j == 6) {
                    string rating = (chineseInterface ? " 評:" : " R:") +
                                    to_string(book->getAverageRating()).substr(0, 3);
                    if (book->isAdultOnly()) {
                        cout << RED << fitCell("18+", 3);
                        cout << (selected ? SELECTED : BLUE) << fitCell(rating, cardWidth - 3);
                    } else {
                        string meta = (chineseInterface ? "全年齡" : "All") + rating;
                        cout << fitCell(meta, cardWidth);
                    }
                }
                cout << NONE;
            } else {
                cout << string(cardWidth, ' ');
            }
            cout << "    ";
        }
        cout << endl;
    }
}

void Library::searchBook() {
    vector<string> options = chineseInterface ?
        vector<string>{"檔名", "書名", "作者", "分類", "內容", "排行榜"} :
        vector<string>{"Filename", "Title", "Author", "Category", "Content", "Leaderboard"};
    int choice = chooseMenu(chineseInterface ? "********** 搜尋書籍 **********" :
                                              "********** Search Books **********", options);
    if (choice == 0) return;

    if (choice == 6) {
        vector<Book*> rankList = books;
        sort(rankList.begin(), rankList.end(), [](Book* a, Book* b) {
            return a->getBorrowCount() > b->getBorrowCount();
        });

        cout << (chineseInterface ? "\n--- 熱門排行榜 ---\n" :
                                   "\n--- Popularity Leaderboard ---\n");
        cout << left << setw(5) << (chineseInterface ? "排名" : "Rank")
             << setw(30) << (chineseInterface ? "書名" : "Title")
             << (chineseInterface ? "借閱次數" : "Borrows") << endl;
        cout << string(45, '-') << endl;
        for (int i = 0; i < (int)rankList.size(); i++) {
            cout << left << setw(5) << i + 1 
                 << setw(30) << rankList[i]->getTitle() 
                 << rankList[i]->getBorrowCount() << endl;
        }
        cout << (chineseInterface ? "\n按 Enter 返回..." : "\nPress Enter to return...");
        waitForHome();
        return; 
    }

    string query;
    cout << (chineseInterface ? "關鍵字：" : "Keyword: ");
    if (!readLineOrHome(query)) return;

    cout << (chineseInterface ? "\n--- 搜尋結果 ---\n" : "\n--- Search Results ---\n");
    int foundCount = 0;

    for (auto b : getVisibleBooks()) {
        bool isMatch = false;
        
        if (choice == 1 && b->getFilename().find(query) != string::npos) isMatch = true;
        else if (choice == 2 && b->getTitle().find(query) != string::npos) isMatch = true;
        else if (choice == 3 && b->getAuthor().find(query) != string::npos) isMatch = true;
        else if (choice == 4 && b->getCategory().find(query) != string::npos) isMatch = true;
        else if (choice == 5 && b->searchContent(query)) isMatch = true;

        if (isMatch) {
            foundCount++;
            cout << foundCount << ". " << b->getTitle() 
                << " [" << b->getCategory() << "] "
                << (chineseInterface ? "作者：" : "by ") << b->getAuthor()
                << " (" << (chineseInterface ? "借閱：" : "Borrows: ")
                << b->getBorrowCount() << ")" << endl;
        }
    }

    if (foundCount == 0) {
        cout << (chineseInterface ? "找不到符合關鍵字的書籍。" :
                                   "No books found matching your keyword.") << endl;
    } else {
        if (chineseInterface) cout << "\n共找到 " << foundCount << " 本書。" << endl;
        else cout << "\nTotal " << foundCount << " book(s) found." << endl;
    }

    cout << (chineseInterface ? "\n按 Enter 返回..." : "\nPress Enter to return...");
    waitForHome();
}

void Library::addBook() {
    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    if (currentUser->isGuest) {
        cout << (chineseInterface ?
            "\e[1;31m提醒：訪客帳號無法新增書籍！\e[0m" :
            "\e[1;31mWarning: Guest accounts cannot add books!\e[0m") << endl;
        waitForHome();
        return;
    }

    string fn, ts, t, a;
    int ageChoice = 1;
    cout << (chineseInterface ? "檔案名稱（End 返回主頁）：" :
                               "Filename ([End] Home): ");
    if (!readLineOrHome(fn)) return;
    fn = normalizeTxtFilename(fn);
    vector<string> typeOptions = chineseInterface ?
        vector<string>{"一般文字書", "ASCII 圖像書", "數學互動書", "動畫書", "摩斯密碼書"} :
        vector<string>{"Text Book", "ASCII Art Book", "Math Interactive Book",
                       "Animation Book", "Morse Code Book"};
    int typeChoice = chooseMenu(chineseInterface ? "書籍類型" : "Book Type", typeOptions);
    if (typeChoice == 0) return;
    const string typeCodes = "TFMAC";
    ts = string(1, typeCodes[typeChoice - 1]);
    vector<string> ageOptions = chineseInterface ?
        vector<string>{"全年齡", "18 歲以上"} :
        vector<string>{"All Ages", "18 and Older"};
    ageChoice = chooseMenu(chineseInterface ? "書籍分級" : "Age Rating", ageOptions);
    if (ageChoice == 0) return;
    bool adultOnly = ageChoice == 2;
    ifstream f("./Bookshelf/" + fn);
    if (!f.is_open()) {
        cout << (chineseInterface ? "自動建立檔案...\n書名：" :
                                   "Auto-creating...\nTitle: ");
        if (!readLineOrHome(t)) return;
        cout << (chineseInterface ? "作者：" : "Author: ");
        if (!readLineOrHome(a)) return;
        ofstream o("./Bookshelf/" + fn); 
        o << "type: new\nTitle: " << t << "\nAge: "
          << (adultOnly ? "18+" : "all") << "\nAuthor: " << a << "\nContent here.\n";
    }
    ofstream l("./Database/bookList.txt", ios::app); 
    l << "\n" << fn << " " << (char)toupper(ts[0]);
    cout << (chineseInterface ? "完成，重新啟動後套用。" :
                               "Done. Restart to apply.");
    waitForHome();
}

bool Library::getExit() { return exit; }

void Library::showLeaderboard() {
    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    cout << (chineseInterface ? "********** 熱門書籍排行榜 **********" :
                                "********** Popular Books Ranking **********") << endl;

    vector<Book*> sortedBooks = books;
    sort(sortedBooks.begin(), sortedBooks.end(), [](Book* a, Book* b) {
        return a->getBorrowCount() > b->getBorrowCount();
    });

    for (int i = 0; i < (int)sortedBooks.size(); i++) {
        cout << i + 1 << ". " << setw(20) << left << sortedBooks[i]->getTitle() 
             << (chineseInterface ? " | 借閱次數：" : " | Borrows: ")
             << sortedBooks[i]->getBorrowCount() << endl;
    }

    waitForHome();
}

void Library::login() {
    system("clear");
    cout << "===========================================" << endl;
    cout << (chineseInterface ? "               使用者登入系統              " :
                                "                 User Login                ") << endl;
    cout << "===========================================" << endl;
    cout << (chineseInterface ? "請輸入姓名（空白為訪客）：" :
                                "Enter your name (blank for Guest): ");
    string name;
    getline(cin, name);
    
    bool isGuest = false;
    if (name.empty() || name == "Guest" || name == "訪客") {
        name = "Guest";
        isGuest = true;
    }
    
    auto it = users.find(name);
    if (it != users.end()) {
        currentUser = it->second;
        currentUser->isGuest = isGuest;
        cout << (chineseInterface ? "已有帳號，歡迎 " :
                                    "Existing account, welcome ")
             << displayUserName(currentUser, chineseInterface) << "!" << endl;
    } else {
        int age = 0;
        cout << (chineseInterface ? "新用戶您好，請輸入您的年齡：" :
                                    "New user, please enter your age: ");
        string ageInput;
        getline(cin, ageInput);
        try {
            age = stoi(ageInput);
        } catch (...) {
            age = 0;
        }
        currentUser = new User(name, age, isGuest);
        users[name] = currentUser;
    }
    if (currentUser->isGuest) {
        currentUser->readingPoints = 0;
        currentUser->banUntil = 0;
    }
    saveUserState(currentUser);
    cout << (chineseInterface ? "\n歡迎，" : "\nWelcome, ") << displayUserName(currentUser, chineseInterface)
         << (chineseInterface ? "！按 Enter 進入圖書館..." :
                                "! Press Enter to enter the library...");
    getchar();
}

void Library::borrowBook(Book* b) {
    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    cout << (chineseInterface ? "========== 借閱書籍 ==========\n" :
                               "========== Borrow Book ==========\n");
    
    if (currentUser->isGuest) {
        cout << (chineseInterface ?
            "\e[1;31m提醒：訪客帳號無法借閱書籍！\e[0m" :
            "\e[1;31mWarning: Guest accounts cannot borrow books!\e[0m") << endl;
        waitForHome();
        return;
    }

    if (isUserBanned()) {
        cout << RED << (chineseInterface ? "你的帳號目前被停權，解除時間：" :
                                           "Your account is banned until: ")
             << formatClock(currentUser->banUntil) << NONE << endl;
        waitForHome();
        return;
    }

    if (b->isAdultOnly() && currentUser->age < 18) {
        cout << RED << (chineseInterface ?
            "此書為 18 歲以上分級，你目前年齡未滿 18 歲，無法借閱。" :
            "This book is rated 18+. You are under 18, so you cannot borrow it.")
             << NONE << endl;
        waitForHome();
        return;
    }
    
    for (const string& title : globalBorrowedBooks) {
        if (title == b->getTitle()) {
            cout << (chineseInterface ?
                "\e[1;31m提醒：這本書已被借閱！\e[0m" :
                "\e[1;31mWarning: This book is already borrowed!\e[0m") << endl;
            waitForHome();
            return;
        }
    }
    
    vector<string> periodOptions = chineseInterface ?
        vector<string>{"1 分鐘", "3 分鐘", "5 分鐘", "其他（輸入 N 分鐘）"} :
        vector<string>{"1 minute", "3 minutes", "5 minutes", "Other (enter N minutes)"};
    int periodChoice = chooseMenu(chineseInterface ? "請選擇借閱期限" :
                                                     "Choose Borrow Period", periodOptions);
    if (periodChoice == 0) return;
    int borrowMinutes = 3;
    if (periodChoice == 1) borrowMinutes = 1;
    else if (periodChoice == 3) borrowMinutes = 5;
    else if (periodChoice == 4) {
        cout << (chineseInterface ? "請輸入借閱分鐘數：" : "Enter borrow minutes: ");
        if (!readNumberOrHome(borrowMinutes)) return;
        if (borrowMinutes <= 0) {
            cout << (chineseInterface ? "分鐘數必須大於 0。" :
                                        "Minutes must be greater than 0.") << endl;
            waitForHome();
            return;
        }
    }

    currentUser->borrowedBooks.push_back(b->getTitle());
    globalBorrowedBooks.push_back(b->getTitle());
    borrowedByUser[b->getTitle()] = currentUser->name;
    currentUser->readingPoints += 10;
    saveUserState(currentUser);
    
    time_t dueDate = time(0) + borrowMinutes * 60;
    currentUser->dueDates.push_back(dueDate);
    
    b->addBorrowCount();
    saveReviewRecord(b->getFilename(), 'B', "1");
    saveBorrowRecords();
    
    if (chineseInterface) cout << "成功借閱《" << b->getTitle() << "》！" << endl;
    else cout << "Successfully borrowed \"" << b->getTitle() << "\"!" << endl;
    
    char buf[80];
    struct tm* timeinfo = localtime(&dueDate);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    cout << (chineseInterface ? "借閱期限：" : "Borrow Period: ") << borrowMinutes
         << (chineseInterface ? " 分鐘" : " minutes") << endl;
    cout << (chineseInterface ? "到期日：" : "Due Date: ") << buf << endl;
    
    waitForHome();
}

void Library::viewAccount() {
    while (true) {
        system("clear");
        cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
             << NONE << "\n\n";
        cout << (chineseInterface ? "========== 我的帳號 ==========\n" :
                                   "========== My Account ==========\n");
        cout << (chineseInterface ? "使用者名稱：" : "User Name: ")
             << displayUserName(currentUser, chineseInterface) << endl;
        cout << (chineseInterface ? "年齡：" : "Age: ") << currentUser->age << endl;
        cout << (chineseInterface ? "閱讀稱號：" : "Reader Title: ")
             << getReaderTitle() << " (" << currentUser->readingPoints
             << (chineseInterface ? " 分)" : " points)") << endl;
        cout << (chineseInterface ? "停權狀態：" : "Ban Status: ");
        if (isUserBanned()) {
            cout << RED << (chineseInterface ? "停權中，解除時間：" : "Banned until: ")
                 << formatClock(currentUser->banUntil) << NONE << endl;
        } else {
            cout << (chineseInterface ? "正常" : "Normal") << endl;
        }
        cout << (chineseInterface ? "預覽：" : "Previewed: ")
             << currentUser->readingHistory.size()
             << (chineseInterface ? " | 歸還：" : " | Returned: ")
             << currentUser->returnedBooks.size()
             << (chineseInterface ? " | 收藏：" : " | Favorites: ")
             << currentUser->favoriteBooks.size() << endl;
        cout << "-------------------------------------------" << endl;
        cout << (chineseInterface ? "借閱書籍：" : "Borrowed Books List:") << endl;
        
        if (currentUser->borrowedBooks.empty()) {
            cout << (chineseInterface ? "目前沒有借閱書籍。" : "No borrowed books.") << endl;
        } else {
            for (int i = 0; i < (int)currentUser->borrowedBooks.size(); i++) {
                time_t dueDate = currentUser->dueDates[i];
                char buf[80];
                struct tm* timeinfo = localtime(&dueDate);
                strftime(buf, sizeof(buf), "%Y-%m-%d", timeinfo);
                int overdueMinutes = calculateOverdueMinutes(dueDate);
                cout << i + 1 << ". " << setw(30) << left << currentUser->borrowedBooks[i]
                     << (chineseInterface ? " | 到期日：" : " | Due Date: ") << buf;
                if (overdueMinutes > 0) {
                    int banMinutes = overdueMinutes * 10;
                    cout << RED << (chineseInterface ? " | 逾期 " : " | Overdue ")
                         << overdueMinutes
                         << (chineseInterface ? " 分鐘沒還，將封禁 " :
                                                " minutes late, ban ")
                         << banMinutes
                         << (chineseInterface ? " 分鐘" : " minutes") << NONE;
                }
                cout << endl;
            }
        }

        cout << "-------------------------------------------" << endl;
        cout << (chineseInterface ? "收藏書籍：" : "Favorite Books:") << endl;
        if (currentUser->favoriteBooks.empty()) {
            cout << (chineseInterface ? "目前沒有收藏書籍。" :
                                       "No favorite books yet.") << endl;
        } else {
            for (int i = 0; i < (int)currentUser->favoriteBooks.size(); i++) {
                cout << i + 1 << ". " << currentUser->favoriteBooks[i] << endl;
            }
        }

        cout << "-------------------------------------------" << endl;
        cout << (chineseInterface ? "最近閱讀紀錄：" : "Recent Reading History:") << endl;
        if (currentUser->readingHistory.empty()) {
            cout << (chineseInterface ? "目前沒有預覽紀錄。" :
                                       "No books previewed yet.") << endl;
        } else {
            int start = max(0, (int)currentUser->readingHistory.size() - 5);
            for (int i = (int)currentUser->readingHistory.size() - 1; i >= start; i--) {
                cout << "- " << currentUser->readingHistory[i] << endl;
            }
        }
        
        cout << "-------------------------------------------" << endl;
        cout << (chineseInterface ? "\n按 Enter 開啟帳號功能選單..." :
                                    "\nPress Enter to open account actions...");
        waitForHome();
        vector<string> accountOptions = chineseInterface ?
            vector<string>{"歸還書籍", "登出", "返回主選單", "語言"} :
            vector<string>{"Return Book", "Log Out", "Back to Main Menu", "Language"};
        int choice = chooseMenu(chineseInterface ? "帳號功能" : "Account Actions",
                                accountOptions);
        if (choice != 0) {
            if (choice == 1) {
                if (currentUser->borrowedBooks.empty()) {
                    cout << (chineseInterface ? "目前沒有可歸還的書籍。" :
                                               "You have no books to return.") << endl;
                    if (!waitForHome()) return;
                    continue;
                }
                vector<string> returnOptions;
                for (const string& title : currentUser->borrowedBooks) {
                    returnOptions.push_back(title);
                }
                int num = chooseMenu(chineseInterface ? "請選擇要歸還的書籍" :
                                                        "Choose a Book to Return",
                                     returnOptions);
                if (num != 0) {
                    if (num > 0 && num <= (int)currentUser->borrowedBooks.size()) {
                        string retTitle = currentUser->borrowedBooks[num - 1];
                        time_t dueDate = currentUser->dueDates[num - 1];
                        int overdueMinutes = calculateOverdueMinutes(dueDate);
                        int banMinutes = overdueMinutes > 0 ? overdueMinutes * 10 : 0;
                        currentUser->borrowedBooks.erase(currentUser->borrowedBooks.begin() + (num - 1));
                        currentUser->dueDates.erase(currentUser->dueDates.begin() + (num - 1));
                        currentUser->returnedBooks.push_back(retTitle);
                        currentUser->readingPoints += 15;
                        if (overdueMinutes > 0) {
                            time_t newBanUntil = time(0) + banMinutes * 60;
                            if (newBanUntil > currentUser->banUntil) {
                                currentUser->banUntil = newBanUntil;
                            }
                        }
                        saveUserState(currentUser);
                        
                        auto it = find(globalBorrowedBooks.begin(), globalBorrowedBooks.end(), retTitle);
                        if (it != globalBorrowedBooks.end()) {
                            globalBorrowedBooks.erase(it);
                        }
                        borrowedByUser.erase(retTitle);
                        Book* returnedBook = nullptr;
                        for (Book* book : books) {
                            if (book->getTitle() == retTitle) {
                                returnedBook = book;
                                break;
                            }
                        }
                        if (returnedBook != nullptr) {
                            saveReturnRecord(returnedBook->getFilename(), currentUser->name,
                                             dueDate, overdueMinutes, banMinutes);
                        }
                        saveBorrowRecords();
                        
                        if (chineseInterface) cout << "成功歸還《" << retTitle << "》！" << endl;
                        else cout << "Successfully returned \"" << retTitle << "\"!" << endl;
                        if (overdueMinutes > 0) {
                            cout << RED << (chineseInterface ? "逾期 " : "Overdue ")
                                 << overdueMinutes
                                 << (chineseInterface ? " 分鐘沒還，已被封禁 " :
                                                        " minutes late, banned for ")
                                 << banMinutes
                                 << (chineseInterface ? " 分鐘" : " minutes") << endl
                                 << (chineseInterface ? "封禁到：" : "Ban ends at: ")
                                 << formatClock(currentUser->banUntil) << NONE << endl;
                        }
                        if (!waitForHome()) return;
                    } else {
                        cout << (chineseInterface ? "編號無效！" : "Invalid number!") << endl;
                        if (!waitForHome()) return;
                    }
                } else return;
            } else if (choice == 2) {
                login();
                break;
            } else if (choice == 3) {
                break;
            } else if (choice == 4) {
                chooseLanguage();
            }
        } else break;
    }
}

void Library::recommendBooks() {
    vector<string> moodOptions = chineseInterface ?
        vector<string>{"平靜沉思", "好奇求知", "冒險想像", "給我驚喜"} :
        vector<string>{"Calm and Reflective", "Curious to Learn",
                       "Adventurous Imagination", "Surprise Me"};
    int mood = chooseMenu(chineseInterface ? "閱讀羅盤：目前心情" :
                                             "Reading Compass: Current Mood",
                          moodOptions);
    if (mood == 0) return;

    vector<string> timeOptions = chineseInterface ?
        vector<string>{"短暫休息", "長時間閱讀"} :
        vector<string>{"Short Break", "Long Reading Session"};
    int readingTime = chooseMenu(chineseInterface ? "閱讀羅盤：閱讀時間" :
                                                    "Reading Compass: Reading Time",
                                 timeOptions);
    if (readingTime == 0) return;

    vector<string> discoveryOptions = chineseInterface ?
        vector<string>{"熱門好書", "冷門寶藏"} :
        vector<string>{"Popular Picks", "Hidden Gems"};
    int discovery = chooseMenu(chineseInterface ? "閱讀羅盤：探索偏好" :
                                                  "Reading Compass: Discovery Style",
                               discoveryOptions);
    if (discovery == 0) return;

    struct Recommendation {
        Book* book;
        int score;
        string reason;
    };

    vector<Recommendation> results;
    vector<string> recommendedTitles;
    const vector<string> calmWords = {
        "story", "history", "forest", "alice", "literature"
    };
    const vector<string> learnWords = {
        "math", "science", "history", "education", "morse", "code"
    };
    const vector<string> adventureWords = {
        "adventure", "fantasy", "forest", "castle", "space", "animation"
    };

    for (Book* book : getVisibleBooks()) {
        if (find(recommendedTitles.begin(), recommendedTitles.end(), book->getTitle()) !=
            recommendedTitles.end()) {
            continue;
        }
        recommendedTitles.push_back(book->getTitle());

        string profile = book->getTitle() + " " + book->getCategory() + " " +
                         book->getAuthor() + " " + book->getFilename();
        transform(profile.begin(), profile.end(), profile.begin(),
                  [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

        int score = 0;
        string reason;
        const vector<string>* moodWords = nullptr;
        if (mood == 1) moodWords = &calmWords;
        else if (mood == 2) moodWords = &learnWords;
        else if (mood == 3) moodWords = &adventureWords;

        if (moodWords != nullptr) {
            for (const string& word : *moodWords) {
                if (profile.find(word) != string::npos || book->searchContent(word)) {
                    score += 6;
                    if (reason.empty()) {
                        reason = chineseInterface ? "符合你目前的心情" :
                                                    "matches your current mood";
                    }
                }
            }
        } else {
            unsigned int seed = 0;
            for (char ch : currentUser->name + book->getTitle()) {
                seed = seed * 31 + static_cast<unsigned char>(ch);
            }
            score += seed % 11;
            reason = chineseInterface ? "專屬於你的驚喜選書" :
                                        "your personal wildcard pick";
        }

        int pages = book->getPageCount();
        bool timeMatch = (readingTime == 1 && pages <= 2) ||
                         (readingTime == 2 && pages > 1);
        if (timeMatch) {
            score += 4;
            if (reason.empty()) {
                reason = chineseInterface ? "符合你的閱讀時間" :
                                            "fits your available reading time";
            } else {
                reason += chineseInterface ? "，也符合你的閱讀時間" :
                                             " and fits your available time";
            }
        }

        if (discovery == 1) {
            score += book->getBorrowCount() * 2;
            if (book->getBorrowCount() > 0) {
                reason += chineseInterface ? "，且受到讀者喜愛" :
                                             ", with reader approval";
            }
        } else {
            score += max(0, 5 - book->getBorrowCount());
            if (book->getBorrowCount() == 0) {
                reason += chineseInterface ? "，是尚待發掘的寶藏" :
                                             ", an undiscovered gem";
            }
        }

        score += static_cast<int>(book->getAverageRating() * 3);
        score += min(5, book->getRatingCount());
        score += min(4, static_cast<int>(book->getComments().size()));
        if (book->getRatingCount() > 0 && reason.empty()) {
            reason = chineseInterface ? "評分表現優秀" : "has strong reader ratings";
        }

        if (reason.empty()) {
            reason = chineseInterface ? "綜合符合你的選擇" :
                                        "a balanced match for your answers";
        }
        results.push_back({book, score, reason});
    }

    stable_sort(results.begin(), results.end(),
                [](const Recommendation& a, const Recommendation& b) {
                    if (a.score != b.score) return a.score > b.score;
                    return a.book->getTitle() < b.book->getTitle();
                });

    cout << (chineseInterface ? "\n========== 羅盤推薦 ==========\n" :
                               "\n========== Your Compass Points To ==========\n");
    int count = min(3, static_cast<int>(results.size()));
    for (int i = 0; i < count; i++) {
        cout << i + 1 << ". " << results[i].book->getTitle()
             << " [" << results[i].book->getCategory() << "]" << endl;
        cout << (chineseInterface ? "   平均評分：" : "   Rating: ")
             << fixed << setprecision(1) << results[i].book->getAverageRating()
             << " / 5 (" << results[i].book->getRatingCount()
             << (chineseInterface ? " 筆評分)" : " ratings)") << endl;
        cout << (chineseInterface ? "   推薦原因：" : "   Why: ")
             << results[i].reason << (chineseInterface ? "。" : ".") << endl;
    }
    if (results.empty()) {
        cout << (chineseInterface ? "目前沒有可推薦的書籍。" :
                                   "No books are available yet.") << endl;
    }

    waitForHome();
}

void Library::toggleFavorite(Book* b) {
    if (currentUser->isGuest) {
        system("clear");
        cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
             << NONE << "\n\n";
        cout << (chineseInterface ?
            "\e[1;31m提醒：訪客帳號無法收藏書籍！\e[0m" :
            "\e[1;31mWarning: Guest accounts cannot favorite books!\e[0m") << endl;
        waitForHome();
        return;
    }

    vector<string>& favorites = currentUser->favoriteBooks;
    auto it = find(favorites.begin(), favorites.end(), b->getTitle());

    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    cout << (chineseInterface ? "========== 收藏書籍 ==========\n" :
                               "========== Favorite Books ==========\n");
    if (it == favorites.end()) {
        favorites.push_back(b->getTitle());
        saveReviewRecord(b->getFilename(), 'F', currentUser->name);
        currentUser->readingPoints += 2;
        saveUserState(currentUser);
        if (chineseInterface) cout << "已將《" << b->getTitle() << "》加入收藏。" << endl;
        else cout << "\"" << b->getTitle() << "\" was added to your favorites." << endl;
    } else {
        favorites.erase(it);
        saveReviewRecord(b->getFilename(), 'V', currentUser->name);
        if (chineseInterface) cout << "已將《" << b->getTitle() << "》移除收藏。" << endl;
        else cout << "\"" << b->getTitle() << "\" was removed from your favorites." << endl;
    }
    waitForHome();
}

void Library::rateBook(Book* b) {
    if (currentUser->isGuest) {
        system("clear");
        cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
             << NONE << "\n\n";
        cout << (chineseInterface ?
            "\e[1;31m提醒：訪客帳號無法評分書籍！\e[0m" :
            "\e[1;31mWarning: Guest accounts cannot rate books!\e[0m") << endl;
        waitForHome();
        return;
    }

    string title = chineseInterface ?
        "書籍評分：《" + b->getTitle() + "》  目前平均：" +
            to_string(b->getAverageRating()).substr(0, 3) + " / 5" :
        "Rate Book: \"" + b->getTitle() + "\"  Current Average: " +
            to_string(b->getAverageRating()).substr(0, 3) + " / 5";
    vector<string> ratingOptions = chineseInterface ?
        vector<string>{"1 分", "2 分", "3 分", "4 分", "5 分"} :
        vector<string>{"1 Star", "2 Stars", "3 Stars", "4 Stars", "5 Stars"};
    int rating = chooseMenu(title, ratingOptions);
    if (rating == 0) return;
    if (rating < 1 || rating > 5) {
        cout << (chineseInterface ? "評分無效，請輸入 1 到 5。" :
                                    "Invalid rating. Please choose 1 to 5.") << endl;
    } else {
        b->setRating(currentUser->name, rating);
        saveReviewRecord(b->getFilename(), 'R', currentUser->name + "|" + to_string(rating));
        currentUser->readingPoints += 3;
        saveUserState(currentUser);
        cout << (chineseInterface ? "評分完成，謝謝你的回饋！" :
                                    "Rating saved. Thanks for your feedback!") << endl;
    }
    waitForHome();
}

void Library::commentBook(Book* b) {
    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    if (currentUser->isGuest) {
        cout << (chineseInterface ?
            "\e[1;31m提醒：訪客帳號無法撰寫評論！\e[0m" :
            "\e[1;31mWarning: Guest accounts cannot write comments!\e[0m") << endl;
        waitForHome();
        return;
    }

    cout << (chineseInterface ? "========== 撰寫評論 ==========" :
                                "========== Write Comment ==========") << endl;
    cout << (chineseInterface ? "書名：《" : "Book: \"") << b->getTitle()
         << (chineseInterface ? "》" : "\"") << endl;
    cout << (chineseInterface ? "請輸入評論內容：" : "Enter your comment: ");

    string comment;
    if (!readLineOrHome(comment)) return;
    if (comment.empty()) {
        cout << (chineseInterface ? "評論不可空白。" : "Comment cannot be empty.") << endl;
    } else {
        b->addComment(currentUser->name, comment);
        saveReviewRecord(b->getFilename(), 'C', currentUser->name + "|" + comment);
        currentUser->readingPoints += 4;
        saveUserState(currentUser);
        cout << (chineseInterface ? "評論已新增。" : "Comment added.") << endl;
    }
    waitForHome();
}

void Library::showBookReviews(Book* b) {
    system("clear");
    cout << DIM << (chineseInterface ? "[End] 返回主頁" : "[End] Back to Home")
         << NONE << "\n\n";
    cout << (chineseInterface ? "========== 評分與評論 ==========" :
                                "========== Ratings and Comments ==========") << endl;
    cout << (chineseInterface ? "書名：《" : "Book: \"") << b->getTitle()
         << (chineseInterface ? "》" : "\"") << endl;
    cout << (chineseInterface ? "平均評分：" : "Average Rating: ")
         << fixed << setprecision(1) << b->getAverageRating()
         << " / 5" << (chineseInterface ? "（" : " (") << b->getRatingCount()
         << (chineseInterface ? " 筆）" : " ratings)") << endl;
    cout << "-------------------------------------------" << endl;

    const map<string, int>& ratings = b->getRatings();
    cout << (chineseInterface ? "評分：" : "Ratings:") << endl;
    if (ratings.empty()) {
        cout << (chineseInterface ? "目前沒有評分。" : "No ratings yet.") << endl;
    } else {
        for (const auto& item : ratings) {
            cout << "[" << item.first << "] : ";
            for (int i = 1; i <= 5; i++) {
                cout << (i <= item.second ? "★" : "☆");
            }
            cout << endl;
        }
    }
    cout << "-------------------------------------------" << endl;

    const vector<string>& comments = b->getComments();
    cout << (chineseInterface ? "評論：" : "Comments:") << endl;
    if (comments.empty()) {
        cout << (chineseInterface ? "目前沒有評論。" : "No comments yet.") << endl;
    } else {
        for (int i = 0; i < (int)comments.size(); i++) {
            string user = chineseInterface ? "使用者" : "User";
            string comment = comments[i];
            size_t sep = comments[i].find(':');
            if (sep != string::npos) {
                user = comments[i].substr(0, sep);
                comment = comments[i].substr(sep + 1);
                while (!comment.empty() && comment.front() == ' ') comment.erase(comment.begin());
            }
            cout << "[" << user << "] : " << comment << endl;
        }
    }
    waitForHome();
}

void Library::loadReviews() {
    ifstream in("./Database/reviews.txt");
    if (!in.is_open()) return;

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        size_t first = line.find('\t');
        size_t second = line.find('\t', first == string::npos ? 0 : first + 1);
        if (first == string::npos || second == string::npos || second + 1 > line.size()) continue;

        string filename = line.substr(0, first);
        char kind = line[first + 1];
        string value = line.substr(second + 1);

        Book* target = nullptr;
        for (Book* book : books) {
            if (book->getFilename() == filename) {
                target = book;
                break;
            }
        }
        if (target == nullptr) continue;

        if (kind == 'R') {
            try {
                size_t sep = value.find('|');
                if (sep == string::npos) {
                    target->addRating(stoi(value));
                } else {
                    string user = value.substr(0, sep);
                    int rating = stoi(value.substr(sep + 1));
                    target->setRating(user, rating);
                }
            } catch (...) {
            }
        } else if (kind == 'B') {
            try {
                int count = stoi(value);
                for (int i = 0; i < count; i++) target->addBorrowCount();
            } catch (...) {
            }
        } else if (kind == 'C') {
            size_t sep = value.find('|');
            if (sep == string::npos) continue;
            string user = value.substr(0, sep);
            string comment = value.substr(sep + 1);
            target->addComment(user, comment);
        } else if (false && kind == 'L') {
            size_t sep1 = value.find('|');
            size_t sep2 = value.find('|', sep1 == string::npos ? 0 : sep1 + 1);
            if (sep1 == string::npos || sep2 == string::npos) continue;
            string userName = value.substr(0, sep1);
            int age = 0;
            time_t dueDate = 0;
            try {
                age = stoi(value.substr(sep1 + 1, sep2 - sep1 - 1));
                dueDate = static_cast<time_t>(stoll(value.substr(sep2 + 1)));
            } catch (...) {
                continue;
            }

            User* borrower = nullptr;
            auto userIt = users.find(userName);
            if (userIt == users.end()) {
                borrower = new User(userName, age, userName == "Guest");
                users[userName] = borrower;
            } else {
                borrower = userIt->second;
                borrower->age = age;
            }

            if (borrowedByUser.find(target->getTitle()) == borrowedByUser.end()) {
                borrower->borrowedBooks.push_back(target->getTitle());
                borrower->dueDates.push_back(dueDate);
                globalBorrowedBooks.push_back(target->getTitle());
                borrowedByUser[target->getTitle()] = userName;
            }
        } else if (false && kind == 'U') {
            string userName = value;
            auto userIt = users.find(userName);
            if (userIt != users.end()) {
                User* borrower = userIt->second;
                for (int i = 0; i < (int)borrower->borrowedBooks.size(); i++) {
                    if (borrower->borrowedBooks[i] == target->getTitle()) {
                        borrower->borrowedBooks.erase(borrower->borrowedBooks.begin() + i);
                        if (i < (int)borrower->dueDates.size()) {
                            borrower->dueDates.erase(borrower->dueDates.begin() + i);
                        }
                        break;
                    }
                }
                borrower->returnedBooks.push_back(target->getTitle());
            }

            auto globalIt = find(globalBorrowedBooks.begin(), globalBorrowedBooks.end(), target->getTitle());
            if (globalIt != globalBorrowedBooks.end()) globalBorrowedBooks.erase(globalIt);
            borrowedByUser.erase(target->getTitle());
        } else if (kind == 'F') {
            string userName = value;
            auto userIt = users.find(userName);
            User* user = nullptr;
            if (userIt == users.end()) {
                user = new User(userName, 0, userName == "Guest");
                users[userName] = user;
            } else {
                user = userIt->second;
            }
            if (find(user->favoriteBooks.begin(), user->favoriteBooks.end(), target->getTitle()) ==
                user->favoriteBooks.end()) {
                user->favoriteBooks.push_back(target->getTitle());
            }
        } else if (kind == 'V') {
            string userName = value;
            auto userIt = users.find(userName);
            if (userIt != users.end()) {
                vector<string>& favorites = userIt->second->favoriteBooks;
                auto favIt = find(favorites.begin(), favorites.end(), target->getTitle());
                if (favIt != favorites.end()) favorites.erase(favIt);
            }
        } else if (kind == 'H') {
            string userName = value;
            auto userIt = users.find(userName);
            User* user = nullptr;
            if (userIt == users.end()) {
                user = new User(userName, 0, userName == "Guest");
                users[userName] = user;
            } else {
                user = userIt->second;
            }
            user->readingHistory.push_back(target->getTitle());
        }
    }
}

void Library::saveReviewRecord(const string& filename, char kind, const string& value) {
    ofstream out("./Database/reviews.txt", ios::app);
    if (!out.is_open()) return;
    out << filename << '\t' << kind << '\t' << value << '\n';
}

void Library::loadUserStates() {
    ifstream in("./UserData/users.txt");
    if (!in.is_open()) return;

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        vector<string> fields;
        size_t start = 0;
        while (true) {
            size_t tab = line.find('\t', start);
            if (tab == string::npos) {
                fields.push_back(line.substr(start));
                break;
            }
            fields.push_back(line.substr(start, tab - start));
            start = tab + 1;
        }
        if (fields.size() < 5) continue;

        try {
            string name = fields[0];
            int age = stoi(fields[1]);
            bool isGuest = fields[2] == "1";
            int points = stoi(fields[3]);
            time_t banUntil = static_cast<time_t>(stoll(fields[4]));

            User* user = nullptr;
            auto it = users.find(name);
            if (it == users.end()) {
                user = new User(name, age, isGuest);
                users[name] = user;
            } else {
                user = it->second;
                user->age = age;
                user->isGuest = isGuest;
            }
            user->readingPoints = points;
            user->banUntil = banUntil;
        } catch (...) {
        }
    }
}

void Library::saveUserState(User* user) {
    if (user == nullptr || user->isGuest) return;
    users[user->name] = user;

    ensureUserDataDirectory();
    ofstream out("./UserData/users.txt");
    if (!out.is_open()) return;
    for (const auto& pair : users) {
        User* savedUser = pair.second;
        if (savedUser == nullptr || savedUser->isGuest) continue;
        out << savedUser->name << '\t'
            << savedUser->age << '\t'
            << (savedUser->isGuest ? 1 : 0) << '\t'
            << savedUser->readingPoints << '\t'
            << static_cast<long long>(savedUser->banUntil) << '\n';
    }
}

void Library::loadBorrowRecords() {
    ifstream in("./Database/borrow.txt");
    if (!in.is_open()) return;

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        vector<string> fields;
        size_t start = 0;
        while (true) {
            size_t tab = line.find('\t', start);
            if (tab == string::npos) {
                fields.push_back(line.substr(start));
                break;
            }
            fields.push_back(line.substr(start, tab - start));
            start = tab + 1;
        }
        if (fields.size() < 4) continue;

        string filename = fields[0];
        string userName = fields[1];
        int age = 0;
        time_t dueDate = 0;
        try {
            age = stoi(fields[2]);
            dueDate = static_cast<time_t>(stoll(fields[3]));
        } catch (...) {
            continue;
        }

        Book* target = nullptr;
        for (Book* book : books) {
            if (book->getFilename() == filename) {
                target = book;
                break;
            }
        }
        if (target == nullptr || borrowedByUser.find(target->getTitle()) != borrowedByUser.end()) {
            continue;
        }

        User* borrower = nullptr;
        auto userIt = users.find(userName);
        if (userIt == users.end()) {
            borrower = new User(userName, age, userName == "Guest");
            users[userName] = borrower;
        } else {
            borrower = userIt->second;
            if (borrower->age == 0) borrower->age = age;
        }

        borrower->borrowedBooks.push_back(target->getTitle());
        borrower->dueDates.push_back(dueDate);
        globalBorrowedBooks.push_back(target->getTitle());
        borrowedByUser[target->getTitle()] = userName;
    }
}

void Library::saveBorrowRecords() {
    ofstream out("./Database/borrow.txt");
    if (!out.is_open()) return;

    for (const auto& pair : users) {
        User* user = pair.second;
        if (user == nullptr || user->isGuest) continue;
        for (int i = 0; i < (int)user->borrowedBooks.size(); i++) {
            string title = user->borrowedBooks[i];
            Book* target = nullptr;
            for (Book* book : books) {
                if (book->getTitle() == title) {
                    target = book;
                    break;
                }
            }
            if (target == nullptr || i >= (int)user->dueDates.size()) continue;
            out << target->getFilename() << '\t'
                << user->name << '\t'
                << user->age << '\t'
                << static_cast<long long>(user->dueDates[i]) << '\n';
        }
    }
}

void Library::saveReturnRecord(const string& filename, const string& userName,
                               time_t dueDate, int overdueMinutes, int banMinutes) {
    ofstream out("./Database/alive.txt", ios::app);
    if (!out.is_open()) return;
    out << filename << '\t'
        << userName << '\t'
        << static_cast<long long>(dueDate) << '\t'
        << static_cast<long long>(time(0)) << '\t'
        << overdueMinutes << '\t'
        << banMinutes << '\n';
}

int Library::calculateOverdueMinutes(time_t dueDate) const {
    time_t now = time(0);
    if (now <= dueDate) return 0;
    return static_cast<int>((now - dueDate) / 60) + 1;
}

bool Library::isUserBanned() const {
    return currentUser != nullptr && currentUser->banUntil > time(0);
}

string Library::formatDate(time_t value) const {
    char buf[80];
    struct tm* timeinfo = localtime(&value);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buf);
}

string Library::formatClock(time_t value) const {
    char buf[16];
    struct tm* timeinfo = localtime(&value);
    strftime(buf, sizeof(buf), "%H:%M", timeinfo);
    return string(buf);
}

vector<Book*> Library::getVisibleBooks() const {
    vector<Book*> visible;
    int age = currentUser == nullptr ? 0 : currentUser->age;
    for (Book* book : books) {
        bool alreadyBorrowed = borrowedByUser.find(book->getTitle()) != borrowedByUser.end();
        if ((!book->isAdultOnly() || age >= 18) && !alreadyBorrowed) {
            visible.push_back(book);
        }
    }
    return visible;
}

bool Library::isBorrowedByOtherUser(Book* b) const {
    if (currentUser == nullptr) return false;
    auto it = borrowedByUser.find(b->getTitle());
    return it != borrowedByUser.end() && it->second != currentUser->name;
}

void Library::recordPreview(Book* b) {
    if (currentUser->isGuest) return;
    vector<string>& history = currentUser->readingHistory;
    bool firstPreview = find(history.begin(), history.end(), b->getTitle()) == history.end();
    history.push_back(b->getTitle());
    saveReviewRecord(b->getFilename(), 'H', currentUser->name);
    if (firstPreview) {
        currentUser->readingPoints += 5;
        saveUserState(currentUser);
    }
}

string Library::getReaderTitle() const {
    if (currentUser != nullptr && currentUser->isGuest) {
        return chineseInterface ? "無" : "None";
    }
    int points = currentUser->readingPoints;
    if (points >= 100) return chineseInterface ? "圖書館傳奇" : "Library Legend";
    if (points >= 60) return chineseInterface ? "書海探險家" : "Book Explorer";
    if (points >= 30) return chineseInterface ? "故事追尋者" : "Story Seeker";
    if (points >= 10) return chineseInterface ? "好奇讀者" : "Curious Reader";
    return chineseInterface ? "閱讀新手" : "New Reader";
}

void Library::chooseLanguage() {
    vector<string> options = chineseInterface ?
        vector<string>{"英文", "中文"} :
        vector<string>{"English", "Chinese"};
    int choice = chooseMenu(chineseInterface ? "語言設定" : "Language Settings", options);
    if (choice == 1 || choice == 2) {
        chineseInterface = choice == 2;
        for (Book* book : books) book->setChinese(chineseInterface);
        cout << (chineseInterface ? "\n介面語言已切換為中文。" :
                                   "\nInterface language changed to English.") << endl;
    }
    waitForHome();
}
