#ifndef chessPiece_H
#define chessPiece_H
#include "chessBoard.h"

enum pieceType{};

class chessPiece
{
  public:
  chessPiece(pieceType piece, bool color);
  chessPiece(pieceType piece, int position[], bool color);
  void moveTo(int position[]);

  private:
  pieceType piece;
  bool color;
  int [2] position;
}

#endif