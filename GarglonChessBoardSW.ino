#include "chessBoard.h"

ChessBoard board;
bool whiteTurn = true;

int resetButtonPin = 2; // FIXME: SET THIS LATER

// --------- MUX & sensor setup ---------
// MUX SIG output to Arduino Analog Inputs
const int pinMuxSig1 = A0;
const int pinMuxSig2 = A1;
const int pinMuxSig3 = A2;
const int pinMuxSig4 = A3;

// MUX select pins S0-S3 to Arduino 8-11; THESE ARE SHARED
const int muxS0 = 8;
const int muxS1 = 9;
const int muxS2 = 10;
const int muxS3 = 11;

#define NUM_CHANNELS 16  // C0–C15

// Baseline measurement vars
unsigned long baselineStartTime;
bool baselineDone = false;

long  baselineSum[NUM_CHANNELS]     = {0};
long  baselineSamples[NUM_CHANNELS] = {0};
float baselineV[NUM_CHANNELS]       = {0};

//  Initialize mux pins
void setupMuxSelectPins() {
  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
}

//  Select mux channel  
void setMuxChannel(byte channel) {
  digitalWrite(muxS0, (channel & 0x01) ? HIGH : LOW); // LSB
  digitalWrite(muxS1, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(muxS2, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(muxS3, (channel & 0x08) ? HIGH : LOW); // MSB
}

void setup() {
  setupMusSelectPins();
  board.reset();
  pinMode(resetButtonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(resetButtonPin), boardReset, RISING);
}

void boardReset() {
  board.reset();
  whiteTurn = true;
}

void loop() {
  game();
}

void game() {
  ChessBoard board;
  board.reset();
  
  int moves[28][2];
  int numMoves;
  std::string command;
  int fromRow, fromCol, toRow, toCol;
  
  std::cout << "Commands:" << std::endl;
  std::cout << "  'check <row> <col>' - Check possible moves for piece at position" << std::endl;
  std::cout << "  'move <fromRow> <fromCol> <toRow> <toCol>' - Make a move" << std::endl;
  
  while (true) {    
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
