#include <iostream>
using namespace std;


enum PieceType {
  EMPTY,
  PAWN,
  KNIGHT,
  BISHOP,
  ROOK,
  QUEEN,
  KING
};


class ChessBoard;

class ChessPiece {
  public:
    PieceType piece;
    bool color;  // true = white, false = black
    int row;
    int col;
    
    ChessPiece();
    ChessPiece(PieceType p, bool pieceColor, int r, int col);
    
    void moveTo(int r, int col);
    void getPossibleMoves(ChessBoard* board, int moves[][2], int& numMoves);
};

class ChessBoard {
  public:
    ChessPiece board[8][8];
    ChessBoard();
    void reset();
    void setPiece(int row, int col, PieceType piece, bool color);
    ChessPiece getPiece(int row, int col);
    void printBoard();
};

void ChessBoard::printBoard() {
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      std::cout << board[i][j].piece << " ";
    }
  }
  std::cout << std::endl;
}

ChessPiece::ChessPiece() {
  // TODO: Initialize default piece
  piece = EMPTY;
  color = false;
  row = 0;
  col = 0;


}

ChessPiece::ChessPiece(PieceType p, bool pieceColor, int r, int col) {
  // TODO: Initialize piece with parameters
  piece = p;
  color = pieceColor;
  row = r;
  col = col;
}

void ChessPiece::moveTo(int r, int col) {
  // TODO: Move piece to new position
  row = r;
  col = col;
}

void ChessPiece::getPossibleMoves(ChessBoard* board, int moves[][2], int& numMoves) {
  // TODO: Calculate and return possible moves
  numMoves = 0;
}

ChessBoard::ChessBoard() {
  
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      board[i][j] = ChessPiece();
    }
  }
}

void ChessBoard::reset() {

  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      board[i][j] = ChessPiece();
    }
  }

  board[0][0] = ChessPiece(ROOK, false, 0, 0);
  board[0][1] = ChessPiece(KNIGHT, false, 0, 1);
  board[0][2] = ChessPiece(BISHOP, false, 0, 2);
  board[0][3] = ChessPiece(QUEEN, false, 0, 3);
  board[0][4] = ChessPiece(KING, false, 0, 4);
  board[0][5] = ChessPiece(BISHOP, false, 0, 5);
  board[0][6] = ChessPiece(KNIGHT, false, 0, 6);
  board[0][7] = ChessPiece(ROOK, false, 0, 7);
  

  for(int j = 0; j < 8; j++) {
    board[1][j] = ChessPiece(PAWN, false, 1, j);
  }

  for(int j = 0; j < 8; j++) {
    board[6][j] = ChessPiece(PAWN, true, 6, j);
  }
  
  board[7][0] = ChessPiece(ROOK, true, 7, 0);
  board[7][1] = ChessPiece(KNIGHT, true, 7, 1);
  board[7][2] = ChessPiece(BISHOP, true, 7, 2);
  board[7][3] = ChessPiece(QUEEN, true, 7, 3);
  board[7][4] = ChessPiece(KING, true, 7, 4);
  board[7][5] = ChessPiece(BISHOP, true, 7, 5);
  board[7][6] = ChessPiece(KNIGHT, true, 7, 6);
  board[7][7] = ChessPiece(ROOK, true, 7, 7);
  

}

void ChessBoard::setPiece(int row, int col, PieceType piece, bool color) {
  // TODO: Set piece at position
  board[row][col] = ChessPiece(piece, color, row, col);
}

ChessPiece ChessBoard::getPiece(int row, int col) {
  // TODO: Get piece at position
  return board[row][col];
}


ChessBoard board;

void setup() {
  
  board.reset();
}

void loop() {
  board.reset();
  
  board.printBoard();
}
