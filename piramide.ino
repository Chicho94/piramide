#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <OneWire.h>

// Constantes y variables
// ---------------------------------------------------

// INICIO/FIN DEL JUEGO
const int LedStartGame = 6;
const int btnTimer = 2;
volatile int BtnEstado = HIGH;  // Estado del botón de inicio
int isGameInit = 0;             // Variable para indicar si el juego ha iniciado

// REPRODUCTOR DE AUDIO
SoftwareSerial DFPlayerSerial(10, 11);
DFRobotDFPlayerMini myDFPlayer;

// SENSOR HALL
const int HallSensorPin = 8;
volatile int EstadoSensorHall = HIGH;  // Estado del sensor Hall
int currentEstadoSensorHall = 0;       // Estado actual del sensor
int isHallSensorActive = 0;            // Variable para indicar si el sensor Hall ha sido activado

// SENSOR DE OBSTRUCCION
const int IRBtn = 9;
int IRBtnEstado = LOW;  // Estado del sensor de obstrucción

// LEDS
#define PIN 7         // definiendo el puerto de los pines
#define NUMPIXELS 60  // definiendo la cantidad de leds que se van a aprender
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// BOTTON MAESTRO
OneWire net(4);
volatile int llaveSwitch = 0;                  // Estado de la llave maestra para reiniciar el juego
const String MasterCode = "01af565d010000a2";  // Código maestro para reiniciar el juego
String code = "";                              // Código recibido

// PUERTA FINAL
int isDoorOpenedActive = 0;    // Variable para indicar si la ultima puerta ha sido abierta
const int DoorSensorPin = 12;  // pin de la ultima puerta

// REGISTROS DE TIEMPO
unsigned long startTimer = 0;           // Variable para guardar el inicio del tiempo
unsigned long tiempoSegundos = 0;       // Variable para guardar el tiempo transcurrido en segundos
unsigned long tiempoSegundosGanar = 0;  // Variable para guardar la diferencia de tiempo

const int audioUno = 900;    // 15 minutos = 900 segundos
const int audioDos = 2700;   // 15 minutos = 900 segundos
const int audioTres = 3600;  // 15 minutos = 900 segundos

// Funciones
// ---------------------------------------------------

// Función de configuración inicial
void setup() {
  Serial.begin(19200);
  DFPlayerSerial.begin(9600);
  myDFPlayer.begin(DFPlayerSerial);
  myDFPlayer.volume(25);

  pinMode(btnTimer, INPUT_PULLUP);
  pinMode(LedStartGame, OUTPUT);
  pinMode(HallSensorPin, INPUT);
  pinMode(IRBtn, INPUT);
  pinMode(DoorSensorPin, INPUT);
  digitalWrite(LedStartGame, HIGH);

  currentEstadoSensorHall = digitalRead(HallSensorPin);

  pixels.begin();
}

// Función principal en bucle
void loop() {
  initGame();
  updateTimer();
  checkHallSensor();
  checkMasterKey();
  checkFinishDoorOpened();
}

// Función para verificar el estado del botón de inicio
void initGame() {
  int currentBtnEstado = digitalRead(btnTimer);
  if (currentBtnEstado != BtnEstado) {
    delay(50);
    if (currentBtnEstado == LOW && isGameInit == 0) {
      Serial.println("El juego ha iniciado (audio introductorio)");
      resetGame();
      isGameInit = 1;
      digitalWrite(LedStartGame, LOW);
      delay(100);
      audioShow(3);
      audioShow(6);
    }
    BtnEstado = currentBtnEstado;
  }
}

// Función para actualizar el temporizador
void updateTimer() {
  if (isGameInit == 1) {
    unsigned long currentMillis = millis();
    if (currentMillis - startTimer >= 1000) {
      startTimer = currentMillis;
      tiempoSegundos++;
    }
    // tiempo necesario para escuchar la primera pista
    if (tiempoSegundos == audioUno) {
      Serial.println("Primera pista");
      audioShow(4);
      audioShow(6);
    // tiempo necesario para escuchar la segunda pista
    } else if (tiempoSegundos == audioDos) {
      Serial.println("Segunda pista");
      audioShow(5);
      audioShow(6);
    // tiempo necesario para escuchar el audio de derrota
    } else if (tiempoSegundos == audioTres) {
      Serial.println("El juego ha finalizado: has perdido");
      audioShow(1);
      resetGame();
    }
  }
}

