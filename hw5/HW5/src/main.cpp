#include <bits/stdc++.h>
#include "Legalizer.hpp"



int main(int argc, char *argv[]) {
    if(argc != 3) {
        std::cout<<"[Usage]: ./hw5 in.txt out.out\n";
        exit(-1);
    }

    // Legalizer
    Legalizer legalizer(argv[1], argv[2]);

    return 0;
}