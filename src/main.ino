#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MicroOscUdp.h>

// Réseau Wi-Fi
const char *ssid = "Scale Router 2G";
const char *password = "scalerouter";

#define espId 1

#define USE_LEFT_SENSOR true
#define USE_RIGHT_SENSOR true
#define USE_CENTER_SENSOR true

#if USE_LEFT_SENSOR
#define TRIG_PIN_L 18
#define ECHO_PIN_L 4
#endif

#if USE_RIGHT_SENSOR
#define TRIG_PIN_R 19
#define ECHO_PIN_R 2
#endif

#if USE_CENTER_SENSOR
#define TRIG_PIN_C 21
#define ECHO_PIN_C 15
#endif

#define MIN_DISTANCE 50
#define MAX_DISTANCE 400

#define TIMEOUT 30000 // Timeout pour pulseIn en microsecondes (30 ms)

// setup ip de l'esp
IPAddress ip(192, 168, 3, espId);
IPAddress gateway(192, 168, 3, 99);
IPAddress dns(192, 168, 3, 99);
IPAddress subnet(255, 255, 255, 0);

// setup config osc
WiFiUDP myUdp;
IPAddress mySendIp(192, 168, 3, 100);

unsigned int myReceivePort = 8888;
unsigned int mySendPort = 7777;

MicroOscUdp<1024> myOsc(&myUdp, mySendIp, mySendPort);

const int numReadings = 20; // Taille du tampon pour la moyenne mobile

#if USE_LEFT_SENSOR
float readingsL[numReadings];
int readIndexL = 0;
float totalL = 0;
float averageL = 0;
bool isActiveL = false;
#endif

#if USE_RIGHT_SENSOR
float readingsR[numReadings];
int readIndexR = 0;
float totalR = 0;
float averageR = 0;
bool isActiveR = false;
#endif

#if USE_CENTER_SENSOR
float readingsC[numReadings];
int readIndexC = 0;
float totalC = 0;
float averageC = 0;
bool isActiveC = false;
#endif

char messageAddress[50];

void setup()
{
  Serial.begin(9600);

#if USE_LEFT_SENSOR
  pinMode(TRIG_PIN_L, OUTPUT);
  pinMode(ECHO_PIN_L, INPUT);
  for (int i = 0; i < numReadings; i++)
    readingsL[i] = 0;
#endif

#if USE_RIGHT_SENSOR
  pinMode(TRIG_PIN_R, OUTPUT);
  pinMode(ECHO_PIN_R, INPUT);
  for (int i = 0; i < numReadings; i++)
    readingsR[i] = 0;
#endif

#if USE_CENTER_SENSOR
  pinMode(TRIG_PIN_C, OUTPUT);
  pinMode(ECHO_PIN_C, INPUT);
  for (int i = 0; i < numReadings; i++)
    readingsC[i] = 0;
#endif

  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  myUdp.begin(myReceivePort);

  snprintf(messageAddress, 50, "/esp%d/wificonnected", espId);
  myOsc.sendInt(messageAddress, 1);
}

void loop()
{
#if USE_LEFT_SENSOR
  float distanceL = readUltrasonicDistance(TRIG_PIN_L, ECHO_PIN_L);
  updateReadings(distanceL, readingsL, readIndexL, totalL, averageL);
  handleSensorState("L", averageL, isActiveL);
#endif

#if USE_RIGHT_SENSOR
  float distanceR = readUltrasonicDistance(TRIG_PIN_R, ECHO_PIN_R);
  updateReadings(distanceR, readingsR, readIndexR, totalR, averageR);
  handleSensorState("R", averageR, isActiveR);
#endif

#if USE_CENTER_SENSOR
  float distanceC = readUltrasonicDistance(TRIG_PIN_C, ECHO_PIN_C);
  updateReadings(distanceC, readingsC, readIndexC, totalC, averageC);
  handleSensorState("C", averageC, isActiveC);
#endif

  delay(20);
}

void handleSensorState(const char *sensorLabel, float average, bool &isActive)
{
  bool newIsActive = average >= MIN_DISTANCE && average <= MAX_DISTANCE;
  if (newIsActive != isActive)
  {
    isActive = newIsActive;
    snprintf(messageAddress, 50, "/esp%d/isActive/%s", espId, sensorLabel);
    myOsc.sendInt(messageAddress, isActive ? 1 : 0);
  }

  if (isActive)
  {
    snprintf(messageAddress, 50, "/esp%d/distance/%s", espId, sensorLabel);
    myOsc.sendFloat(messageAddress, average);
  }
}

void updateReadings(float newReading, float readings[], int &readIndex, float &total, float &average)
{
  // Soustraire la lecture la plus ancienne du total
  total = total - readings[readIndex];
  // Ajouter la nouvelle lecture au tableau
  readings[readIndex] = newReading;
  // Ajouter la nouvelle lecture au total
  total = total + readings[readIndex];
  // Avancer à la prochaine position dans le tableau
  readIndex = (readIndex + 1) % numReadings;
  // Calculer la moyenne
  average = total / numReadings;
}

long readUltrasonicDistance(int trigPin, int echoPin)
{
  long duration;

  // Envoi d'une impulsion de 10 microsecondes sur le pin TRIG
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Lecture du pin ECHO avec timeout
  duration = pulseIn(echoPin, HIGH, TIMEOUT);

  // Calculer la distance en cm
  return duration * 0.034 / 2;
}