#define _HAS_STD_BYTE 0  // 確保關閉 C++ 標準庫的 std::byte 以免與 Windows 發生衝突
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <conio.h>
#include "book.h"        // 搬移到 Windows 系統標頭檔之後

Book::Book(string f, string t, string a, string c)
    : filename(f), title(t), author(a), category(c) {}

Book::~Book() {
    for (auto p : page_vec) delete p;
}

string Book::getTitle() { return title; }
string Book::getAuthor() { return author; }
string Book::getCategory() { return category; }
string Book::getFilename() { return filename; }

char Book::getKey() {
    int key = _getch();

    // 防禦性安全防護：防範不相容環境造成畫面突發消失空轉
    if (key == -1 || key == 255) {
        Sleep(50);
        return ' ';
    }

    // 擴充：支援新舊控制台的 224 與 0 前綴
    if (key == 224 || key == 0) {
        int k = _getch();
        if (k == 77) return 'N'; // 右方向鍵 -> 下一頁
        if (k == 75) return 'P'; // 左方向鍵 -> 上一頁
        if (k == 79) return 'M'; // End 鍵 -> 返回選單
    }

    if (toupper(key) == 'J') return 'J';

    return ' ';
}

void Book::preview() {
    int cur = 0;
    readContent();

    while (1) {
        system("cls");

        cout << title << "\n"
             << string(PAGE_W, '=') << "\n\n";

        if (cur < (int)page_vec.size())
            page_vec[cur]->showPageCont();

        cout << "\n"
             << string(PAGE_W / 2 - 4, ' ')
             << "Page " << cur << "\n"
             << string(PAGE_W, '=') << "\n";

        cout << " [→]:Next  [←]:Prev  [J]:Jump  [ESC]:Menu\n";

        char op = getKey();

        if (op == 'N' && cur < (int)page_vec.size() - 1)
            cur++;

        else if (op == 'P' && cur > 0)
            cur--;

        else if (op == 'J') {
            cout << "\nJump to: ";

            int t;
            cin >> t;

            if (t >= 0 && t < (int)page_vec.size())
                cur = t;
        }

        else if (GetAsyncKeyState(VK_ESCAPE))
            break;
    }
}

void Book::readContent() {
    fstream fin("./TXT/" + filename, ios::in);

    if (!fin.is_open()) return;

    string s;

    while (getline(fin, s))
        if (s.find("Author:") != string::npos)
            break;

    int lc = 0;

    while (getline(fin, s)) {

        if (!s.empty() && s.back() == '\r')
            s.pop_back();

        if (lc == 0 || lc >= PAGE_H) {

            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);

            char** c = new char*[PAGE_H];

            for (int i = 0; i < PAGE_H; i++) {
                c[i] = new char[PAGE_W];
                memset(c[i], 0, PAGE_W);
            }

            p->setPageCont(c);
            page_vec.push_back(p);

            lc = 0;
        }

        strncpy(page_vec.back()->getPageCont()[lc++],
                s.c_str(),
                PAGE_W - 1);
    }
}

bool Book::searchContent(string q) {

    if (page_vec.empty())
        readContent();

    for (auto p : page_vec) {

        for (int i = 0; i < PAGE_H; i++) {

            if (string(p->getPageCont()[i]).find(q) != string::npos)
                return true;
        }
    }

    return false;
}

void MthBook::readContent() {

    fstream fin("./TXT/" + filename, ios::in);

    string s;

    while (getline(fin, s))
        if (s.find("Author:") != string::npos)
            break;

    int lc = 0;

    while (getline(fin, s)) {

        if (!s.empty() && s.back() == '\r')
            s.pop_back();

        size_t q1 = s.find('?');

        while (q1 != string::npos) {

            size_t q2 = s.find('?', q1 + 1);

            if (q2 != string::npos) {

                string ans =
                    calculateEquation(s.substr(q1 + 1, q2 - q1 - 1));

                s.replace(q1, q2 - q1 + 1, ans);

                q1 = s.find('?', q1 + ans.length());

            } else break;
        }

        if (lc == 0 || lc >= PAGE_H) {

            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);

            char** c = new char*[PAGE_H];

            for (int i = 0; i < PAGE_H; i++) {

                c[i] = new char[PAGE_W];
                memset(c[i], 0, PAGE_W);
            }

            p->setPageCont(c);
            page_vec.push_back(p);

            lc = 0;
        }

        strncpy(page_vec.back()->getPageCont()[lc++],
                s.c_str(),
                PAGE_W - 1);
    }
}

