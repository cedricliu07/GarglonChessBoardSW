#include <Adafruit_NeoPixel.h>

// --------- Shared MUX select pins ---------
const int muxS0 = 10, muxS1 = 11, muxS2 = 12, muxS3 = 13;

// --------- 4 MUX board config ---------
#define MUX_COUNT     4
#define CH_PER_MUX    16
#define TOTAL_SENSORS (MUX_COUNT * CH_PER_MUX)  // 64

const int muxSigPins[MUX_COUNT] = {A0, A1, A2, A3};

// --------- WS2812B LED strip setup ---------
#define LED_PIN   3
#define NUM_LEDS  64
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --------- Baseline measurement vars ---------
bool baselineDone = false;
const uint8_t BASELINE_LOOPS = 16;  // how many full scans to average
uint8_t baselineLoops = 0;

uint16_t baselineSum[TOTAL_SENSORS] = {0}; // sum of raw readings
uint16_t baselineMV[TOTAL_SENSORS]  = {0}; // baseline in mV

// --------- Threshold + persistence ---------
bool stableTrig[TOTAL_SENSORS]   = {0};
bool instantTrig[TOTAL_SENSORS]  = {0};
byte onCount[TOTAL_SENSORS]      = {0};
byte offCount[TOTAL_SENSORS]     = {0};

// Make ON stronger than OFF for stability
const byte ON_CYCLES  = 8;
const byte OFF_CYCLES = 3;

// Default thresholds (in mV)
const uint16_t HIGH_THRESH_MV_DEFAULT = 10;  // for all normal squares
const uint16_t LOW_THRESH_MV          = 10;  // common low threshold

// --------- Previous stable state + highlight state ---------
bool stablePrev[TOTAL_SENSORS] = {0};
bool showingMoves = false;
int  liftedRow = -1, liftedCol = -1;

// ---------- Capture lock state ----------
bool captureLocked = false;
int  captureRow = -1, captureCol = -1;

// ---------- Compact chess types (1 byte each) ----------
typedef uint8_t PieceType;
typedef uint8_t Side;

const PieceType PIECE_NONE   = 0;
const PieceType PIECE_PAWN   = 1;
const PieceType PIECE_ROOK   = 2;
const PieceType PIECE_KNIGHT = 3;
const PieceType PIECE_BISHOP = 4;
const PieceType PIECE_QUEEN  = 5;
const PieceType PIECE_KING   = 6;

const Side SIDE_NONE  = 0;
const Side SIDE_WHITE = 1;
const Side SIDE_BLACK = 2;

// ---------- Logical board state ----------
PieceType boardPiece[8][8];
Side      boardSide[8][8];
bool      boardHasMoved[8][8];

// Currently lifted piece (in the air)
PieceType liftedPieceType = PIECE_NONE;
Side      liftedSide      = SIDE_NONE;
bool      liftedHasMoved  = false;
int       liftedStartRow  = -1;
int       liftedStartCol  = -1;

// Whose turn is it?
Side currentTurn = SIDE_WHITE;

// ---------- Problem (drifty) square ----------
// Square in front of the black king pawn: row 2, col 4.
// Change these if your orientation is different.
const int DRIFT_ROW = 2;
const int DRIFT_COL = 4;
int DRIFT_IDX = -1;  // will be set in setup()

// ---------- MUX + mapping helpers ----------
void setupMuxSelectPins() {
  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
}

