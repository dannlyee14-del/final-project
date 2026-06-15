#include "book.h"
#include <cmath>
#include <sstream>
#include <unistd.h>
#include <sys/select.h>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <functional>

bool Book::chineseInterface = false;

namespace {
string trimLine(string s) {
    while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

bool isFigStart(const string& s) {
    return trimLine(s) == ".fig";
}

bool isFigEnd(const string& s) {
    return trimLine(s) == ".figend";
}

void writeRowSegment(char* row, int rowWidth, int prefix, const string& text, int maxWidth) {
    int len = min(maxWidth, static_cast<int>(text.size()));
    if (prefix < 0 || prefix >= rowWidth - 1) return;
    len = min(len, rowWidth - 1 - prefix);
    if (len > 0) memcpy(row + prefix, text.c_str(), len);
    row[rowWidth - 1] = '\0';
}

string calculateInlineEquation(string eq) {
    string c = "";
    for (char ch : eq) if (ch != ' ') c += ch;
    try {
        if (c.find("sqrt") != string::npos) {
            size_t left = c.find('('), right = c.find(')', left + 1);
            return to_string((int)sqrt(stod(c.substr(left + 1, right - left - 1))));
        }
        if (c.find("power") != string::npos) {
            size_t left = c.find('('), comma = c.find(',', left + 1), right = c.find(')', comma + 1);
            double base = stod(c.substr(left + 1, comma - left - 1));
            double exp = stod(c.substr(comma + 1, right - comma - 1));
            return to_string((int)pow(base, exp));
        }
        size_t caret = c.find('^');
        if (caret != string::npos) return to_string((int)pow(stod(c.substr(0, caret)), stod(c.substr(caret + 1))));
        size_t p = c.find('+'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) + stod(c.substr(p + 1))));
        p = c.find('-'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) - stod(c.substr(p + 1))));
        p = c.find('*'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) * stod(c.substr(p + 1))));
        p = c.find('/'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) / stod(c.substr(p + 1))));
    } catch (...) {
        return "err";
    }
    return "0";
}

string resolveInlineEquations(string s) {
    size_t q1 = s.find('?');
    while (q1 != string::npos) {
        size_t q2 = s.find('?', q1 + 1);
        if (q2 == string::npos) break;
        string ans = calculateInlineEquation(s.substr(q1 + 1, q2 - q1 - 1));
        s.replace(q1, q2 - q1 + 1, ans);
        q1 = s.find('?', q1 + ans.length());
    }
    return s;
}

void layoutContentWithFigures(const string& filename,
                              vector<Page*>& pages,
                              int pageWidth,
                              int pageHeight,
                              const function<string(const string&)>& transformLine) {
    fstream fin("./Bookshelf/" + filename, ios::in);
    if (!fin.is_open()) return;
    string s;
    while (getline(fin, s)) {
        if (s.find("Author:") != string::npos) break;
    }

    int lc = 0;
    int figTop = -1, figHeight = 0, figWidth = 0;

    auto newPage = [&]() {
        Page* p = new Page(pages.size(), pageWidth, pageHeight);
        char** c = new char*[pageHeight];
        for (int i = 0; i < pageHeight; i++) {
            c[i] = new char[pageWidth];
            memset(c[i], ' ', pageWidth - 1);
            c[i][pageWidth - 1] = '\0';
        }
        p->setPageCont(c);
        pages.push_back(p);
        lc = 0;
        figTop = -1;
        figHeight = 0;
        figWidth = 0;
    };

    auto ensurePage = [&]() {
        if (pages.empty() || lc >= pageHeight) newPage();
    };

    auto putText = [&](const string& text) {
        stringstream ss(text);
        string word, line;
        if (text.empty()) {
            ensurePage();
            lc++;
            return;
        }
        while (ss >> word) {
            ensurePage();
            bool besideFigure = figTop >= 0 && lc >= figTop && lc < figTop + figHeight;
            int prefix = besideFigure ? min(pageWidth - 2, figWidth + 2) : 0;
            int maxWidth = max(1, pageWidth - prefix - 1);

            if (line.empty()) {
                line = word;
            } else if ((int)line.size() + 1 + (int)word.size() <= maxWidth) {
                line += " " + word;
            } else {
                writeRowSegment(pages.back()->getPageCont()[lc], pageWidth, prefix, line, maxWidth);
                lc++;
                line = word;
            }

            if ((int)line.size() >= maxWidth) {
                writeRowSegment(pages.back()->getPageCont()[lc], pageWidth, prefix, line, maxWidth);
                lc++;
                line.clear();
            }
        }
        if (!line.empty()) {
            ensurePage();
            bool besideFigure = figTop >= 0 && lc >= figTop && lc < figTop + figHeight;
            int prefix = besideFigure ? min(pageWidth - 2, figWidth + 2) : 0;
            int maxWidth = max(1, pageWidth - prefix - 1);
            writeRowSegment(pages.back()->getPageCont()[lc], pageWidth, prefix, line, maxWidth);
            lc++;
        }
    };

    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (isFigStart(s)) {
            vector<string> figLines;
            while (getline(fin, s)) {
                if (!s.empty() && s.back() == '\r') s.pop_back();
                if (isFigEnd(s)) break;
                figLines.push_back(s);
            }
            if (figLines.empty()) continue;
            ensurePage();
            if (figTop >= 0 && lc < figTop + figHeight) lc = figTop + figHeight;
            ensurePage();
            if ((int)figLines.size() > pageHeight) figLines.resize(pageHeight);
            if (lc + (int)figLines.size() > pageHeight) newPage();

            figTop = lc;
            figHeight = figLines.size();
            figWidth = 0;
            for (int i = 0; i < figHeight; i++) {
                int len = min((int)figLines[i].size(), pageWidth - 1);
                figWidth = max(figWidth, len);
                memcpy(pages.back()->getPageCont()[lc + i], figLines[i].c_str(), len);
            }
        } else if (!isFigEnd(s)) {
            putText(transformLine(s));
        }
    }
}
}

