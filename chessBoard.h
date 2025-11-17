#ifndef chessBoard_H
#define chessBoard_H
#include "chessPiece.h"

class chessBoard
{
  public:
  chessBoard();
  bool inCheck(bool color);
  void reset();
  chessPiece[][] getBoardState();


  private:
  chessPiece[][] boardState;
}

#endif