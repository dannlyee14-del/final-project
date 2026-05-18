// ===== 跨平臺系統資源處理 =====
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN  // 阻擋 Windows 載入包含 'byte' 衝突的冗餘標頭檔
#include <windows.h>
#include <conio.h>
#define CLEAR_CMD "cls"
#else
#include <unistd.h>
#include <sys/select.h>
#define CLEAR_CMD "clear"
#endif
// ===================================

#include "book.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>

Book::Book(string f, string t, string a, string c) : filename(f), title(t), author(a), category(c) {}
Book::~Book() { for (auto p : page_vec) delete p; }
string Book::getTitle() { return title; }
string Book::getAuthor() { return author; }
string Book::getCategory() { return category; }
string Book::getFilename() { return filename; }

char Book::getKey() {
    char key;
    system("stty raw -echo");
    key = getchar();
    if (key == '\x1b') {
        char next_char = getchar();
        if (next_char == '\x5b' || next_char == 'O') {
            char d = getchar();
            system("stty cooked echo");
            if (d == 'C') return 'N'; // Right
            if (d == 'D') return 'P'; // Left
            if (d == 'F') return 'M'; // End
            return ' ';
        }
    }
    system("stty cooked echo");
    return (toupper(key) == 'J') ? 'J' : ' ';
}
void Book::preview() {
    int cur = 0; readContent();
    while (1) {
        system(CLEAR_CMD);
        cout << title << "\n" << string(PAGE_W, '=') << "\n\n";
        if (cur < (int)page_vec.size()) page_vec[cur]->showPageCont();
        cout << "\n" << string(PAGE_W/2-4, ' ') << "Page " << cur << "\n" << string(PAGE_W, '=') << "\n";
        cout << " [→]:Next  [←]:Prev  [J]:Jump  [End]:Menu\n";
        char op = getKey();
        if (op == 'N' && cur < (int)page_vec.size()-1) cur++;
        else if (op == 'P' && cur > 0) cur--;
        else if (op == 'J') {
            cout << "Jump to: "; int t; cin >> t;
            if (t >= 0 && t < (int)page_vec.size()) cur = t;
        } else if (op == 'M') break;
    }
}

void Book::readContent() {
    fstream fin("./TXT/" + filename, ios::in);
    if (!fin.is_open()) return;
    string s; while (getline(fin, s)) if (s.find("Author:") != string::npos) break;
    int lc = 0;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (lc == 0 || lc >= PAGE_H) {
            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
            char** c = new char*[PAGE_H];
            for (int i=0; i<PAGE_H; i++) { c[i] = new char[PAGE_W]; c[i][0]='\0'; }
            p->setPageCont(c); page_vec.push_back(p); lc = 0;
        }
        strncpy(page_vec.back()->getPageCont()[lc++], s.c_str(), PAGE_W-1);
    }
}

bool Book::searchContent(string q) {
    if (page_vec.empty()) readContent();
    for (auto p : page_vec) {
        for (int i=0; i<PAGE_H; i++) if (string(p->getPageCont()[i]).find(q) != string::npos) return true;
    }
    return false;
}

void MthBook::readContent() {
    fstream fin("./TXT/" + filename, ios::in);
    string s; while (getline(fin, s)) if (s.find("Author:") != string::npos) break;
    int lc = 0;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        size_t q1 = s.find('?');
        while (q1 != string::npos) {
            size_t q2 = s.find('?', q1+1);
            if (q2 != string::npos) {
                string ans = calculateEquation(s.substr(q1+1, q2-q1-1));
                s.replace(q1, q2-q1+1, ans);
                q1 = s.find('?', q1+ans.length());
            } else break;
        }
        if (lc == 0 || lc >= PAGE_H) {
            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
            char** c = new char*[PAGE_H];
            for (int i=0; i<PAGE_H; i++) { c[i]=new char[PAGE_W]; c[i][0]='\0'; }
            p->setPageCont(c); page_vec.push_back(p); lc = 0;
        }
        strncpy(page_vec.back()->getPageCont()[lc++], s.c_str(), PAGE_W-1);
    }
}

string MthBook::calculateEquation(string eq) {
    string c = ""; for (char ch : eq) if (ch != ' ') c += ch;
    try {
        if (c.find("sqrt") != string::npos) return to_string((int)sqrt(stod(c.substr(c.find('(')+1))));
        size_t p = c.find('+'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) + stod(c.substr(p+1))));
        p = c.find('-'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) - stod(c.substr(p+1))));
        p = c.find('*'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) * stod(c.substr(p+1))));
        p = c.find('/'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) / stod(c.substr(p+1))));
    } catch(...) { return "err"; }
    return "0";
}