Book::Book(string f, string t, string a, string c)
    : filename(f), title(t), author(a), category(c), adultOnly(false) {}
Book::~Book() { for (auto p : page_vec) delete p; }
string Book::getTitle() { return title; }
string Book::getAuthor() { return author; }
string Book::getCategory() { return category; }
string Book::getFilename() { return filename; }
bool Book::isAdultOnly() const { return adultOnly; }
void Book::setAdultOnly(bool enabled) { adultOnly = enabled; }
int Book::getPageCount() {
    if (page_vec.empty()) readContent();
    return page_vec.size();
}

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

bool Book::readPageOrMenu(int& page) {
    string input;
    system("stty raw -echo");
    while (true) {
        char key = getchar();
        if (key == '\x1b') {
            char second = getchar();
            if (second == '[' || second == 'O') {
                if (getchar() == 'F') {
                    system("stty cooked echo");
                    cout << endl;
                    return false;
                }
            }
        } else if (key == '\r' || key == '\n') {
            system("stty cooked echo");
            cout << endl;
            try {
                page = stoi(input);
                return true;
            } catch (...) {
                return false;
            }
        } else if (key == 127 || key == '\b') {
            if (!input.empty()) {
                input.pop_back();
                cout << "\b \b" << flush;
            }
        } else if (isdigit(static_cast<unsigned char>(key))) {
            input += key;
            cout << key << flush;
        }
    }
}

void Book::preview() {
    int cur = 0; readContent();
    while (1) {
        system("clear");
        cout << title << "\n" << string(PAGE_W, '=') << "\n\n";
        cout << (chineseInterface ? "平均評分: " : "Average Rating: ")
             << fixed << setprecision(1) << getAverageRating()
             << " / 5 (" << getRatingCount()
             << (chineseInterface ? " 筆評分)" : " ratings)") << endl;
        cout << (chineseInterface ? "評論:" : "Comments:") << endl;
        if (comments.empty()) {
            cout << (chineseInterface ? "  目前沒有評論。" : "  No comments yet.") << endl;
        } else {
            int count = min(3, static_cast<int>(comments.size()));
            for (int i = 0; i < count; i++) {
                cout << "  - " << comments[i] << endl;
            }
        }
        cout << string(PAGE_W, '-') << "\n\n";
        if (cur < (int)page_vec.size()) page_vec[cur]->showPageCont();
        cout << "\n" << string(PAGE_W/2-4, ' ')
             << (chineseInterface ? "頁碼 " : "Page ") << cur
             << "\n" << string(PAGE_W, '=') << "\n";
        if (chineseInterface) cout << " [→]:下一頁  [←]:上一頁  [J]:跳頁  [End]:返回\n";
        else cout << " [→]:Next  [←]:Prev  [J]:Jump  [End]:Menu\n";
        char op = getKey();
        if (op == 'N' && cur < (int)page_vec.size()-1) cur++;
        else if (op == 'P' && cur > 0) cur--;
        else if (op == 'J') {
            cout << (chineseInterface ? "跳至頁碼：" : "Jump to: ");
            int t;
            if (!readPageOrMenu(t)) break;
            if (t >= 0 && t < (int)page_vec.size()) cur = t;
        } else if (op == 'M') break;
    }
}

