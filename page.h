#ifndef _PAGE_H_
#define _PAGE_H_

#include <iostream>

using namespace std;

class Page {
private:
    int page_num;
    int width, height;
    char** page_cont;
public:
    Page(int num, int w, int h);
    ~Page();
    void setPageCont(char** cont);
    char** getPageCont();
    void showPageCont();
};

#endif