void setMuxChannel(byte channel) {
  digitalWrite(muxS0, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(muxS1, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(muxS2, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(muxS3, (channel & 0x08) ? HIGH : LOW);
}

inline int idxOf(int mux, int ch) { return mux * CH_PER_MUX + ch; }

// logical (row,col) -> sensor idx
inline int idxFromRowCol(int row, int col) {
  int mux = row / 2;
  bool secondRowInMux = (row & 1);
  int ch = secondRowInMux ? (8 + col) : col;
  return idxOf(mux, ch);
}

// sensor idx -> (row,col)
inline void rowColFromIdx(int idx, int &row, int &col) {
  int mux = idx / 16;
  int ch  = idx % 16;
  row = mux * 2 + (ch >= 8 ? 1 : 0);
  col = ch & 7;
}

// physical LED mapping (your bottom-left LED0 zigzag)
int ledIndex(int row, int col) {
  int pr = 7 - row;   // physical row from bottom
  int base = pr * 8;
  return (pr % 2 == 0) ? (base + col) : (base + (7 - col));
}

// ---------- Color wheel helper for rainbow ----------
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// ---------- Rainbow during calibration ----------
void showCalibratingRainbow() {
  static uint32_t lastUpdate = 0;
  static uint8_t shift = 0;

  if (millis() - lastUpdate < 5) return;
  lastUpdate = millis();

  shift++;  // move pattern down one row each step

  for (int row = 0; row < 8; row++) {
    uint8_t hue = ((row + shift) * 32) & 255;
    uint32_t c = Wheel(hue);

    for (int col = 0; col < 8; col++) {
      int led = ledIndex(row, col);
      strip.setPixelColor(led, c);
    }
  }

  strip.show();
}

// ---------- Rainbow on checkmate ----------
void showCheckmateRainbow() {
  for (int t = 0; t < 200; t++) { // ~1 second (200 * 5ms)
    uint8_t shift = t;
    for (int row = 0; row < 8; row++) {
      uint8_t hue = ((row + shift) * 32) & 255;
      uint32_t c = Wheel(hue);
      for (int col = 0; col < 8; col++) {
        int led = ledIndex(row, col);
        strip.setPixelColor(led, c);
      }
    }
    strip.show();
    delay(5);
  }
}

// ---------- Green flash after calibration ----------
void flashGreen() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  delay(300);

  strip.clear();
  strip.show();
}

// ---------- Chess helpers ----------

bool isOnBoard(int r, int c) {
  return (r >= 0 && r < 8 && c >= 0 && c < 8);
}

// Use logical board occupancy for move blocking
bool isOccupied(int row, int col) {
  return (boardPiece[row][col] != PIECE_NONE);
}

void highlightSquare(int row, int col) {
  if (!isOnBoard(row, col)) return;
  int led = ledIndex(row, col);
  // blue for normal move
  strip.setPixelColor(led, strip.Color(0, 0, 255));
}

void highlightCaptureSquare(int row, int col) {
  if (!isOnBoard(row, col)) return;
  int led = ledIndex(row, col);
  // green for captures
  strip.setPixelColor(led, strip.Color(0, 255, 0));
}

// ---------- Movement / capture legality ----------

// Check clear path for rook-like move
bool clearPathRook(int fr, int fc, int tr, int tc) {
  if (fr != tr && fc != tc) return false;
  int dr = (tr > fr) ? 1 : (tr < fr ? -1 : 0);
  int dc = (tc > fc) ? 1 : (tc < fc ? -1 : 0);
  int r = fr + dr;
  int c = fc + dc;
  while (r != tr || c != tc) {
    if (boardPiece[r][c] != PIECE_NONE) return false;
    r += dr;
    c += dc;
  }
  return true;
}

// Check clear path for bishop-like move
bool clearPathBishop(int fr, int fc, int tr, int tc) {
  int dr = tr - fr;
  int dc = tc - fc;
  if (abs(dr) != abs(dc)) return false;
  int sr = (dr > 0) ? 1 : -1;
  int sc = (dc > 0) ? 1 : -1;
  int r = fr + sr;
  int c = fc + sc;
  while (r != tr || c != tc) {
    if (boardPiece[r][c] != PIECE_NONE) return false;
    r += sr;
    c += sc;
  }
  return true;
}

// True if lifted piece *could* capture (tr,tc) given current board
// (used for the "capture lock" behavior)
bool canPieceCaptureSquare(PieceType pt, Side side,
                           int fr, int fc, int tr, int tc) {
  if (!isOnBoard(tr, tc)) return false;
  if (side == SIDE_NONE || pt == PIECE_NONE) return false;
  // must be an enemy piece logically on that square
  if (boardSide[tr][tc] == SIDE_NONE || boardSide[tr][tc] == side) return false;

  int dr = tr - fr;
  int dc = tc - fc;
  int adr = abs(dr);
  int adc = abs(dc);

  switch (pt) {
    case PIECE_PAWN: {
      int dir = (side == SIDE_WHITE) ? -1 : +1;
      if (dr == dir && (dc == 1 || dc == -1)) {
        return true;
      }
      return false;
    }
    case PIECE_KNIGHT:
      if ((adr == 2 && adc == 1) || (adr == 1 && adc == 2)) return true;
      return false;

    case PIECE_KING:
      if (adr <= 1 && adc <= 1) return true;
      return false;

    case PIECE_ROOK:
      if (!clearPathRook(fr, fc, tr, tc)) return false;
      return true;

    case PIECE_BISHOP:
      if (!clearPathBishop(fr, fc, tr, tc)) return false;
      return true;

    case PIECE_QUEEN:
      if (fr == tr || fc == tc) {
        if (!clearPathRook(fr, fc, tr, tc)) return false;
        return true;
      }
      if (adr == adc) {
        if (!clearPathBishop(fr, fc, tr, tc)) return false;
        return true;
      }
      return false;

    default:
      return false;
  }
}

// Sliding move helper: empty = blue, enemy = green (then stop), own = stop
void showRayMoves(int row, int col, int dRow, int dCol, Side side) {
  int r = row + dRow;
  int c = col + dCol;

  while (isOnBoard(r, c)) {
    if (!isOccupied(r, c)) {
      highlightSquare(r, c);   // empty square
    } else {
      if (boardSide[r][c] != side && boardSide[r][c] != SIDE_NONE) {
        highlightCaptureSquare(r, c);  // enemy piece
      }
      break; // stop either way
    }
    r += dRow;
    c += dCol;
  }
}

// Pawn moves depend on side and hasMoved (for double-step + captures)
void showPawnMoves(int row, int col, Side side, bool hasMoved) {
  if (side == SIDE_NONE) return;

  int dir      = (side == SIDE_WHITE) ? -1 : +1;   // white moves up, black down
  int startRow = (side == SIDE_WHITE) ?  6 :  1;   // starting rank

  int r1 = row + dir;
  int r2 = row + 2 * dir;

  // 1-step forward if empty
  if (isOnBoard(r1, col) && !isOccupied(r1, col)) {
    highlightSquare(r1, col);

    // 2-step only if never moved AND on starting rank AND both empty
    if (!hasMoved && row == startRow &&
        isOnBoard(r2, col) && !isOccupied(r2, col)) {
      highlightSquare(r2, col);
    }
  }

  // diagonal captures (one step forward + left/right)
  int cLeft  = col - 1;
  int cRight = col + 1;
  int rCap   = row + dir;

  if (isOnBoard(rCap, cLeft) && isOccupied(rCap, cLeft) &&
      boardSide[rCap][cLeft] != side && boardSide[rCap][cLeft] != SIDE_NONE) {
    highlightCaptureSquare(rCap, cLeft);
  }
  if (isOnBoard(rCap, cRight) && isOccupied(rCap, cRight) &&
      boardSide[rCap][cRight] != side && boardSide[rCap][cRight] != SIDE_NONE) {
    highlightCaptureSquare(rCap, cRight);
  }
}

void showRookMoves(int row, int col, Side side) {
  showRayMoves(row, col, -1,  0, side);
  showRayMoves(row, col,  1,  0, side);
  showRayMoves(row, col,  0, -1, side);
  showRayMoves(row, col,  0,  1, side);
}

void showBishopMoves(int row, int col, Side side) {
  showRayMoves(row, col, -1, -1, side);
  showRayMoves(row, col, -1,  1, side);
  showRayMoves(row, col,  1, -1, side);
  showRayMoves(row, col,  1,  1, side);
}

void showQueenMoves(int row, int col, Side side) {
  showRookMoves(row, col, side);
  showBishopMoves(row, col, side);
}

void showKnightMoves(int row, int col, Side side) {
  const int dr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
  const int dc[8] = {-1,  1, -2,  2,-2, 2,-1, 1};

  for (int i = 0; i < 8; i++) {
    int r = row + dr[i];
    int c = col + dc[i];
    if (!isOnBoard(r, c)) continue;

    if (!isOccupied(r, c)) {
      highlightSquare(r, c);  // empty
    } else if (boardSide[r][c] != side && boardSide[r][c] != SIDE_NONE) {
      highlightCaptureSquare(r, c);  // capture
    }
  }
}

void showKingMoves(int row, int col, Side side) {
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) continue;
      int r = row + dr;
      int c = col + dc;
      if (!isOnBoard(r, c)) continue;

      if (!isOccupied(r, c)) {
        highlightSquare(r, c);
      } else if (boardSide[r][c] != side && boardSide[r][c] != SIDE_NONE) {
        highlightCaptureSquare(r, c);
      }
    }
  }
}