void Book::readContent() {
    if (!page_vec.empty()) return;
    layoutContentWithFigures(filename, page_vec, PAGE_W, PAGE_H,
                             [](const string& line) {
                                 return resolveInlineEquations(line);
                             });
}

bool Book::searchContent(string q) {
    if (page_vec.empty()) readContent();
    for (auto p : page_vec) {
        for (int i=0; i<PAGE_H; i++) if (string(p->getPageCont()[i]).find(q) != string::npos) return true;
    }
    return false;
}

void MthBook::readContent() {
    if (!page_vec.empty()) return;
    layoutContentWithFigures(filename, page_vec, PAGE_W, PAGE_H,
                             [](const string& line) {
                                 return resolveInlineEquations(line);
                             });
}

string MthBook::calculateEquation(string eq) {
    string c = ""; for (char ch : eq) if (ch != ' ') c += ch;
    try {
        if (c.find("sqrt") != string::npos) {
            size_t left = c.find('('), right = c.find(')', left + 1);
            return to_string((int)sqrt(stod(c.substr(left + 1, right - left - 1))));
        }
        if (c.find("power") != string::npos) {
            size_t left = c.find('('), comma = c.find(',', left + 1), right = c.find(')', comma + 1);
            double base = stod(c.substr(left + 1, comma - left - 1));
            double exp = stod(c.substr(comma + 1, right - comma - 1));
            return to_string((int)pow(base, exp));
        }
        size_t caret = c.find('^');
        if (caret != string::npos) return to_string((int)pow(stod(c.substr(0, caret)), stod(c.substr(caret + 1))));
        size_t p = c.find('+'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) + stod(c.substr(p+1))));
        p = c.find('-'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) - stod(c.substr(p+1))));
        p = c.find('*'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) * stod(c.substr(p+1))));
        p = c.find('/'); if (p != string::npos) return to_string((int)(stod(c.substr(0, p)) / stod(c.substr(p+1))));
    } catch(...) { return "err"; }
    return "0";
}

