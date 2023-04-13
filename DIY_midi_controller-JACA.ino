/*
    MetaReciclarte 2023 - Coletivo JACA
    Controlador MIDI com Arduino
    Colaboradores: Coletivo JACA, Muziek Mutantti, 
    Desenvolvedor: Vagné L.
    Email:dev@muziekmutantti.com.br
    GitHub: https://github.com/muziekmutantti
    GitBook:https://muziek-mutantti.gitbook.io/arduino-controlador-midi/
    Data:17.12.22
*/

/* "ATMEGA328" para placas ATmega328 - Uno, Mega, Nano  | "DEBUG" para debugar o código via monitor serial */
#define ATMEGA328 1  // *uC que você está usando como "ATMEGA328 1", "DEBUG 1".

// Adicionando BIBLIOTECAS
#ifdef ATMEGA328
#include <MIDI.h>  // Procure por "Francois Best, lathoub"
//MIDI_CREATE_DEFAULT_INSTANCE();  // Caso haja alguma falha ao compilar, comentar essa linha
#endif

// Declaração de Variaveis e Constantes //

//LED
const int LED = 12;

// BOTÕES
const int N_BOT = 4;                                //  número total de botões
const int BOT_ARDUINO_PIN[N_BOT] = { 2, 3, 4, 5 };  //* pinos de cada botão conectado diretamente ao Arduino

//#define pin13 1 // descomente se você estiver usando o pino 13 (o pino com led), ou comente a linha se não
//byte pin13index = 12;  //* coloque o índice do pin 13 do array buttonPin[] se você estiver usando, se não, comente

int botVAtual[N_BOT] = {};     // armazena o valor atual do botão
int botVAnterior[N_BOT] = {};  // armazena o valor anterior do botão

// debounce
unsigned long lastDebounceTime[N_BOT] = {};  // a última vez que o pino de saída foi alternado
unsigned long debounceDelay = 5;             //* o tempo de debounce; aumentar se a saída estiver mandando muitas notas de uma vez so

// POTENCIÔMETROS
const int N_POTS = 2;                            //* número total de pots (slide e rotativo)
const int POT_ARDUINO_PIN[N_POTS] = { A0, A1 };  //* pinos de cada pot conectado diretamente ao Arduino

int potVAtual[N_POTS] = { 0 };     // estado atual da porta analogica
int potVAnterior[N_POTS] = { 0 };  // estado previo da porta analogica
int potVar = 0;                    // variacao entre o valor do estado previo e o atual da porta analogica

int midiVAtual[N_POTS] = { 0 };     // Estado atual do valor midi
int midiVAnterior[N_POTS] = { 0 };  // Estado anterior do valor midi

const int TIMEOUT = 300;                     //* quantidade de tempo em que o potenciometro sera lido apos ultrapassar o varThreshold
const int varThreshold = 10;                 //* threshold para a variacao no sinal do potenciometro
boolean potMov = true;                       // se o potenciometro esta se movendo
unsigned long anteriorTime[N_POTS] = { 0 };  // tempo armazenado anteriormente
unsigned long timer[N_POTS] = { 0 };         // armazena o tempo que passou desde que o timer foi zerado

// midi
byte midiCh = 1;  //* Canal MIDI a ser usado
byte note = 36;   //* nota mais baixa a ser usada
byte cc = 1;      //* O mais baixo MIDI CC a ser usado

// SETUP
void setup() {

  // Baud Rate use se estiver usando with ATmega328 (uno, mega, nano...)
  // 31250 para MIDI class compliant | 115200 para Hairless MIDI
  Serial.begin(115200);  //*
  pinMode(LED, OUTPUT);

#ifdef DEBUG
  Serial.println("Modo Debug");
  Serial.println();
#endif

  // Buttons
  // Inicializa botões com pull up resistor
  for (int i = 0; i < N_BOT; i++) {
    pinMode(BOT_ARDUINO_PIN[i], INPUT_PULLUP);
  }

#ifdef pin13  // inicializa o pino 13 como uma entrada
  pinMode(BOT_ARDUINO_PIN[pin13index], INPUT);
#endif
}

// LOOP
void loop() {
  botoes();
  potenciometros();
}
// LOOP

// BOTÕES
void botoes() {
  for (int i = 0; i < N_BOT; i++) {
    botVAtual[i] = digitalRead(BOT_ARDUINO_PIN[i]);  // lê os pinos do arduino

#ifdef pin13
    if (i == pin13index) {
      botVAtual[i] = !botVAtual[i];  // inverte o pino 13 porque tem um resistor pull down em vez de um pull up
    }
#endif

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (botVAnterior[i] != botVAtual[i]) {
        lastDebounceTime[i] = millis();

        if (botVAtual[i] == LOW) {

// Envia a nota MIDI ON de acordo com a placa escolhida
#ifdef ATMEGA328                                   // ATmega328 (uno, mega, nano...)
          MIDI.sendNoteOn(note + i, 127, midiCh);  // note, velocity, channel
          leds(botVAtual[i]);
#elif DEBUG
          Serial.print(i);
          Serial.println(": Botão Ligado");
          leds(botVAtual[i]);
#endif

        } else {
// Envia a nota MIDI OFF de acordo com a placa escolhida
#ifdef ATMEGA328                                 // ATmega328 (uno, mega, nano...)
          MIDI.sendNoteOn(note + i, 0, midiCh);  // note, velocity, channel
          leds(botVAtual[i]);
#elif DEBUG
          Serial.print(i);
          Serial.println(": Botão Desligado");
          leds(botVAtual[i]);
#endif
        }
        botVAnterior[i] = botVAtual[i];
      }
    }
  }
}// FIM BOTÕES

//LEDS
void leds(int btnCS) {
  if (btnCS == LOW) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}//LEDS

// POTENCIÔMETROS
void potenciometros() {

  for (int i = 0; i < N_POTS; i++) {  // Faz o loop de todos os potenciômetros
    potVAtual[i] = analogRead(POT_ARDUINO_PIN[i]);
    midiVAtual[i] = map(potVAtual[i], 0, 1023, 0, 127);  // Mapeia a leitura do potVAtual para um valor utilizável em midi
    potVar = abs(potVAtual[i] - potVAnterior[i]);        // Calcula o valor absoluto entre a diferença entre o estado atual e o anterior do pot

    if (potVar > varThreshold) {   // Abre o portão se a variação do potenciômetro for maior que o limite (varThreshold)
      anteriorTime[i] = millis();  // Armazena o tempo anterior
    }

    timer[i] = millis() - anteriorTime[i];  // Reseta o timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) {  // Se o timer for menor que o tempo máximo permitido, significa que o potenciômetro ainda está se movendo
      potMov = true;
    } else {
      potMov = false;
    }

    if (potMov == true) {  // Se o potenciômetro ainda estiver em movimento, envie control change
      if (midiVAnterior[i] != midiVAtual[i]) {
// Envia o MIDI CC de acordo com a placa escolhida
#ifdef ATMEGA328
        // ATmega328 (uno, mega, nano...)
        MIDI.sendControlChange(cc + i, midiVAtual[i], midiCh);  // cc number, cc value, midi channel
#elif DEBUG
        Serial.print("Potenciometro: ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(midiVAtual[i]);
//Serial.print("  ");
#endif

        potVAnterior[i] = potVAtual[i];  // Armazena a leitura atual do potenciômetro para comparar com a próxima
        midiVAnterior[i] = midiVAtual[i];
      }
    }
  }
}// POTENCIÔMETROS