void showMovesForLiftedPiece() {
  if (liftedRow == -1 || liftedCol == -1) return;
  if (liftedPieceType == PIECE_NONE || liftedSide == SIDE_NONE) return;

  switch (liftedPieceType) {
    case PIECE_PAWN:
      showPawnMoves(liftedRow, liftedCol, liftedSide, liftedHasMoved);
      break;
    case PIECE_ROOK:
      showRookMoves(liftedRow, liftedCol, liftedSide);
      break;
    case PIECE_BISHOP:
      showBishopMoves(liftedRow, liftedCol, liftedSide);
      break;
    case PIECE_QUEEN:
      showQueenMoves(liftedRow, liftedCol, liftedSide);
      break;
    case PIECE_KNIGHT:
      showKnightMoves(liftedRow, liftedCol, liftedSide);
      break;
    case PIECE_KING:
      showKingMoves(liftedRow, liftedCol, liftedSide);
      break;
    default:
      break;
  }
}

// ---------- EXTRA: check / checkmate logic ----------

Side oppositeSide(Side s) {
  if (s == SIDE_WHITE) return SIDE_BLACK;
  if (s == SIDE_BLACK) return SIDE_WHITE;
  return SIDE_NONE;
}

// Like canPieceCaptureSquare, but does NOT require a piece on target square.
// Used to see if a square is attacked.
bool canPieceAttackSquare(PieceType pt, Side side,
                          int fr, int fc, int tr, int tc) {
  if (!isOnBoard(tr, tc)) return false;
  if (side == SIDE_NONE || pt == PIECE_NONE) return false;

  int dr = tr - fr;
  int dc = tc - fc;
  int adr = abs(dr);
  int adc = abs(dc);

  switch (pt) {
    case PIECE_PAWN: {
      int dir = (side == SIDE_WHITE) ? -1 : +1;
      if (dr == dir && (dc == 1 || dc == -1)) return true;
      return false;
    }
    case PIECE_KNIGHT:
      if ((adr == 2 && adc == 1) || (adr == 1 && adc == 2)) return true;
      return false;

    case PIECE_KING:
      if (adr <= 1 && adc <= 1) return true;
      return false;

    case PIECE_ROOK:
      if (!clearPathRook(fr, fc, tr, tc)) return false;
      return true;

    case PIECE_BISHOP:
      if (!clearPathBishop(fr, fc, tr, tc)) return false;
      return true;

    case PIECE_QUEEN:
      if (fr == tr || fc == tc) {
        if (!clearPathRook(fr, fc, tr, tc)) return false;
        return true;
      }
      if (adr == adc) {
        if (!clearPathBishop(fr, fc, tr, tc)) return false;
        return true;
      }
      return false;

    default:
      return false;
  }
}

