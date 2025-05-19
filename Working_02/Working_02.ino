#include <avr/pgmspace.h>

typedef void (*StepFunction)();

struct Step {
  const char* desc;
  StepFunction execute;
};

// === Descrizioni test in flash memory ===
const char desc_1A[] PROGMEM = "1.A - Inizializza DAC";
const char desc_1B[] PROGMEM = "1.B - Genera triangola 5V 1Hz";
const char desc_2A[] PROGMEM = "2.A - Attiva relè 1";
const char desc_2B[] PROGMEM = "2.B - Disattiva relè 1";

// === Funzioni di test ===
void test_initDAC() {
  Serial.println(">> DAC inizializzato");
  delay(500);
}

unsigned long lastToggleTime = 0;
bool waveHigh = false;
void test_generateTriangola() {
  unsigned long now = millis();
  if (now - lastToggleTime >= 500) {
    lastToggleTime = now;
    waveHigh = !waveHigh;
    Serial.println(waveHigh ? "Triangola: +5V" : "Triangola: 0V");
  }
}

void test_attivaRele() {
  Serial.println("Relè 1 ON");
  delay(500);
}

void test_disattivaRele() {
  Serial.println("Relè 1 OFF");
  delay(500);
}

// === Sequenza dei test ===
const Step testSequence[][5] PROGMEM = {
  {
    { desc_1A, test_initDAC },
    { desc_1B, test_generateTriangola },
    { nullptr, nullptr }
  },
  {
    { desc_2A, test_attivaRele },
    { desc_2B, test_disattivaRele },
    { nullptr, nullptr }
  }
};

const int numMainPoints = sizeof(testSequence) / sizeof(testSequence[0]);

// === Stato ===
int currentMain = 0;
int currentSub = 0;
bool isExecuting = false;
StepFunction currentStepFunction = nullptr;

bool menuMode = false;
char menuBuffer[4];
byte menuBufferIndex = 0;
int totalSteps = 0;

void setup() {
  Serial.begin(9600);
  delay(500);
  totalSteps = countTotalSteps();
  Serial.println("Sistema di collaudo pronto.");
  printCurrentStep();
}

void loop() {
  // === Gestione modalità menù numerico ===
  if (menuMode) {
    while (Serial.available()) {
      char c = Serial.read();
      if (isdigit(c) && menuBufferIndex < 3) {
        menuBuffer[menuBufferIndex++] = c;
        Serial.write(c);
      } else if (c == '\n' || c == '\r') {
        menuBuffer[menuBufferIndex] = '\0';
        int sel = atoi(menuBuffer);
        menuMode = false;
        menuBufferIndex = 0;
        memset(menuBuffer, 0, sizeof(menuBuffer));
        if (sel >= 1 && sel <= totalSteps) {
          gotoStepByIndex(sel);
          startExecution();
        } else {
          Serial.println("\n>> Numero non valido.");
          printCurrentStep();
        }
        return;
      }
    }
    return;
  }

  // === Esecuzione continua se attiva ===
  if (isExecuting && currentStepFunction != nullptr) {
    currentStepFunction();
  }

  // === Input seriale normale ===
  if (Serial.available()) {
    char cmd = toupper(Serial.read());

    if (isExecuting && cmd == 'Q') {
      isExecuting = false;
      Serial.println(">> Esecuzione interrotta (Q).");
      printCurrentStep();
      return;
    }

    if (!isExecuting) {
      handleCommand(cmd);
      printCurrentStep();
    }
  }
}

// === Comandi base ===
void handleCommand(char cmd) {
  switch (cmd) {
    case 'A': nextStep(); break;
    case 'I': prevStep(); break;
    case 'R': currentSub = 0; break;
    case 'S': skipToNextMain(); break;
    case 'E': startExecution(); break;
    case 'M': enterMenu(); break;
    default: Serial.println("Comando non valido. Usa A/I/R/S/E/M o Q.");
  }
}

void enterMenu() {
  Serial.println("\n-- MENÙ TEST DISPONIBILI --");
  printAllTests();
  Serial.print("Inserisci numero da 1 a ");
  Serial.print(totalSteps);
  Serial.println(" e premi INVIO:");
  menuMode = true;
  menuBufferIndex = 0;
  memset(menuBuffer, 0, sizeof(menuBuffer));
}

// === Navigazione test ===
bool hasStep(int mainIdx, int subIdx) {
  Step step;
  memcpy_P(&step, &testSequence[mainIdx][subIdx], sizeof(Step));
  return step.desc != nullptr;
}

void nextStep() {
  if (hasStep(currentMain, currentSub + 1)) {
    currentSub++;
  } else if (currentMain + 1 < numMainPoints) {
    currentMain++;
    currentSub = 0;
  }
}

void prevStep() {
  if (currentSub > 0) {
    currentSub--;
  } else if (currentMain > 0) {
    currentMain--;
    currentSub = 0;
  }
}

void skipToNextMain() {
  if (currentMain + 1 < numMainPoints) {
    currentMain++;
    currentSub = 0;
  }
}

void startExecution() {
  Step step;
  memcpy_P(&step, &testSequence[currentMain][currentSub], sizeof(Step));
  if (step.execute) {
    currentStepFunction = step.execute;
    isExecuting = true;
    Serial.println(">> ESECUZIONE IN CORSO... premi Q per uscire");
  }
}

void gotoStepByIndex(int idx) {
  int count = 1;
  for (int i = 0; i < numMainPoints; i++) {
    for (int j = 0; hasStep(i, j); j++) {
      if (count == idx) {
        currentMain = i;
        currentSub = j;
        Serial.print("\n>> Selezionato test ");
        Serial.println(idx);
        printCurrentStep();
        return;
      }
      count++;
    }
  }
}

int countTotalSteps() {
  int count = 0;
  for (int i = 0; i < numMainPoints; i++) {
    for (int j = 0; hasStep(i, j); j++) {
      count++;
    }
  }
  return count;
}

void printCurrentStep() {
  Serial.println("--------------------------------------------------");
  Serial.println("A=Avanti | I=Indietro | R=Ripeti | S=Salta | E=Esegui | M=Menù | Q=Quit");

  Step step;
  memcpy_P(&step, &testSequence[currentMain][currentSub], sizeof(Step));

  char buffer[64];
  if (step.desc != nullptr) {
    strcpy_P(buffer, step.desc);
    Serial.println(buffer);
  } else {
    Serial.println("(Nessun passo disponibile)");
  }

  Serial.println("--------------------------------------------------");
}

void printAllTests() {
  int count = 1;
  char buffer[64];
  for (int i = 0; i < numMainPoints; i++) {
    for (int j = 0; hasStep(i, j); j++) {
      Step step;
      memcpy_P(&step, &testSequence[i][j], sizeof(Step));
      strcpy_P(buffer, step.desc);
      Serial.print(count);
      Serial.print(") ");
      Serial.println(buffer);
      count++;
    }
  }
}
