#include <avr/pgmspace.h>

// Tipo Step con descrizione in PROGMEM e funzione associata
typedef void (*StepFunction)();

struct Step {
  const char* desc;       // Descrizione in PROGMEM
  StepFunction execute;   // Funzione associata al passo
};

// Descrizioni in Flash
const char desc_1A[] PROGMEM = "1.A - Inizializza DAC";
const char desc_1B[] PROGMEM = "1.B - Genera triangola 5V 1Hz";
const char desc_2A[] PROGMEM = "2.A - Attiva relè 1";
const char desc_2B[] PROGMEM = "2.B - Disattiva relè 1";

// === Funzioni di test ===
void test_initDAC() {
  Serial.println(">> Inizializzazione DAC completata.");
  // qui puoi inizializzare realmente il DAC
}

void test_generateTriangola() {
  Serial.println(">> Generazione onda triangolare 5V 1Hz.");
  // codice che genera realmente l'onda
}

void test_attivaRele() {
  Serial.println(">> Relè 1 attivato.");
  // digitalWrite(RELE1_PIN, HIGH);
}

void test_disattivaRele() {
  Serial.println(">> Relè 1 disattivato.");
  // digitalWrite(RELE1_PIN, LOW);
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

int currentMain = 0;
int currentSub = 0;

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("Sistema di collaudo pronto.");
  printCurrentStep();
}

void loop() {
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
    handleCommand(cmd);
    printCurrentStep();
  }
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'A': nextStep(); break;
    case 'I': prevStep(); break;
    case 'R': currentSub = 0; break;
    case 'S': skipToNextMain(); break;
    case 'E': executeCurrent(); break;
    default: Serial.println("Comando non valido. Usa A/I/R/S/E");
  }
}

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

void executeCurrent() {
  Step step;
  memcpy_P(&step, &testSequence[currentMain][currentSub], sizeof(Step));
  if (step.execute) step.execute();
}

void printCurrentStep() {
  Serial.println("--------------------------------------------------");
  Serial.println("A=Avanti | I=Indietro | R=Ripeti | S=Salta | E=Esegui");

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
