#ifndef chessBoard_H
#define chessBoard_H

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
    bool color;
    int row;
    int col;
    
    ChessPiece();
    ChessPiece(PieceType p, bool pieceColor, int r, int col);
    
    void moveTo(int r, int col);
    void calculateMoves(ChessBoard* board, int moves[][2], int& numMoves);
};

class ChessBoard {
  public:
    ChessPiece board[8][8];
    int enPassantCol;
    int enPassantRow;
    bool enPassantColor;
    ChessBoard();
    void reset();
    void setPiece(int row, int col, PieceType piece, bool color);
    ChessPiece getPiece(int row, int col);
    void printBoard();
    void getPossibleMoves(int row, int col, int moves[][2], int& numMoves);
    bool moveTo(int fromRow, int fromCol, int toRow, int toCol);
};

#endif