#include <iostream>
#include "chessBoard.h"
#include <string>

class Board{
    public:
    char board[8][8];
    Board() {
        reset();
    }
    void reset() {
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                board[i][j] = ' ';
            }
        }
    }
    memcpy
};

int main() {
    Person person;
    person.first = "Hello";
    person.last = "World";
    person.printFullName();
    return 0;
}