string MthBook::calculateEquation(string eq) {

    string c = "";

    for (char ch : eq)
        if (ch != ' ')
            c += ch;

    try {

        if (c.find("sqrt") != string::npos)
            return to_string(
                (int)sqrt(stod(c.substr(c.find('(') + 1)))
            );

        size_t p = c.find('+');

        if (p != string::npos)
            return to_string(
                (int)(stod(c.substr(0, p)) +
                stod(c.substr(p + 1)))
            );

        p = c.find('-');

        if (p != string::npos)
            return to_string(
                (int)(stod(c.substr(0, p)) -
                stod(c.substr(p + 1)))
            );

        p = c.find('*');

        if (p != string::npos)
            return to_string(
                (int)(stod(c.substr(0, p)) *
                stod(c.substr(p + 1)))
            );

        p = c.find('/');

        if (p != string::npos)
            return to_string(
                (int)(stod(c.substr(0, p)) /
                stod(c.substr(p + 1)))
            );

    } catch (...) {
        return "err";
    }

    return "0";
}

void AniBook::preview() {

    fstream fin("./TXT/" + filename, ios::in);

    string s;

    vector<vector<string>> frames;
    vector<string> cf;

    bool in = false;

    while (getline(fin, s)) {

        if (!s.empty() && s.back() == '\r')
            s.pop_back();

        if (s.find(".figend") != string::npos) {

            in = false;
            frames.push_back(cf);
            cf.clear();
        }

        else if (s.find(".fig") != string::npos)
            in = true;

        else if (in)
            cf.push_back(s);
    }

    int cur = 0;

    if (frames.empty()) return;

    while (1) {

        system("cls");

        cout << title << "\n"
             << string(PAGE_W, '=') << "\n\n";

        for (auto l : frames[cur])
            cout << l << endl;

        cout << "\n"
             << string(PAGE_W, '=')
             << "\n [ESC]: Back to Menu\n";

        Sleep(200);

        if (_kbhit()) {

            int k = _getch();

            if (k == 27)
                break;
        }

        cur = (cur + 1) % frames.size();
    }
}

void Book::addBorrowCount() {
    borrowCount++;
}

int Book::getBorrowCount() const {
    return borrowCount;
}

// =================================================================
// FigBook 類別與 MorseBook 類別的完整多型功能實作體
// =================================================================
char** FigBook::get_figure(fstream& fin, int* fig_h) {
    string s;
    vector<string> fig_lines;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (s.find(".figend") != string::npos) break;
        fig_lines.push_back(s);
    }
    *fig_h = fig_lines.size();
    if (*fig_h == 0) return nullptr;

    char** res = new char*[*fig_h];
    for (int i = 0; i < *fig_h; i++) {
        res[i] = new char[PAGE_W];
        memset(res[i], 0, PAGE_W);
        strncpy(res[i], fig_lines[i].c_str(), PAGE_W - 1);
    }
    return res;
}

