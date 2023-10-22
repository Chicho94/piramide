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
int isGameFinish = 0;           // Variable para indicar si el juego ha finalizado

// REPRODUCTOR DE AUDIO
SoftwareSerial DFPlayerSerial(10, 11);
DFRobotDFPlayerMini myDFPlayer;

// SENSOR HALL
const int HallSensorPin = 8;
volatile int EstadoSensorHall = HIGH;  // Estado del sensor Hall
int isHallSensorActive = 0;            // Variable para indicar si el sensor Hall ha sido activado

// SENSOR DE OBSTRUCCION
const int IRBtn = 9;
int isIRledsActive = 0;  // Variable para indicar si los LEDs están encendidos debido al sensor de obstrucción
int IRBtnEstado = HIGH;  // Estado del sensor de obstrucción

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

const int audioTime = 900;
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

  pixels.begin();
}

// Función principal en bucle
void loop() {
  checkButton();
  checkIRSensor();
  updateTimer();
  checkHallSensor();
  checkMasterKey();
  checkFinishDoorOpened();
}

// Función para verificar el estado del botón de inicio
void checkButton() {
  int currentBtnEstado = digitalRead(btnTimer);
  if (currentBtnEstado != BtnEstado) {
    delay(50);
    if (currentBtnEstado == LOW && isGameInit == 0) {
      Serial.println("El juego ha iniciado (intro con primera pista)");
      resetGame();
      isGameInit = 1;
      digitalWrite(LedStartGame, LOW);
      delay(100);
      audioShow(1);
      audioShow(6);
    }
    BtnEstado = currentBtnEstado;
  }
}

// Función para verificar el estado del sensor de obstrucción
void checkIRSensor() {
  if (isIRledsActive == 0 && isHallSensorActive == 1) {
    int currentIRBtnEstado = digitalRead(IRBtn);
    if (currentIRBtnEstado != IRBtnEstado) {
      delay(50);
      if (currentIRBtnEstado == LOW && isIRledsActive == 0 && isGameInit == 1) {
        Serial.println("Encendiendo luces led");
        isIRledsActive = 1;
        lucesLed(1);
      }
      IRBtnEstado = currentIRBtnEstado;
    }
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
    if (tiempoSegundos == audioTime) {
      Serial.println("segunda pista");
      audioShow(2);
      audioShow(6);
      // tiempo necesario para escuchar la segunda pista
    } else if (tiempoSegundos == audioTime * 3) {
      Serial.println("cuarta pista");
      audioShow(3);
      audioShow(6);
    } else if (tiempoSegundos == audioTime * 4) {
      Serial.println("El juego ha finalizado: has perdido");
      audioShow(4);
      resetGame();
    }
  }
}

// Función para verificar el estado del sensor Hall
void checkHallSensor() {
  if (isHallSensorActive == 0) {
    int currentEstadoSensorHall = digitalRead(HallSensorPin);
    if (currentEstadoSensorHall != EstadoSensorHall) {
      delay(50);
      if (currentEstadoSensorHall == LOW && isGameInit == 1 && isHallSensorActive == 0) {
        isHallSensorActive = 1;
        tiempoSegundosGanar = tiempoSegundos + 10;
        isGameFinish = 1;
        Serial.println("El escarabajo ha sido activado");
      }
      EstadoSensorHall = currentEstadoSensorHall;
    }
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
  audioShow(5);
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
  IRBtnEstado = HIGH;
  isIRledsActive = 0;
  isGameFinish = 0;
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

// Función para reproducir un audio
void audioShow(int current) {
  Serial.println("Reproduciendo audio: " + String(current));
  delay(50);
  myDFPlayer.pause();
  myDFPlayer.play(current);
  if (current != 8) {
    delay(5000);
  }
}