// Función para verificar el estado del sensor Hall
void checkHallSensor() {
  currentEstadoSensorHall = digitalRead(HallSensorPin);
  if (currentEstadoSensorHall != EstadoSensorHall && isGameInit == 1) {
    delay(50);
    lucesLed(1);
    Serial.println("El escarabajo ha sido colocado");
    EstadoSensorHall = currentEstadoSensorHall;
  }
}

// Función para verificar la llave maestra
void checkMasterKey() {
  byte addr[8];
  if (llaveSwitch == 0 && net.search(addr)) {
    decodeBytes(addr, 8);
    if (code == MasterCode) {
      Serial.println("Se ha usado la llave maestra");
      llaveSwitch = 1;
      digitalWrite(LedStartGame, HIGH);
    }
    net.reset();
  }

  if (llaveSwitch == 1 && digitalRead(btnTimer) == LOW) {
    Serial.println("Reinicio maestro del juego");
    resetGame();
  }
}

// Función para verificar si la puerta final fue abierta
void checkFinishDoorOpened() {
  if (isDoorOpenedActive == 0) {
    int currentEstadoSensorDoor = digitalRead(DoorSensorPin);
    if (currentEstadoSensorDoor != DoorSensorPin) {
      delay(50);
      if (currentEstadoSensorDoor == LOW && isGameInit == 1) {
        Serial.println("La puerta final ha sido abierta");
        isDoorOpenedActive = 1;
        winner();
      }
    }
  }
}

void winner() {
  Serial.println("El juego ha finalizado: has ganado");
  audioShow(2);
  resetGame();
}

// Función para controlar los LEDs
void lucesLed(int Estadoleds) {
  uint32_t rojo = pixels.Color(150, 0, 0);
  uint32_t negro = pixels.Color(0, 0, 0);

  for (int i = 0; i < NUMPIXELS; i++) {
    if (Estadoleds == 1) {
      pixels.setPixelColor(i, rojo);
    } else {
      pixels.setPixelColor(i, negro);
    }
  }
  pixels.show();
}

// Función para reiniciar el juego
void resetGame() {
  startTimer = 0;
  tiempoSegundos = 0;
  tiempoSegundosGanar = 0;
  llaveSwitch = 0;
  BtnEstado = HIGH;
  IRBtnEstado = LOW;
  isGameInit = 0;
  isHallSensorActive = 0;
  isDoorOpenedActive = 0;
  lucesLed(0);
  resetTimer();
  digitalWrite(LedStartGame, HIGH);
}

// Función para reiniciar el temporizador
void resetTimer() {
  startTimer = millis();
}

// Función para decodificar los bytes de la llave utilizada
void decodeBytes(const uint8_t* addr, uint8_t count) {
  code = "";
  for (uint8_t i = 0; i < count; i++) {
    code += String(addr[i] >> 4, HEX);
    code += String(addr[i] & 0x0f, HEX);
  }
}

/*
Tiempos de espera por cada audio
  intro: 31s 
  primer audio: 15s
  segundo audio: 25s
  derrota: 45s
  victoria: 26s
*/
// Función para reproducir un audio
void audioShow(int current) {
  Serial.println("Reproduciendo audio: " + String(current));
  delay(50);
  myDFPlayer.pause();
  myDFPlayer.play(current);
  switch (current) {
    case 3 : // audio intro
      delay(31000);
      break;
    case 4: // audio 2do
      delay(15000);
      break;
    case 5: // audio 3ro
      delay(25000);
      break;
    case 1: // audio de derrota
      delay(45000);
      break;
    case 2: // audio de victoria
      delay(26000);
      break;
    default: // otros audios
      delay(5000);
      break;
  }
}