// Is square (row,col) attacked by any piece of "attacker" side?
bool isSquareAttackedBy(int row, int col, Side attacker) {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (boardSide[r][c] == attacker) {
        PieceType pt = boardPiece[r][c];
        if (canPieceAttackSquare(pt, attacker, r, c, row, col)) {
          return true;
        }
      }
    }
  }
  return false;
}

// Is "side" currently in check?
bool isKingInCheck(Side side) {
  if (side == SIDE_NONE) return false;
  // find king
  int kr = -1, kc = -1;
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (boardPiece[r][c] == PIECE_KING && boardSide[r][c] == side) {
        kr = r;
        kc = c;
        break;
      }
    }
    if (kr != -1) break;
  }
  if (kr == -1) return false; // king missing -> treat as not in check

  Side opp = oppositeSide(side);
  if (opp == SIDE_NONE) return false;

  return isSquareAttackedBy(kr, kc, opp);
}

// Try a hypothetical move and see if it leaves "side" not in check.
// Origin (fr,fc) -> dest (tr,tc).
bool tryMoveLegal(Side side, int fr, int fc, int tr, int tc) {
  if (!isOnBoard(tr, tc)) return false;
  if (boardSide[fr][fc] != side) return false;
  if (boardSide[tr][tc] == side) return false; // can't capture own piece

  PieceType movingPiece = boardPiece[fr][fc];
  bool      movingMoved = boardHasMoved[fr][fc];

  PieceType destPiece   = boardPiece[tr][tc];
  Side      destSide    = boardSide[tr][tc];
  bool      destMoved   = boardHasMoved[tr][tc];

  // apply move
  boardPiece[fr][fc]    = PIECE_NONE;
  boardSide[fr][fc]     = SIDE_NONE;
  boardHasMoved[fr][fc] = false;

  boardPiece[tr][tc]    = movingPiece;
  boardSide[tr][tc]     = side;
  boardHasMoved[tr][tc] = true;

  bool ok = !isKingInCheck(side);

  // undo move
  boardPiece[fr][fc]    = movingPiece;
  boardSide[fr][fc]     = side;
  boardHasMoved[fr][fc] = movingMoved;

  boardPiece[tr][tc]    = destPiece;
  boardSide[tr][tc]     = destSide;
  boardHasMoved[tr][tc] = destMoved;

  return ok;
}

