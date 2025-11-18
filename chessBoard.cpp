#include <iostream>
#include <cstdlib>
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

void ChessBoard::printBoard() {

  std::cout << "   ";
  for(int j = 0; j < 8; j++) {
    std::cout << j << " ";
  }
  std::cout << std::endl;
  
 
  for(int i = 0; i < 8; i++) {
    std::cout << i << " "; 
    for(int j = 0; j < 8; j++) {
      std::cout << board[i][j].piece << " ";
    }
    std::cout << std::endl;
  }
}

ChessPiece::ChessPiece() {
  piece = EMPTY;
  color = false;
  row = 0;
  col = 0;
}

ChessPiece::ChessPiece(PieceType p, bool pieceColor, int r, int col) {
  piece = p;
  color = pieceColor;
  row = r;
  col = col;
}

void ChessPiece::moveTo(int r, int col) {
  row = r;
  col = col;
}

void ChessPiece::calculateMoves(ChessBoard* board, int moves[][2], int& numMoves) {
  numMoves = 0;
  switch(piece)
  {
    case PAWN:
      {
        
        int direction = color ? -1 : 1;  
        int startRow = color ? 6 : 1;     
        
        
        int newRow = row + direction;
        if (newRow >= 0 && newRow < 8) {
        
          if (board->board[newRow][col].piece == EMPTY) {
            moves[numMoves][0] = newRow;
            moves[numMoves][1] = col;
            numMoves++;
            
            if (row == startRow) {
              int newRow2 = row + 2 * direction;
              if (newRow2 >= 0 && newRow2 < 8 && board->board[newRow2][col].piece == EMPTY) {
                moves[numMoves][0] = newRow2;
                moves[numMoves][1] = col;
                numMoves++;
              }
            }
          }
        }
        
        for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
          int newRow = row + direction;
          int newCol = col + colOffset;
          if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            ChessPiece target = board->board[newRow][newCol];
            if (target.piece != EMPTY && target.color != color) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            }
          }
        }
        
        if (board->enPassantCol >= 0 && board->enPassantColor != color) {
          int captureDirection = board->enPassantColor ? 1 : -1;
          int movedPawnRow = board->enPassantRow - captureDirection;
          
          if (row == movedPawnRow && (col == board->enPassantCol - 1 || col == board->enPassantCol + 1)) {
            int enPassantTargetRow = row + direction;
            int enPassantTargetCol = board->enPassantCol;
            if (enPassantTargetRow >= 0 && enPassantTargetRow < 8) {
              moves[numMoves][0] = enPassantTargetRow;
              moves[numMoves][1] = enPassantTargetCol;
              numMoves++;
            }
          }
        }
      }
      break;
      
    case KNIGHT:
      {

        int knightMoves[8][2] = {{-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}};
        for(int i = 0; i < 8; i++) {
          int newRow = row + knightMoves[i][0];
          int newCol = col + knightMoves[i][1];
          if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            ChessPiece target = board->board[newRow][newCol];
            if (target.piece == EMPTY || target.color != color) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            }
          }
        }
      }
      break;
      
    case BISHOP:
      {
        int directions[4][2] = {{-1,-1}, {-1,1}, {1,-1}, {1,1}};
        for(int dir = 0; dir < 4; dir++) {
          for(int dist = 1; dist < 8; dist++) {
            int newRow = row + directions[dir][0] * dist;
            int newCol = col + directions[dir][1] * dist;
            if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
            
            ChessPiece target = board->board[newRow][newCol];
            if (target.piece == EMPTY) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            } else {
              if (target.color != color) {
                moves[numMoves][0] = newRow;
                moves[numMoves][1] = newCol;
                numMoves++;
              }
              break;  
            }
          }
        }
      }
      break;
      
    case ROOK:
      {

        int directions[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
        for(int dir = 0; dir < 4; dir++) {
          for(int dist = 1; dist < 8; dist++) {
            int newRow = row + directions[dir][0] * dist;
            int newCol = col + directions[dir][1] * dist;
            if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
            
            ChessPiece target = board->board[newRow][newCol];
            if (target.piece == EMPTY) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            } else {
              if (target.color != color) {
                moves[numMoves][0] = newRow;
                moves[numMoves][1] = newCol;
                numMoves++;
              }
              break;
            }
          }
        }
      }
      break;
      
    case QUEEN:
      {

        int directions[8][2] = {{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}};
        for(int dir = 0; dir < 8; dir++) {
          for(int dist = 1; dist < 8; dist++) {
            int newRow = row + directions[dir][0] * dist;
            int newCol = col + directions[dir][1] * dist;
            if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
            
            ChessPiece target = board->board[newRow][newCol];
            if (target.piece == EMPTY) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            } else {
              if (target.color != color) {
                moves[numMoves][0] = newRow;
                moves[numMoves][1] = newCol;
                numMoves++;
              }
              break;
            }
          }
        }
      }
      break;
      
    case KING:
      {
        
        int directions[8][2] = {{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}};
        for(int dir = 0; dir < 8; dir++) {
          int newRow = row + directions[dir][0];
          int newCol = col + directions[dir][1];
          if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            ChessPiece target = board->board[newRow][newCol];
            
            if (target.piece == EMPTY || target.color != color) {
              moves[numMoves][0] = newRow;
              moves[numMoves][1] = newCol;
              numMoves++;
            }
          }
        }
      }
      break;
      
    default:
      break;
  }
}

