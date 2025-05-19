#include <avr/pgmspace.h>

const byte ledPin = 13;

// Stato corrente
int currentStepIndex = 0;
int currentSubStepIndex = 0;
bool isExecuting = false;
bool inMenu = false;

// Descrizioni in PROGMEM
const char desc_1A[] PROGMEM = "1.A - Inizializza DAC";
const char desc_1B[] PROGMEM = "1.B - Genera triangola 5V 1Hz";
const char desc_1C[] PROGMEM = "1.C - Blink LED finché non premi Q";

// Tipo Step
typedef void (*TestFunc)();
struct Step {
  const char* description;
  TestFunc function;
};

// Prototipi
void test_initDAC();
void test_generateTriangola();
void test_blinkLED();

// Sequenza test
const Step testSequence[][5] PROGMEM = {
  {
    { desc_1A, test_initDAC },
    { desc_1B, test_generateTriangola },
    { desc_1C, test_blinkLED },
    { nullptr, nullptr }
  },
  {
    { desc_1A, test_initDAC }, // per esempio, ripeti per test 2.A ecc.
    { nullptr, nullptr }
  }
};

const int numMainSteps = sizeof(testSequence) / sizeof(testSequence[0]);

// Blink non bloccante
unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  printCurrentStep();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r' || c == ' ') return;
    char cmd = toupper(c);

    if (isExecuting && cmd == 'Q') {
      isExecuting = false;
      Serial.println(F(">> Esecuzione interrotta (Q)"));
      printCurrentStep();
      return;
    }

    if (inMenu && isDigit(cmd)) {
      static int menuIndex = 0;
      menuIndex = menuIndex * 10 + (cmd - '0');
      delay(10);
      if (!Serial.available()) {
        goToMenuStep(menuIndex - 1);
        inMenu = false;
        menuIndex = 0;
        printCurrentStep();
      }
      return;
    }

    if (!isExecuting) {
      handleCommand(cmd);
      printCurrentStep();
    }
  }

  if (isExecuting) {
    Step s;
    memcpy_P(&s, &testSequence[currentStepIndex][currentSubStepIndex], sizeof(Step));
    if (s.function) s.function();
  }
}

// Comandi
void handleCommand(char cmd) {
  switch (cmd) {
    case 'A':
      currentStepIndex++;
      currentSubStepIndex = 0;
      if (currentStepIndex >= numMainSteps) currentStepIndex = numMainSteps - 1;
      break;
    case 'I':
      currentStepIndex--;
      currentSubStepIndex = 0;
      if (currentStepIndex < 0) currentStepIndex = 0;
      break;
    case 'R':
      currentSubStepIndex = 0;
      break;
    case 'S':
      currentStepIndex++;
      currentSubStepIndex = 0;
      if (currentStepIndex >= numMainSteps) currentStepIndex = numMainSteps - 1;
      break;
    case 'E':
      isExecuting = true;
      break;
    case 'M':
      Serial.println(F("== MENU: Digita numero test (es: 1..99) =="));
      inMenu = true;
      break;
    default:
      Serial.println(F("Comando non valido. Usa A/I/R/S/E/M o Q."));
  }
}

void printCurrentStep() {
  Step s;
  memcpy_P(&s, &testSequence[currentStepIndex][currentSubStepIndex], sizeof(Step));
  char buffer[64];
  strcpy_P(buffer, (PGM_P)s.description);
  Serial.println();
  Serial.println(F("=========================="));
  Serial.println(buffer);
  Serial.println(F("Comandi: A=Avanti I=Indietro R=Ripeti S=Salta E=Esegui M=Menù Q=Stop"));
  Serial.println(F("=========================="));
}

// Vai a test da menu
void goToMenuStep(int index) {
  if (index < 0 || index >= numMainSteps) {
    Serial.println(F("Test non valido."));
    return;
  }
  currentStepIndex = index;
  currentSubStepIndex = 0;
}

// === ESECUZIONE TEST ===
void test_initDAC() {
  Serial.println(F(">> Simulo inizializzazione DAC"));
  delay(1000);
  isExecuting = false;
}

void test_generateTriangola() {
  Serial.println(F(">> Simulo triangola 1Hz 5V"));
  delay(1000);
  isExecuting = false;
}

void test_blinkLED() {
  unsigned long now = millis();
  if (now - lastBlink >= 500) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? F("LED ON") : F("LED OFF"));
  }
}