// Does "side" have any legal move?
bool hasAnyLegalMove(Side side) {
  if (side == SIDE_NONE) return false;

  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (boardSide[r][c] != side) continue;
      PieceType pt = boardPiece[r][c];

      int dir, startRow;
      switch (pt) {
        case PIECE_PAWN: {
          dir      = (side == SIDE_WHITE) ? -1 : +1;
          startRow = (side == SIDE_WHITE) ?  6 :  1;
          // forward 1
          int r1 = r + dir;
          if (isOnBoard(r1, c) && !isOccupied(r1, c)) {
            if (tryMoveLegal(side, r, c, r1, c)) return true;
            // forward 2 from start
            int r2 = r + 2 * dir;
            if (r == startRow && isOnBoard(r2, c) && !isOccupied(r2, c)) {
              if (tryMoveLegal(side, r, c, r2, c)) return true;
            }
          }
          // captures
          int cL = c - 1, cR = c + 1;
          int rCap = r + dir;
          if (isOnBoard(rCap, cL) && boardSide[rCap][cL] == oppositeSide(side)) {
            if (tryMoveLegal(side, r, c, rCap, cL)) return true;
          }
          if (isOnBoard(rCap, cR) && boardSide[rCap][cR] == oppositeSide(side)) {
            if (tryMoveLegal(side, r, c, rCap, cR)) return true;
          }
          break;
        }

        case PIECE_KNIGHT: {
          const int dr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
          const int dc[8] = {-1,  1, -2,  2,-2, 2,-1, 1};
          for (int i = 0; i < 8; i++) {
            int rr = r + dr[i];
            int cc = c + dc[i];
            if (!isOnBoard(rr, cc)) continue;
            if (boardSide[rr][cc] == side) continue;
            if (tryMoveLegal(side, r, c, rr, cc)) return true;
          }
          break;
        }

        case PIECE_KING: {
          for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
              if (dr == 0 && dc == 0) continue;
              int rr = r + dr;
              int cc = c + dc;
              if (!isOnBoard(rr, cc)) continue;
              if (boardSide[rr][cc] == side) continue;
              if (tryMoveLegal(side, r, c, rr, cc)) return true;
            }
          }
          break;
        }

        case PIECE_ROOK: {
          const int dr[4] = {-1, 1,  0, 0};
          const int dc[4] = { 0, 0, -1, 1};
          for (int d = 0; d < 4; d++) {
            int rr = r + dr[d];
            int cc = c + dc[d];
            while (isOnBoard(rr, cc)) {
              if (boardSide[rr][cc] == side) break;
              if (tryMoveLegal(side, r, c, rr, cc)) return true;
              if (boardSide[rr][cc] != SIDE_NONE) break; // hit enemy
              rr += dr[d];
              cc += dc[d];
            }
          }
          break;
        }

        case PIECE_BISHOP: {
          const int dr[4] = {-1,-1, 1, 1};
          const int dc[4] = {-1, 1,-1, 1};
          for (int d = 0; d < 4; d++) {
            int rr = r + dr[d];
            int cc = c + dc[d];
            while (isOnBoard(rr, cc)) {
              if (boardSide[rr][cc] == side) break;
              if (tryMoveLegal(side, r, c, rr, cc)) return true;
              if (boardSide[rr][cc] != SIDE_NONE) break;
              rr += dr[d];
              cc += dc[d];
            }
          }
          break;
        }

        case PIECE_QUEEN: {
          const int dr[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
          const int dc[8] = {-1, 0, 1,-1, 1,-1, 0, 1};
          for (int d = 0; d < 8; d++) {
            int rr = r + dr[d];
            int cc = c + dc[d];
            while (isOnBoard(rr, cc)) {
              if (boardSide[rr][cc] == side) break;
              if (tryMoveLegal(side, r, c, rr, cc)) return true;
              if (boardSide[rr][cc] != SIDE_NONE) break;
              rr += dr[d];
              cc += dc[d];
            }
          }
          break;
        }

        default:
          break;
      }
    }
  }
  return false;
}

