#include <SoftwareSerial.h>

// ---------------- CONFIGURACIÓN ESP ----------------
SoftwareSerial esp(2, 3); // RX Arduino(2) <-- TX ESP | TX Arduino(3) --> RX ESP
const char* WIFI_SSID = "claudio";
const char* WIFI_PASS = "123456789";

// ---------------- CONFIGURACIÓN THINGSPEAK ----------------
const char* THINGSPEAK_SERVER = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = "8XBJK1GAW61UV11Q"; // Write API Key
const int THINGSPEAK_PORT = 80;

// ---------------- PINES SENSORES ----------------
const int trigPin1 = 9, echoPin1 = 10;
const int trigPin2 = 11, echoPin2 = 12;
const int trigPin3 = 5,  echoPin3 = 6;
const int trigPin4 = 13, echoPin4 = 4;
const int ledPin = 7;

// ---------------- VARIABLES ----------------
float distancia1, distancia2, distancia3, distancia4, promedio;
const float factor1 = 1, factor2 = 1, factor3 = 1, factor4 = 1;

// ---------------- FUNCIONES ----------------

// Envía comando AT y devuelve respuesta
String enviarAT(String comando, const int timeOut) {
  String respuesta = "";
  esp.println(comando);
  long t = millis();
  while (millis() - t < timeOut) {
    if (esp.available()) respuesta += esp.readString();
  }
  Serial.println("→ " + comando);
  Serial.println("← " + respuesta);
  return respuesta;
}

// Medición ultrasónica (promedio de 3 lecturas válidas)
float medirDistancia(int trig, int echo, float factor) {
  long duracion;
  float suma = 0;
  int validas = 0;

  for (int i = 0; i < 3; i++) {
    digitalWrite(trig, LOW);
    delayMicroseconds(3);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    duracion = pulseIn(echo, HIGH, 30000);
    if (duracion > 0) {
      suma += (duracion * 0.034 / 2) * factor;
      validas++;
    }
    delay(40);
  }

  return (validas > 0) ? suma / validas : -1;
}

// Conexión WiFi
void conectarWiFi() {
  Serial.println("⏳ Conectando al WiFi...");
  enviarAT("AT", 1000);
  enviarAT("AT+CWMODE=1", 1000);

  String cmd = String("AT+CWJAP=\"") + WIFI_SSID + "\",\"" + WIFI_PASS + "\"";
  String resp = enviarAT(cmd, 10000);

  if (resp.indexOf("WIFI CONNECTED") != -1 || resp.indexOf("OK") != -1) {
    Serial.println("✅ WiFi conectado correctamente");
  } else {
    Serial.println("❌ Falló conexión WiFi, reintentando...");
    delay(5000);
    conectarWiFi();
  }

  enviarAT("AT+CIPMUX=0", 1000);
}

// Envío de datos a ThingSpeak
void enviarAThingSpeak(float altura) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += THINGSPEAK_SERVER;
  cmd += "\",80";
  String resp = enviarAT(cmd, 5000);
  if (resp.indexOf("CONNECT") == -1) {
    Serial.println("⚠ Error al conectar con ThingSpeak");
    return;
  }

  // Datos a enviar
  String data = "api_key=" + String(THINGSPEAK_API_KEY) + "&field1=" + String(altura, 2);

  // Encabezado HTTP
  String peticion = "POST /update HTTP/1.1\r\n";
  peticion += "Host: api.thingspeak.com\r\n";
  peticion += "Connection: close\r\n";
  peticion += "Content-Type: application/x-www-form-urlencoded\r\n";
  peticion += "Content-Length: " + String(data.length()) + "\r\n\r\n";
  peticion += data;

  enviarAT("AT+CIPSEND=" + String(peticion.length()), 2000);
  enviarAT(peticion, 5000);
  enviarAT("AT+CIPCLOSE", 1000);

  Serial.println("✅ Datos enviados a ThingSpeak");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  pinMode(trigPin1, OUTPUT); pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT); pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT); pinMode(echoPin3, INPUT);
  pinMode(trigPin4, OUTPUT); pinMode(echoPin4, INPUT);
  pinMode(ledPin, OUTPUT);

  conectarWiFi();
}

// ---------------- LOOP ----------------
void loop() {
  // Medir sensores
  distancia1 = medirDistancia(trigPin1, echoPin1, factor1);
  distancia2 = medirDistancia(trigPin2, echoPin2, factor2);
  distancia3 = medirDistancia(trigPin3, echoPin3, factor3);
  distancia4 = medirDistancia(trigPin4, echoPin4, factor4);

  // Mostrar mediciones
  Serial.println("📏 Lecturas de sensores ultrasónicos:");
  Serial.print("Sensor 1: "); Serial.print(distancia1, 2); Serial.println(" cm");
  Serial.print("Sensor 2: "); Serial.print(distancia2, 2); Serial.println(" cm");
  Serial.print("Sensor 3: "); Serial.print(distancia3, 2); Serial.println(" cm");
  Serial.print("Sensor 4: "); Serial.print(distancia4, 2); Serial.println(" cm");
  Serial.println("----------------------------------------");

  if (distancia1 < 0 || distancia2 < 0 || distancia3 < 0 || distancia4 < 0) {
    Serial.println("⚠ Error de sensor, repitiendo en 5s");
    digitalWrite(ledPin, HIGH);
    delay(5000);
    return;
  }

  // Calcular promedio
  promedio = (distancia1 + distancia2 + distancia3 + distancia4) / 4.0;
  Serial.print("🌿 Altura promedio: ");
  Serial.print(promedio, 2);
  Serial.println(" cm");

  // LED indicador
  digitalWrite(ledPin, promedio < 20 ? HIGH : LOW);

  // Enviar a ThingSpeak
  enviarAThingSpeak(promedio);

  Serial.println();
  delay(20000); // ThingSpeak permite 1 envío cada 15 segundos (mínimo)
}
