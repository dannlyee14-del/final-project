#include "page.h"

Page::Page(int num, int w, int h) : page_num(num), width(w), height(h) {
    page_cont = nullptr;
}

Page::~Page() {
    if (page_cont) {
        for (int i = 0; i < height; i++) delete[] page_cont[i];
        delete[] page_cont;
    }
}

void Page::setPageCont(char** cont) { page_cont = cont; }
char** Page::getPageCont() { return page_cont; }

void Page::showPageCont() {
    if (!page_cont) return;
    for (int i = 0; i < height; i++) {
        if (page_cont[i]) std::cout << page_cont[i] << std::endl;
    }
}