// Call after a move is fully applied to the board
void handlePostMoveCheckmate() {
  // Switch turn
  currentTurn = oppositeSide(currentTurn);

  // Check side to move
  if (isKingInCheck(currentTurn)) {
    if (!hasAnyLegalMove(currentTurn)) {
      // CHECKMATE!
      showCheckmateRainbow();
    }
  }
}

// ---------- Logical board init: standard chess position ----------

void initStandardBoard() {
  // clear everything first
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      boardPiece[r][c]    = PIECE_NONE;
      boardSide[r][c]     = SIDE_NONE;
      boardHasMoved[r][c] = false;
    }
  }

  // White pieces at bottom (rows 7 and 6)
  // Back rank row 7
  boardPiece[7][0] = PIECE_ROOK;   boardSide[7][0] = SIDE_WHITE;
  boardPiece[7][1] = PIECE_KNIGHT; boardSide[7][1] = SIDE_WHITE;
  boardPiece[7][2] = PIECE_BISHOP; boardSide[7][2] = SIDE_WHITE;
  boardPiece[7][3] = PIECE_QUEEN;  boardSide[7][3] = SIDE_WHITE;
  boardPiece[7][4] = PIECE_KING;   boardSide[7][4] = SIDE_WHITE;
  boardPiece[7][5] = PIECE_BISHOP; boardSide[7][5] = SIDE_WHITE;
  boardPiece[7][6] = PIECE_KNIGHT; boardSide[7][6] = SIDE_WHITE;
  boardPiece[7][7] = PIECE_ROOK;   boardSide[7][7] = SIDE_WHITE;

  // Pawns row 6
  for (int c = 0; c < 8; c++) {
    boardPiece[6][c] = PIECE_PAWN;
    boardSide[6][c]  = SIDE_WHITE;
  }

  // Black pieces at top (rows 0 and 1)
  // Back rank row 0
  boardPiece[0][0] = PIECE_ROOK;   boardSide[0][0] = SIDE_BLACK;
  boardPiece[0][1] = PIECE_KNIGHT; boardSide[0][1] = SIDE_BLACK;
  boardPiece[0][2] = PIECE_BISHOP; boardSide[0][2] = SIDE_BLACK;
  boardPiece[0][3] = PIECE_QUEEN;  boardSide[0][3] = SIDE_BLACK;
  boardPiece[0][4] = PIECE_KING;   boardSide[0][4] = SIDE_BLACK;
  boardPiece[0][5] = PIECE_BISHOP; boardSide[0][5] = SIDE_BLACK;
  boardPiece[0][6] = PIECE_KNIGHT; boardSide[0][6] = SIDE_BLACK;
  boardPiece[0][7] = PIECE_ROOK;   boardSide[0][7] = SIDE_BLACK;

  // Pawns row 1
  for (int c = 0; c < 8; c++) {
    boardPiece[1][c] = PIECE_PAWN;
    boardSide[1][c]  = SIDE_BLACK;
  }

  // hasMoved already false everywhere
  currentTurn = SIDE_WHITE;
}

// ---------- Arduino setup / loop ----------

void setup() {
  setupMuxSelectPins();
  for (int m = 0; m < MUX_COUNT; m++) pinMode(muxSigPins[m], INPUT);

  Serial.begin(9600);

  strip.begin();
  strip.setBrightness(50);  // overall brightness limit
  strip.clear();
  strip.show();

  initStandardBoard();  // assume players set up correctly in these squares

  // compute the sensor index of the drifting square
  DRIFT_IDX = idxFromRowCol(DRIFT_ROW, DRIFT_COL);
}

