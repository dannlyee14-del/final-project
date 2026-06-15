#include <bits/stdc++.h>
#include "library.h"

using namespace std;

int t = 0;

int main(){
    char opCode;
    Library lib;

    while(!lib.getExit()){
        lib.coutMainPage();
        opCode = lib.getKey();
        lib.operation(opCode);

        //t++;
        //if(t > 20) break;
    }

    return 0;
}