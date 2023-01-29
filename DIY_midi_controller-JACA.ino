/*
    MetaReciclarte 2023 - Coletivo JACA
    Controlador MIDI
    Colaboradores: Coletivo JACA, Muziek Mutantti, 
    Desenvolvedor: Vagné L.
    Email:dev@muziekmutantti.com.br
    GitHub: https://github.com/muziekmutantti
    GitBook:
    Data:17.12.22
*/

/*
 Defina o modelo da placa:
 "ATMEGA328" se estiver usando o ATmega328 - Uno, Mega, Nano ...
 "DEBUG" se você quer apenas debugar o código no monitor serial.
*/

#define DEBUG 1 // * coloque aqui o uC que você está usando, como nas linhas acima seguidas de "1", como "ATMEGA328 1", "DEBUG 1", etc.

/////////////////////////////////////////////
// BIBLIOTECAS
// -- Define a biblioteca MIDI -- //

// se estiver usando com ATmega328 - Uno, Mega, Nano ...
#ifdef ATMEGA328
#include <MIDI.h> // Use a "by Francois Best, lathoub"
MIDI_CREATE_DEFAULT_INSTANCE();  // Caso haja alguma falha ao compilar, comentar essa linha

// se estiver usando com ATmega32U4 - Micro, Pro Micro, Leonardo ...
#elif ATMEGA32U4
#include "MIDIUSB.h"

#endif


// --Criação de Variaveis e Constantes-- //

//LED
int LED = 12;
 
// BOTÕES
const int N_BUTTONS = 4; //*  número total de botões
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {2,3,4,5}; //* pinos de cada botão conectado diretamente ao Arduino

//#define pin13 1 // descomente se você estiver usando o pino 13 (o pino com led), ou comente a linha se não
byte pin13index = 12; //* coloque o índice do pin 13 do array buttonPin[] se você estiver usando, se não, comente

int buttonCState[N_BUTTONS] = {};        // armazena o valor atual do botão
int buttonPState[N_BUTTONS] = {};        // armazena o valor anterior do botão

// debounce
unsigned long lastDebounceTime[N_BUTTONS] = {0};  // a última vez que o pino de saída foi alternado
unsigned long debounceDelay = 5;    //* o tempo de debounce; aumentar se a saída estiver mandando muitas notas de uma vez so

// POTENCIÔMETROS
const int N_POTS = 2; //* número total de pots (slide e rotativo)
const int POT_ARDUINO_PIN[N_POTS] = {A0,A1}; //* pinos de cada pot conectado diretamente ao Arduino

int potCState[N_POTS] = {0}; // estado atual da porta analogica
int potPState[N_POTS] = {0}; // estado previo da porta analogica
int potVar = 0; // variacao entre o valor do estado previo e o atual da porta analogica

int midiCState[N_POTS] = {0}; // Estado atual do valor midi
int midiPState[N_POTS] = {0}; // Estado anterior do valor midi

const int TIMEOUT = 300; //* quantidade de tempo em que o potenciometro sera lido apos ultrapassar o varThreshold
const int varThreshold = 10; //* threshold para a variacao no sinal do potenciometro
boolean potMoving = true; // se o potenciometro esta se movendo
unsigned long PTime[N_POTS] = {0}; // tempo armazenado anteriormente
unsigned long timer[N_POTS] = {0}; // armazena o tempo que passou desde que o timer foi zerado


// Configuração MIDI
byte midiCh = 1; //* Canal MIDI a ser usado
byte note = 36; //* nota mais baixa a ser usada
byte cc = 1; //* O mais baixo MIDI CC a ser usado


// SETUP
void setup() {

  /* Baud Rate
     use se estiver usando with ATmega328 (uno, mega, nano...)
     31250 para MIDI class compliant | 115200 para Hairless MIDI */
  Serial.begin(115200); 
  pinMode(LED, OUTPUT);

#ifdef DEBUG
Serial.println("Modo Debug");
Serial.println();
#endif

  // Buttons
  // Inicializa botões com pull up resistor
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }

#ifdef pin13 // inicializa o pino 13 como uma entrada
pinMode(BUTTON_ARDUINO_PIN[pin13index], INPUT);
#endif

}

/////////////////////////////////////////////
// LOOP
void loop() {
  botoes();
  potenciometros();
  leds();
}

/////////////////////////////////////////////

//LEDS
void leds(){

}

// BOTÕES
void botoes() {
  for (int i = 0; i < N_BUTTONS; i++) {
    buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]);   // lê os pinos do arduino

#ifdef pin13
if (i == pin13index) {
buttonCState[i] = !buttonCState[i]; // inverte o pino 13 porque tem um resistor pull down em vez de um pull up
}
#endif

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {

          // Envia a nota MIDI ON de acordo com a placa escolhida
          #ifdef ATMEGA328
          // ATmega328 (uno, mega, nano...)
          MIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel
          digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)
          #elif DEBUG
          Serial.print(i);
          Serial.println(": Botão Ligado");
          digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)

          #endif

        } else {
            // Envia a nota MIDI OFF de acordo com a placa escolhida
            #ifdef ATMEGA328
            // ATmega328 (uno, mega, nano...)
            MIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel
            digitalWrite(LED, LOW);  // turn the LED on (LOW is the voltage level)
            #elif DEBUG
            Serial.print(i);
            Serial.println(": Botão Desligado");
            digitalWrite(LED, LOW);  // turn the LED on (HIGH is the voltage level)

            #endif
        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}

/////////////////////////////////////////////
// POTENCIÔMETROS
void potenciometros() {

  for (int i = 0; i < N_POTS; i++) { // Faz o loop de todos os potenciômetros

    potCState[i] = analogRead(POT_ARDUINO_PIN[i]);

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Mapeia a leitura do potCState para um valor utilizável em midi

    potVar = abs(potCState[i] - potPState[i]); // Calcula o valor absoluto entre a diferença entre o estado atual e o anterior do pot

    if (potVar > varThreshold) { // Abre o portão se a variação do potenciômetro for maior que o limite (varThreshold)
      PTime[i] = millis(); // Armazena o tempo anterior
    }

    timer[i] = millis() - PTime[i]; // Reseta o timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // Se o timer for menor que o tempo máximo permitido, significa que o potenciômetro ainda está se movendo
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // Se o potenciômetro ainda estiver em movimento, envie control change
      if (midiPState[i] != midiCState[i]) {
        // Envia o MIDI CC de acordo com a placa escolhida
        #ifdef ATMEGA328
        // ATmega328 (uno, mega, nano...)
        MIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel
        #elif DEBUG
        Serial.print("Potenciometro: ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(midiCState[i]);
        //Serial.print("  ");
        #endif

        potPState[i] = potCState[i]; // Armazena a leitura atual do potenciômetro para comparar com a próxima
        midiPState[i] = midiCState[i];
      }
    }
  }
}


