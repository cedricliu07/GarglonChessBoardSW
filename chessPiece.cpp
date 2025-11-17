#include "chessPiece.h"
#include "chessBoard.h"

class chessPiece

chessPiece::chessPiece(pieceType piece, bool color) {
  this->piece = piece;
  this->color = color;
}

chessPiece::possibleMoves(chessBoard board)
{
  chessPiece[][] state = board.getBoardState();
  switch(piece)
  {
    case PAWN:
      break;
    case KNIGHT:
      break;
    case BISHOP:
      break;
    case ROOK:
      break;
    case QUEEN:
      break;
    case KING:
      break;
    default:
      board.
  }
}

chessPiece::moveTo(int position[])
{
  this->position = position;
}