void FigBook::readContent() {
    fstream fin("./TXT/" + filename, ios::in);
    if (!fin.is_open()) return;

    string s;
    while (getline(fin, s)) {
        if (s.find("Author:") != string::npos) break;
    }

    int lc = 0;
    Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
    char** c = new char*[PAGE_H];
    for (int i = 0; i < PAGE_H; i++) { 
        c[i] = new char[PAGE_W]; 
        memset(c[i], 0, PAGE_W); 
    }
    p->setPageCont(c);
    page_vec.push_back(p);

    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();

        if (s.find(".fig") != string::npos) {
            int fig_h = 0;
            char** fig = get_figure(fin, &fig_h);
            if (!fig) continue;

            // 智慧分頁演算法：如果當前頁面放不下這張圖，直接換到下一頁，防止 ASCII Art 被鋸斷
            if (lc + fig_h >= PAGE_H) {
                p = new Page(page_vec.size(), PAGE_W, PAGE_H);
                c = new char*[PAGE_H];
                for (int i = 0; i < PAGE_H; i++) { 
                    c[i] = new char[PAGE_W]; 
                    memset(c[i], 0, PAGE_W); 
                }
                p->setPageCont(c);
                page_vec.push_back(p);
                lc = 0;
            }

            for (int i = 0; i < fig_h; i++) {
                strncpy(page_vec.back()->getPageCont()[lc++], fig[i], PAGE_W - 1);
                delete[] fig[i];
            }
            delete[] fig;
        } else {
            if (lc >= PAGE_H) {
                p = new Page(page_vec.size(), PAGE_W, PAGE_H);
                c = new char*[PAGE_H];
                for (int i = 0; i < PAGE_H; i++) { 
                    c[i] = new char[PAGE_W]; 
                    memset(c[i], 0, PAGE_W); 
                }
                p->setPageCont(c);
                page_vec.push_back(p);
                lc = 0;
            }
            strncpy(page_vec.back()->getPageCont()[lc++], s.c_str(), PAGE_W - 1);
        }
    }
}

char MorseBook::translateMorse(string code) {
    if (code == ".-") return 'A';    if (code == "-...") return 'B';
    if (code == "-.-.") return 'C';  if (code == "-..") return 'D';
    if (code == ".") return 'E';     if (code == "..-.") return 'F';
    if (code == "--.") return 'G';   if (code == "....") return 'H';
    if (code == "..") return 'I';    if (code == ".---") return 'J';
    if (code == "-.-") return 'K';   if (code == ".-..") return 'L';
    if (code == "--") return 'M';    if (code == "-.") return 'N';
    if (code == "---") return 'O';   if (code == ".--.") return 'P';
    if (code == "--.-") return 'Q';  if (code == ".-.") return 'R';
    if (code == "...") return 'S';   if (code == "-") return 'T';
    if (code == "..-") return 'U';   if (code == "...-") return 'V';
    if (code == ".--") return 'W';   if (code == "-..-") return 'X';
    if (code == "-.--") return 'Y';  if (code == "--..") return 'Z';
    
    if (code == "-----") return '0';  if (code == ".----") return '1';
    if (code == "..---") return '2';  if (code == "...--") return '3';
    if (code == "....-") return '4';  if (code == ".....") return '5';
    if (code == "-....") return '6';  if (code == "--...") return '7';
    if (code == "---..") return '8';  if (code == "----.") return '9';
    return ' ';
}

void MorseBook::readContent() {
    fstream fin("./TXT/" + filename, ios::in);
    if (!fin.is_open()) return;

    string s;
    while (getline(fin, s)) {
        if (s.find("Author:") != string::npos) break;
    }

    int lc = 0;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();

        // 檢查該行是否由摩斯密碼組成，是的話做即時翻譯附註在後方
        if (!s.empty() && (s[0] == '.' || s[0] == '-' || s[0] == ' ')) {
            string translated = "";
            stringstream ss(s);
            string token;
            while (ss >> token) {
                translated += translateMorse(token);
            }
            if (!translated.empty()) {
                s = s + " -> (" + translated + ")";
            }
        }

        if (lc == 0 || lc >= PAGE_H) {
            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
            char** c = new char*[PAGE_H];
            for (int i = 0; i < PAGE_H; i++) { 
                c[i] = new char[PAGE_W]; 
                memset(c[i], 0, PAGE_W); 
            }
            p->setPageCont(c);
            page_vec.push_back(p);
            lc = 0;
        }
        strncpy(page_vec.back()->getPageCont()[lc++], s.c_str(), PAGE_W - 1);
    }
}