ChessBoard::ChessBoard() {
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      board[i][j] = ChessPiece();
    }
  }
  enPassantCol = -1;
  enPassantRow = -1;
}

void ChessBoard::reset() {
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      board[i][j] = ChessPiece();
    }
  }
  enPassantCol = -1;
  enPassantRow = -1;

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
  board[row][col] = ChessPiece(piece, color, row, col);
}

ChessPiece ChessBoard::getPiece(int row, int col) {
  return board[row][col];
}

void ChessBoard::getPossibleMoves(int row, int col, int moves[][2], int& numMoves) {
  
  if (row < 0 || row >= 8 || col < 0 || col >= 8) {
    numMoves = 1;
    moves[0][0] = -1;
    moves[0][1] = -1;
    return;
  }
  

  ChessPiece piece = board[row][col];
  if (piece.piece == EMPTY) {
    numMoves = 1;
    moves[0][0] = -1;
    moves[0][1] = -1;
    return;
  }
  
  
  piece.row = row;
  piece.col = col;
  
  piece.calculateMoves(this, moves, numMoves);
}

bool ChessBoard::moveTo(int fromRow, int fromCol, int toRow, int toCol) {
  
  if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
      toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) {
    return false;  
  }
  
  
  ChessPiece sourcePiece = board[fromRow][fromCol];
  if (sourcePiece.piece == EMPTY) {
    return false;  
  }
  
  
  ChessPiece targetPiece = board[toRow][toCol];
  if (targetPiece.piece != EMPTY && targetPiece.color == sourcePiece.color) {
    return false;  
  }
  
 
  int moves[28][2];
  int numMoves;
  getPossibleMoves(fromRow, fromCol, moves, numMoves);
  

  bool isValidMove = false;
  bool isEnPassant = false;
  for(int i = 0; i < numMoves; i++) {
    if (moves[i][0] == toRow && moves[i][1] == toCol) {
      isValidMove = true;
      if (sourcePiece.piece == PAWN && enPassantCol >= 0 && 
          toCol == enPassantCol && toRow == enPassantRow) {
        isEnPassant = true;
      }
      break;
    }
  }
  
  if (!isValidMove) {
    return false;  
  }
  
  if (isEnPassant) {
    int captureDirection = sourcePiece.color ? -1 : 1;
    int capturedPawnRow = toRow - captureDirection;
    board[capturedPawnRow][enPassantCol] = ChessPiece();
  }
  
 
  int newEnPassantCol = -1;
  int newEnPassantRow = -1;
  bool newEnPassantColor = false;
  if (sourcePiece.piece == PAWN) {
    int startRow = sourcePiece.color ? 6 : 1;
    int rowDiff = toRow > fromRow ? toRow - fromRow : fromRow - toRow;
    if (fromRow == startRow && rowDiff == 2) {
      newEnPassantCol = fromCol;
      int captureDirection = sourcePiece.color ? 1 : -1;
      newEnPassantRow = toRow + captureDirection;
      newEnPassantColor = sourcePiece.color;
    }
  }
  
  enPassantCol = newEnPassantCol;
  enPassantRow = newEnPassantRow;
  enPassantColor = newEnPassantColor;
  
  if (newEnPassantCol == -1) {
    enPassantCol = -1;
    enPassantRow = -1;
  }
  
  if (sourcePiece.piece == PAWN) {
    int promotionRow = sourcePiece.color ? 0 : 7;
    if (toRow == promotionRow) {
      sourcePiece.piece = QUEEN;
    }
  }
  
  sourcePiece.moveTo(toRow, toCol);
  
 
  board[toRow][toCol] = sourcePiece;
  
  
  board[fromRow][fromCol] = ChessPiece();
  
  return true;  
}