void AniBook::preview() {
    fstream fin("./TXT/" + filename, ios::in); string s;
    vector<vector<string>> frames; vector<string> cf; bool in = false;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (s.find(".figend") != string::npos) { in=false; frames.push_back(cf); cf.clear(); }
        else if (s.find(".fig") != string::npos) in=true;
        else if (in) cf.push_back(s);
    }
    int cur = 0; if (frames.empty()) return;
    while (1) {
        system(CLEAR_CMD); 
        cout << title << "\n" << string(PAGE_W, '=') << "\n\n";
        for (auto l : frames[cur]) cout << l << endl;
        cout << "\n" << string(PAGE_W, '=') << "\n [End]: Back to Menu\n";

#if defined(_WIN32) || defined(_WIN64)
        Sleep(200); 
        if (_kbhit()) {
            int k = _getch();
            if (k == 0 || k == 224) {
                if (_getch() == 79) break; 
            }
        }
#else
        system("stty raw -echo");
        struct timeval tv = {0, 200000}; fd_set fds; FD_ZERO(&fds); FD_SET(0, &fds);
        if (select(1, &fds, NULL, NULL, &tv) > 0) {
            char k = getchar();
            if (k == '\x1b') {
                if (getchar() == '\x5b' && getchar() == 'F') { system("stty cooked echo"); break; }
            }
        }
        system("stty cooked echo"); 
#endif
        cur = (cur+1) % frames.size();
    }
}

void MorseBook::readContent() {
    fstream fin("./TXT/" + filename, ios::in); string s;
    while (getline(fin, s)) if (s.find("Author:") != string::npos) break;
    int lc = 0;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        string trans = ""; stringstream ss(s); string tok;
        while (ss >> tok) {
            bool isMorse = true;
            for (char c : tok) {
                if (c != '.' && c != '-') { isMorse = false; break; }
            }
            if (isMorse) trans += translateMorse(tok);
            else {
                if (!trans.empty() && trans.back() != ' ') trans += " "; 
                trans += tok + " ";
            }
        }
        
        int start = 0;
        while (start < (int)trans.length() || trans.empty()) {
            if (lc == 0 || lc >= PAGE_H) {
                Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
                char** c = new char*[PAGE_H];
                for (int i = 0; i < PAGE_H; i++) { 
                    c[i] = new char[PAGE_W]; 
                    memset(c[i], 0, PAGE_W); 
                }
                p->setPageCont(c); page_vec.push_back(p); lc = 0;
            }
            string sub = trans.substr(start, PAGE_W - 1);
            strncpy(page_vec.back()->getPageCont()[lc], sub.c_str(), PAGE_W - 1);
            page_vec.back()->getPageCont()[lc][PAGE_W - 1] = '\0';
            lc++;
            start += PAGE_W - 1;
            if (trans.empty()) break; 
        }
    }
}

char MorseBook::translateMorse(string code) {
    if (code == ".-") return 'A'; if (code == "-...") return 'B'; if (code == "-.-.") return 'C';
    if (code == "-..") return 'D'; if (code == ".") return 'E'; if (code == "..-.") return 'F';
    if (code == "--.") return 'G'; if (code == "....") return 'H'; if (code == "..") return 'I';
    if (code == ".---") return 'J'; if (code == "-.-") return 'K'; if (code == ".-..") return 'L';
    if (code == "--") return 'M'; if (code == "-.") return 'N'; if (code == "---") return 'O';
    if (code == ".--.") return 'P'; if (code == "--.-") return 'Q'; if (code == ".-.") return 'R';
    if (code == "...") return 'S'; if (code == "-") return 'T'; if (code == "..-") return 'U';
    if (code == "...-") return 'V'; if (code == ".--") return 'W'; if (code == "-..-") return 'X';
    if (code == "-.--") return 'Y'; if (code == "--..") return 'Z';if (code == "-----") return '0';
    if (code == ".----") return '1'; if (code == "..---") return '2'; if (code == "...--") return '3';
    if (code == "....-") return '4'; if (code == ".....") return '5'; if (code == "-....") return '6';
    if (code == "--...") return '7'; if (code == "---..") return '8'; if (code == "----.") return '9';
    return '?';
}

void FigBook::readContent() {
    fstream fin("./TXT/" + filename, ios::in); string s;
    while (getline(fin, s)) if (s.find("Author:") != string::npos) break;
    int lc = 0;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (lc == 0 || lc >= PAGE_H) {
            Page* p = new Page(page_vec.size(), PAGE_W, PAGE_H);
            char** c = new char*[PAGE_H];
            for (int i=0; i<PAGE_H; i++) { c[i]=new char[PAGE_W]; c[i][0]='\0'; }
            p->setPageCont(c); page_vec.push_back(p); lc = 0;
        }
        if (s.find(".fig") != string::npos) {
            int fh = 0; char** fig = get_figure(fin, &fh);
            if (lc + fh >= PAGE_H) { lc = PAGE_H; continue; }
            for (int i=0; i<fh; i++) strcpy(page_vec.back()->getPageCont()[lc++], fig[i]);
            for (int i=0; i<fh; i++) delete[] fig[i]; delete[] fig;
        } else strncpy(page_vec.back()->getPageCont()[lc++], s.c_str(), PAGE_W-1);
    }
}

char** FigBook::get_figure(fstream& fin, int* h) {
    char** f = new char*[PAGE_H]; string s; int c=0;
    while (getline(fin, s) && s.find(".figend")==string::npos) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        f[c] = new char[PAGE_W]; strncpy(f[c], s.c_str(), PAGE_W-1); f[c++][PAGE_W-1]='\0';
    }
    *h = c; return f;
}

void Book::addBorrowCount() {
    this->borrowCount++;
}

int Book::getBorrowCount() {
    return this->borrowCount;
}