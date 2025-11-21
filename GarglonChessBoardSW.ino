#include "chessBoard.h"
#include <FastLED.h>

struct Change {
  int row;
  int col;
  bool changed;
};  

// --------- MUX & sensor setup ---------
// MUX SIG output to Arduino Analog Inputs
#define pinMuxSig1 A0
#define pinMuxSig2 A1
#define pinMuxSig3 A2
#define pinMuxSig4 A3

// MUX select pins S0-S3 to Arduino 8-11; THESE ARE SHARED
#define muxS0 10
#define muxS1 11
#define muxS2 12
#define muxS3 13

#define NUM_CHANNELS 64  // C0–C15

// LED Settings
#define LED_PIN 6
#define NUM_LEDS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Baseline measurement vars
unsigned long baselineStartTime;
bool baselineDone = false;

long  baselineSum[NUM_CHANNELS]     = {0};
long  baselineSamples[NUM_CHANNELS] = {0};
float baselineV[NUM_CHANNELS]       = {0};

bool triggeredBoard[8][8];
CRGB leds[NUM_LEDS];

#define RESET_PIN 2 // FIXME: SET THIS LATER

ChessBoard board;
bool whiteTurn = true;

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

void baselineCalibration() {
  long  rawMeasure[NUM_CHANNELS];

  for (int ch = 0; ch < NUM_CHANNELS / 4; ch++) {
    setMuxChannel(ch);
    delayMicroseconds(50);  // mux settling time

    long sum1 = 0;
    long sum2 = 0;
    long sum3 = 0;
    long sum4 = 0;

    for (int i = 0; i < 10; i++) {
      sum1 += analogRead(pinMuxSig1);
      sum2 += analogRead(pinMuxSig2);
      sum3 += analogRead(pinMuxSig3);
      sum4 += analogRead(pinMuxSig4);
    }

    rawMeasure[ch] = sum1 / 10;
    rawMeasure[ch + (NUM_CHANNELS / 4)] = sum2 / 10;
    rawMeasure[ch + 2 * (NUM_CHANNELS / 4)] = sum3 / 10;
    rawMeasure[ch + 3 * (NUM_CHANNELS / 4)] = sum4 / 10;
  }

  if (millis() - baselineStartTime < 2000) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      baselineSum[ch] += rawMeasure[ch];
      baselineSamples[ch]++;
    }
    // Serial.println("Calibrating baseline...");
  } else {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      float avgRaw = (float)baselineSum[ch] / baselineSamples[ch];
      baselineV[ch] = avgRaw * 5000.0 / 1023.0;
    }

    baselineDone = true;
  }

  delay(50);
}


Change detectChange() {
  long  rawMeasure[NUM_CHANNELS];
  float outputV[NUM_CHANNELS];

  for (int ch = 0; ch < NUM_CHANNELS / 4; ch++) {
    setMuxChannel(ch);
    delayMicroseconds(50);  // mux settling time

    long sum1 = 0;
    long sum2 = 0;
    long sum3 = 0;
    long sum4 = 0;

    for (int i = 0; i < 10; i++) {
      sum1 += analogRead(pinMuxSig1);
      sum2 += analogRead(pinMuxSig2);
      sum3 += analogRead(pinMuxSig3);
      sum4 += analogRead(pinMuxSig4);
    }

    rawMeasure[ch] = sum1 / 10;
    rawMeasure[ch + (NUM_CHANNELS / 4)] = sum2 / 10;
    rawMeasure[ch + 2 * (NUM_CHANNELS / 4)] = sum3 / 10;
    rawMeasure[ch + 3 * (NUM_CHANNELS / 4)] = sum4 / 10;

    outputV[ch] = rawMeasure[ch] * 5000.0 / 1023.0;  // mV
    outputV[ch + (NUM_CHANNELS / 4)] = rawMeasure[ch + (NUM_CHANNELS / 4)] * 5000.0 / 1023.0;  // mV
    outputV[ch + 2*(NUM_CHANNELS / 4)] = rawMeasure[ch + 2*(NUM_CHANNELS / 4)] * 5000.0 / 1023.0;  // mV
    outputV[ch + 3*(NUM_CHANNELS / 4)] = rawMeasure[ch + 3*(NUM_CHANNELS / 4)] * 5000.0 / 1023.0;  // mV
  }

  // ---------- Detection ----------
  const float threshold = 200.0;  // mV
  int row = -1;
  int col = -1;
  bool changed = false;
  bool newVal;
  bool oldVal;

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    newVal = (abs(outputV[ch] - baselineV[ch]) > threshold);
    oldVal = triggeredBoard[ch/8][ch%8];

    if (oldVal != newVal)
    {
      row = ch/8;
      col = ch%8;
      changed = true;
      triggeredBoard[row][col] = newVal;
      break;
    }
  }

  return {row, col, changed}; 
}

int getLEDIndex(int row, int col) {
  int index;
  if (row % 2 == 0) {
    index = row * 8 + 7 - col;
  }
  else {
    index = row * 8 + col;
  }

  return index;
}

void boardReset() {
  board.reset();
  whiteTurn = true;
  FastLED.clear();
  FastLED.show();
}

void setup() {
  setupMuxSelectPins();
  pinMode(pinMuxSig1, INPUT);
  pinMode(pinMuxSig2, INPUT);
  pinMode(pinMuxSig3, INPUT);
  pinMode(pinMuxSig4, INPUT);

  // Serial.begin(9600);

  baselineStartTime = millis(); // begin 2s baseline window

  while (!baselineDone) {
    baselineCalibration();
  }

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(30); // Safe brightness
  FastLED.clear();
  FastLED.show();

  boardReset();

  pinMode(RESET_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(RESET_PIN), boardReset, RISING);
}

void loop() {
  FastLED.clear();
  FastLED.show();

  Change hasChanged = detectChange();

  while(!hasChanged.changed)
  {
    hasChanged = detectChange();
    delay(50);
  }

  int targetRow = hasChanged.row;
  int targetCol = hasChanged.col;

  ChessPiece targetPiece = board.getPiece(targetRow, targetCol);

  int moves[28][2];
  int numMoves;

  if (targetPiece.piece != EMPTY && targetPiece.color != whiteTurn) {
    board.getPossibleMoves(targetRow, targetCol, moves, numMoves);

    // indicate on LEDs
    for (int i = 0; i < numMoves; i++)
    {
      leds[getLEDIndex(moves[i][0], moves[i][1])] = CRGB::Green;
    }
    
    Change newPosition = detectChange();

    while (!newPosition.changed)
    {
      newPosition = detectChange();
      delay(50);
    }

    int newRow = newPosition.row;
    int newCol = newPosition.col;

    if (newRow == targetRow && newCol == targetCol) {
      return;
    }

    bool flag = false;

    for (int i = 0; i < numMoves; i++) {
      if (newRow == moves[i][0] && newCol == moves[i][1])
      {
        flag = true;
      }
    }

    if (!flag) {
      // handle illegal move FINISH LATER
      leds[getLEDIndex(newRow, newCol)] = CRGB::Red;
      FastLED.show();
      delay(1000);
      
      return;
    }

    board.moveTo(targetRow, targetCol, newRow, newCol);
    whiteTurn = !whiteTurn;
  }
}