void loop() {
  // ---------- Read all sensors ----------
  for (int m = 0; m < MUX_COUNT; m++) {
    for (int ch = 0; ch < CH_PER_MUX; ch++) {
      setMuxChannel(ch);

      delayMicroseconds(150);
      analogRead(muxSigPins[m]);  // throwaway
      delayMicroseconds(50);

      uint32_t sum = 0;
      for (int i = 0; i < 10; i++) sum += analogRead(muxSigPins[m]);

      int idx = idxOf(m, ch);
      uint16_t raw = sum / 10;
      uint16_t mv  = (uint32_t)raw * 5000UL / 1023UL;

      if (!baselineDone) {
        // accumulate raw for baseline
        baselineSum[idx] += raw;
      } else {
        uint16_t base = baselineMV[idx];
        uint16_t diff = (mv > base) ? (mv - base) : (base - mv);

        // per-square high threshold:
        //   - 10 mV normally
        //   - 55 mV for the drifty square
        uint16_t highThresh = (idx == DRIFT_IDX) ? 55 : HIGH_THRESH_MV_DEFAULT;
        uint16_t lowThresh  = LOW_THRESH_MV;

        if (diff >= highThresh) {
          instantTrig[idx] = true;
        } else if (diff <= lowThresh) {
          instantTrig[idx] = false;
        } else {
          // keep previous instantTrig state
        }
      }
    }
  }

  // ---------- Baseline handling ----------
  if (!baselineDone) {
    baselineLoops++;
    showCalibratingRainbow();

    if (baselineLoops >= BASELINE_LOOPS) {
      // compute baseline mV from accumulated raw sums
      for (int i = 0; i < TOTAL_SENSORS; i++) {
        uint16_t avgRaw = baselineSum[i] / baselineLoops;
        baselineMV[i] = (uint32_t)avgRaw * 5000UL / 1023UL;
      }
      baselineDone = true;
      Serial.println("Baseline established!");
      flashGreen();
    }

    // skip rest of loop until baseline is ready
    return;
  }

  // ---------- Persistence filter (stableTrig) ----------
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (instantTrig[i] == stableTrig[i]) {
      onCount[i] = 0;
      offCount[i] = 0;
    } else {
      if (instantTrig[i]) {
        if (++onCount[i] >= ON_CYCLES) {
          stableTrig[i] = true;
          onCount[i] = 0;
        }
        offCount[i] = 0;
      } else {
        if (++offCount[i] >= OFF_CYCLES) {
          stableTrig[i] = false;
          offCount[i] = 0;
        }
        onCount[i] = 0;
      }
    }
  }

  // ---------- Detect lift/place of any piece (sensor-level) ----------
  int liftIdx = -1, placeIdx = -1;
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (stablePrev[i] && !stableTrig[i]) {
      liftIdx = i;
      break;
    }
  }
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (!stablePrev[i] && stableTrig[i]) {
      placeIdx = i;
      break;
    }
  }

  // ---------- Handle new move start (attacker lifted) ----------
  if (!showingMoves && liftIdx != -1) {
    rowColFromIdx(liftIdx, liftedRow, liftedCol);

    liftedPieceType = boardPiece[liftedRow][liftedCol];
    liftedSide      = boardSide[liftedRow][liftedCol];
    liftedHasMoved  = boardHasMoved[liftedRow][liftedCol];

    // If someone lifts the wrong color (not currentTurn), ignore for logic
    if (liftedSide != currentTurn || liftedSide == SIDE_NONE) {
      // don't start a move, don't modify board
      liftedRow = liftedCol = -1;
      liftedPieceType = PIECE_NONE;
      liftedSide      = SIDE_NONE;
      liftedHasMoved  = false;
    } else {
      liftedStartRow  = liftedRow;
      liftedStartCol  = liftedCol;

      // Remove from logical board while in the air
      boardPiece[liftedRow][liftedCol]    = PIECE_NONE;
      boardSide[liftedRow][liftedCol]     = SIDE_NONE;
      boardHasMoved[liftedRow][liftedCol] = false;

      // reset capture lock
      captureLocked = false;
      captureRow = captureCol = -1;

      showingMoves = true;
    }
  }

  // ---------- While attacker is in the air: detect captured piece being lifted ----------
  if (showingMoves && !captureLocked) {
    for (int i = 0; i < TOTAL_SENSORS; i++) {
      if (stablePrev[i] && !stableTrig[i]) {
        int r, c;
        rowColFromIdx(i, r, c);

        // Must be an enemy piece logically there to consider it a capture candidate
        if (boardSide[r][c] != SIDE_NONE && boardSide[r][c] != liftedSide) {
          if (canPieceCaptureSquare(liftedPieceType, liftedSide,
                                    liftedRow, liftedCol, r, c)) {
            // Lock capture onto this square
            captureLocked = true;
            captureRow = r;
            captureCol = c;

            // Remove captured piece logically
            boardPiece[r][c]    = PIECE_NONE;
            boardSide[r][c]     = SIDE_NONE;
            boardHasMoved[r][c] = false;
            break;
          }
        }
      }
    }
  }

  // ---------- Handle placing the attacker ----------
  if (showingMoves && placeIdx != -1) {
    int newRow, newCol;
    rowColFromIdx(placeIdx, newRow, newCol);

    bool moveCompleted = false;

    // If capture is locked, only allow placing on the locked square
    if (captureLocked) {
      if (newRow == captureRow && newCol == captureCol) {
        if (liftedPieceType != PIECE_NONE && liftedSide != SIDE_NONE) {
          boardPiece[newRow][newCol] = liftedPieceType;
          boardSide[newRow][newCol]  = liftedSide;

          bool nowMoved = liftedHasMoved;
          if (newRow != liftedStartRow || newCol != liftedStartCol) {
            nowMoved = true; // actually moved
          }
          boardHasMoved[newRow][newCol] = nowMoved;
          moveCompleted = true;
        }
      } else {
        // ignore placement if not on locked capture square
      }
    } else {
      // Normal (non-locked) move or capture without pre-lift
      if (liftedPieceType != PIECE_NONE && liftedSide != SIDE_NONE) {
        // If enemy piece exists there logically, this is a capture
        if (boardSide[newRow][newCol] != SIDE_NONE &&
            boardSide[newRow][newCol] != liftedSide) {
          boardPiece[newRow][newCol]    = liftedPieceType;
          boardSide[newRow][newCol]     = liftedSide;
          boardHasMoved[newRow][newCol] = true;
          moveCompleted = true;
        } else if (boardSide[newRow][newCol] == SIDE_NONE) {
          // simple move to empty square
          bool nowMoved = liftedHasMoved;
          if (newRow != liftedStartRow || newCol != liftedStartCol) {
            nowMoved = true;
          }
          boardPiece[newRow][newCol]    = liftedPieceType;
          boardSide[newRow][newCol]     = liftedSide;
          boardHasMoved[newRow][newCol] = nowMoved;
          moveCompleted = true;
        }
      }
    }

    // reset lift/capture state
    showingMoves    = false;
    liftedRow       = -1;
    liftedCol       = -1;
    liftedPieceType = PIECE_NONE;
    liftedSide      = SIDE_NONE;
    liftedHasMoved  = false;
    liftedStartRow  = -1;
    liftedStartCol  = -1;
    captureLocked   = false;
    captureRow      = -1;
    captureCol      = -1;

    // If a real move just completed, check for checkmate
    if (moveCompleted) {
      handlePostMoveCheckmate();
    }
  }

  // Update previous stable state
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    stablePrev[i] = stableTrig[i];
  }

  // ---------- LEDs ----------
  strip.clear();

  // Red: actual occupied squares from sensors
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int idx = idxFromRowCol(row, col);
      if (stableTrig[idx]) {
        strip.setPixelColor(ledIndex(row, col), strip.Color(255, 0, 0));
      }
    }
  }

  // Blue / Green: legal moves for lifted piece
  if (showingMoves && liftedRow != -1) {
    if (captureLocked && captureRow != -1 && captureCol != -1) {
      // Only the locked capture square remains as the landing square (blue)
      highlightSquare(captureRow, captureCol);
    } else {
      showMovesForLiftedPiece();
    }
  }

  strip.show();
  delay(150);
}
