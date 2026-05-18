#ifndef _BOOK_H_
#define _BOOK_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>
#include "page.h"

using namespace std;

class Book {
protected:
    string filename, title, author, category;
    vector<Page*> page_vec;
    static const int PAGE_W = 100;
    static const int PAGE_H = 40;
private:
    int borrowCount=0;
public:
    Book(string filename, string title, string author, string category);
    virtual ~Book();
    virtual void readContent();
    virtual void preview();
    virtual char getKey();
    string getTitle();
    string getAuthor();
    string getCategory();
    string getFilename();
    virtual bool searchContent(string query);
    void addBorrowCount();
    int getBorrowCount() const;
};

class TxtBook : public Book {
public:
    TxtBook(string f, string t, string a, string c) : Book(f, t, a, c) {}
};

class FigBook : public Book {
public:
    FigBook(string f, string t, string a, string c) : Book(f, t, a, c) {}
    void readContent() override;
    char** get_figure(fstream& fin, int* fig_h);
};

class MthBook : public Book {
public:
    MthBook(string f, string t, string a, string c) : Book(f, t, a, c) {}
    void readContent() override;
private:
    string calculateEquation(string eq);
};

class AniBook : public Book {
public:
    AniBook(string f, string t, string a, string c) : Book(f, t, a, c) {}
    void preview() override;
};

class MorseBook : public Book {
public:
    MorseBook(string f, string t, string a, string c) : Book(f, t, a, c) {}
    void readContent() override;
private:
    char translateMorse(string code);
};

#endif