#ifndef TEST_MODE
int main() {
  ChessBoard board;
  board.reset();
  
  bool whiteTurn = true; 
  int moves[28][2];
  int numMoves;
  std::string command;
  int fromRow, fromCol, toRow, toCol;
  
  std::cout << "Chess Game" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  'check <row> <col>' - Check possible moves for piece at position" << std::endl;
  std::cout << "  'move <fromRow> <fromCol> <toRow> <toCol>' - Make a move" << std::endl;
  std::cout << "  'board' - Show the board" << std::endl;
  std::cout << "  'reset' - Reset the board" << std::endl;
  std::cout << "  'quit' - Quit the game" << std::endl;
  std::cout << std::endl;
  
  while (true) {
    std::cout << "\n";
    board.printBoard();
    std::cout << "\nCurrent turn: " << (whiteTurn ? "White" : "Black") << std::endl;
    std::cout << "Enter command: ";
    
    std::cin >> command;
    
    if (command == "quit" || command == "q") {
      std::cout << "Thanks for playing!" << std::endl;
      break;
    }
    else if (command == "board" || command == "b") {
      
      continue;
    }
    else if (command == "reset" || command == "r") {
      board.reset();
      whiteTurn = true;
      std::cout << "Board reset!" << std::endl;
      continue;
    }
    else if (command == "check" || command == "c") {
      std::cin >> fromRow >> fromCol;
      
      if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8) {
        std::cout << "Invalid position! Row and column must be 0-7." << std::endl;
        continue;
      }
      
      board.getPossibleMoves(fromRow, fromCol, moves, numMoves);
      
      if (numMoves == 1 && moves[0][0] == -1) {
        std::cout << "No piece at position (" << fromRow << ", " << fromCol << ")" << std::endl;
      } else {
        ChessPiece piece = board.getPiece(fromRow, fromCol);
        std::cout << "Piece at (" << fromRow << ", " << fromCol << "): ";
        std::cout << (piece.color ? "White " : "Black ");
        switch(piece.piece) {
          case PAWN: std::cout << "Pawn"; break;
          case KNIGHT: std::cout << "Knight"; break;
          case BISHOP: std::cout << "Bishop"; break;
          case ROOK: std::cout << "Rook"; break;
          case QUEEN: std::cout << "Queen"; break;
          case KING: std::cout << "King"; break;
          default: std::cout << "Unknown"; break;
        }
        std::cout << std::endl;
        std::cout << "Possible moves (" << numMoves << "):" << std::endl;
        for(int i = 0; i < numMoves; i++) {
          std::cout << "  " << (i+1) << ". (" << moves[i][0] << ", " << moves[i][1] << ")" << std::endl;
        }
      }
    }
    else if (command == "move" || command == "m") {
      std::cin >> fromRow >> fromCol >> toRow >> toCol;
      
      if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
          toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) {
        std::cout << "Invalid positions! Row and column must be 0-7." << std::endl;
        continue;
      }
      
      ChessPiece sourcePiece = board.getPiece(fromRow, fromCol);
      if (sourcePiece.piece == EMPTY) {
        std::cout << "No piece at position (" << fromRow << ", " << fromCol << ")" << std::endl;
        continue;
      }
      
      if ((whiteTurn && !sourcePiece.color) || (!whiteTurn && sourcePiece.color)) {
        std::cout << "Not your turn! It's " << (whiteTurn ? "White" : "Black") << "'s turn." << std::endl;
        continue;
      }
      
     
      if (board.moveTo(fromRow, fromCol, toRow, toCol)) {
        std::cout << "Move successful!" << std::endl;
        whiteTurn = !whiteTurn;  
      } else {
        std::cout << "Invalid move! Please check the rules." << std::endl;
      }
    }
    else {
      std::cout << "Unknown command. Type 'quit' to exit." << std::endl;
      std::cin.ignore(10000, '\n');
    }
  }
  
  return 0;
}
#endif