void AniBook::preview() {
    fstream fin("./Bookshelf/" + filename, ios::in); string s;
    vector<vector<string>> frames; vector<string> cf; bool in = false;
    while (getline(fin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (isFigEnd(s)) { in=false; frames.push_back(cf); cf.clear(); }
        else if (isFigStart(s)) in=true;
        else if (in) cf.push_back(resolveInlineEquations(s));
    }
    int cur = 0; if (frames.empty()) return;
    while (1) {
        system("clear"); cout << title << "\n" << string(PAGE_W, '=') << "\n\n";
        cout << (Book::isChinese() ? "平均評分: " : "Average Rating: ")
             << fixed << setprecision(1) << getAverageRating()
             << " / 5 (" << getRatingCount()
             << (Book::isChinese() ? " 筆評分)" : " ratings)") << endl;
        cout << (Book::isChinese() ? "評論:" : "Comments:") << endl;
        if (comments.empty()) {
            cout << (Book::isChinese() ? "  目前沒有評論。" : "  No comments yet.") << endl;
        } else {
            int count = min(3, static_cast<int>(comments.size()));
            for (int i = 0; i < count; i++) {
                cout << "  - " << comments[i] << endl;
            }
        }
        cout << string(PAGE_W, '-') << "\n\n";
        for (auto l : frames[cur]) cout << l << endl;
        cout << "\n" << string(PAGE_W, '=') << "\n";
        cout << (Book::isChinese() ? " [End]: 返回選單\n" : " [End]: Back to Menu\n");
        system("stty raw -echo");
        struct timeval tv = {0, 200000}; fd_set fds; FD_ZERO(&fds); FD_SET(0, &fds);
        if (select(1, &fds, NULL, NULL, &tv) > 0) {
            char k = getchar();
            if (k == '\x1b') {
                char next_char = getchar();
                if (next_char == '\x5b' || next_char == 'O') {
                    if (getchar() == 'F') { system("stty cooked echo"); break; }
                }
            }
        }
        system("stty cooked echo"); cur = (cur+1) % frames.size();
    }
}

void MorseBook::readContent() {
    if (!page_vec.empty()) return;
    layoutContentWithFigures(filename, page_vec, PAGE_W, PAGE_H,
                             [this](const string& line) {
        string s = resolveInlineEquations(line);
        string trans = ""; stringstream ss(s); string tok;
        while (ss >> tok) {
            bool isMorse = true;
            for (char c : tok) {
                if (c != '.' && c != '-') { isMorse = false; break; }
            }
            if (isMorse) trans += translateMorse(tok);
            else {
                if (!trans.empty() && trans.back() != ' ') trans += " "; 
                trans += resolveInlineEquations(tok) + " ";
            }
        }
        return trans;
    });
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
    if (!page_vec.empty()) return;
    layoutContentWithFigures(filename, page_vec, PAGE_W, PAGE_H,
                             [](const string& line) {
                                 return resolveInlineEquations(line);
                             });
}

char** FigBook::get_figure(fstream& fin, int* h) {
    char** f = new char*[PAGE_H]; string s; int c=0;
    while (getline(fin, s) && !isFigEnd(s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        if (c < PAGE_H) {
            f[c] = new char[PAGE_W];
            strncpy(f[c], s.c_str(), PAGE_W - 1);
            f[c++][PAGE_W - 1] = '\0';
        }
    }
    *h = c; return f;
}

void FigBook::preview() {
    readContent();
    int cur = 0;
    while (true) {
        system("clear");
        cout << title << "\n" << string(PAGE_W, '=') << "\n\n";
        cout << (Book::isChinese() ? "平均評分: " : "Average Rating: ")
             << fixed << setprecision(1) << getAverageRating()
             << " / 5 (" << getRatingCount()
             << (Book::isChinese() ? " 筆評分)" : " ratings)") << endl;
        cout << (Book::isChinese() ? "評論:" : "Comments:") << endl;
        if (comments.empty()) {
            cout << (Book::isChinese() ? "  目前沒有評論。" : "  No comments yet.") << endl;
        } else {
            int count = min(3, static_cast<int>(comments.size()));
            for (int i = 0; i < count; i++) {
                cout << "  - " << comments[i] << endl;
            }
        }
        cout << string(PAGE_W, '-') << "\n\n";
        if (cur < (int)page_vec.size()) page_vec[cur]->showPageCont();
        cout << "\n" << string(PAGE_W / 2 - 4, ' ')
             << (Book::isChinese() ? "頁碼 " : "Page ") << cur
             << "\n" << string(PAGE_W, '=') << "\n";
        cout << (Book::isChinese()
            ? " [→]:下一頁  [←]:上一頁  [J]:跳頁  [End]:返回\n"
            : " [→]:Next  [←]:Prev  [J]:Jump  [End]:Menu\n");

        char op = getKey();
        if (op == 'N' && cur < (int)page_vec.size() - 1) cur++;
        else if (op == 'P' && cur > 0) cur--;
        else if (op == 'J') {
            cout << (Book::isChinese() ? "跳至頁碼：" : "Jump to: ");
            int t;
            if (!readPageOrMenu(t)) break;
            if (t >= 0 && t < (int)page_vec.size()) cur = t;
        } else if (op == 'M') break;
    }
}

void Book::addBorrowCount() {
    this->borrowCount++;
}

int Book::getBorrowCount() {
    return this->borrowCount;
}

void Book::addRating(int rating) {
    if (rating >= 1 && rating <= 5) {
        string user = "SeedUser" + to_string(ratings.size() + 1);
        ratings[user] = rating;
    }
}

void Book::setRating(const string& user, int rating) {
    if (rating >= 1 && rating <= 5) ratings[user] = rating;
}

double Book::getAverageRating() const {
    if (ratings.empty()) return 0.0;
    int total = 0;
    for (const auto& item : ratings) total += item.second;
    return static_cast<double>(total) / ratings.size();
}

int Book::getRatingCount() const {
    return ratings.size();
}

const map<string, int>& Book::getRatings() const {
    return ratings;
}

void Book::addComment(const string& user, const string& comment) {
    comments.push_back(user + ": " + comment);
}

const vector<string>& Book::getComments() const {
    return comments;
}

void Book::setChinese(bool enabled) {
    chineseInterface = enabled;
}

bool Book::isChinese() {
    return chineseInterface;
}
