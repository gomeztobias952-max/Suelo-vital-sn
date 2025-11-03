#include <SoftwareSerial.h>

// ---------------- CONFIGURACIÓN DEL ESP ----------------
SoftwareSerial esp(2, 3);  // RX Arduino(2) <-- TX ESP | TX Arduino(3) --> RX ESP
const char* WIFI_SSID = "claudio";  // ⚠️ tu red WiFi exacta
const char* WIFI_PASS = "123456789";             // ⚠️ contraseña exacta

// Canal ThingSpeak
String writeAPIKey = "8XBJK1GAW61UV11Q";
String servidor = "api.thingspeak.com";

// ---------------- PINES DE LOS SENSORES ----------------
const int trigPin1 = 9,  echoPin1 = 10;
const int trigPin2 = 11, echoPin2 = 12;
const int trigPin3 = 5,  echoPin3 = 6;
const int trigPin4 = 13, echoPin4 = 4;
const int ledPin = 7;

// ---------------- VARIABLES ----------------
float d1, d2, d3, d4, promedio;
const float f1 = 1.018, f2 = 1.038, f3 = 1.038, f4 = 1.038;

// ---------------- FUNCIONES ----------------
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

float medirDistancia(int trig, int echo, float factor) {
  long duracion;
  float suma = 0;
  int validas = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(trig, LOW); delayMicroseconds(3);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    duracion = pulseIn(echo, HIGH, 30000);
    if (duracion > 0) { suma += (duracion * 0.034 / 2) * factor; validas++; }
    delay(40);
  }
  return (validas > 0) ? suma / validas : -1;
}

void conectarWiFi() {
  Serial.println("⏳ Conectando al WiFi...");
  enviarAT("AT", 1000);
  enviarAT("AT+CWMODE=1", 1000);
  String cmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + WIFI_PASS + "\"";
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

void enviarDatos(float s1, float s2, float s3, float s4) {
  // Crear solicitud HTTP para ThingSpeak
  String url = "GET /update?api_key=" + writeAPIKey +
               "&field1=" + String(s1, 2) +
               "&field2=" + String(s2, 2) +
               "&field3=" + String(s3, 2) +
               "&field4=" + String(s4, 2) + "\r\n";

  enviarAT("AT+CIPSTART=\"TCP\",\"" + servidor + "\",80", 4000);
  enviarAT("AT+CIPSEND=" + String(url.length() + 16), 2000);
  esp.print("GET /update?api_key=" + writeAPIKey +
            "&field1=" + String(s1, 2) +
            "&field2=" + String(s2, 2) +
            "&field3=" + String(s3, 2) +
            "&field4=" + String(s4, 2) +
            "\r\nHost: api.thingspeak.com\r\n\r\n");
  delay(3000);
  enviarAT("AT+CIPCLOSE", 1000);
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
  d1 = medirDistancia(trigPin1, echoPin1, f1);
  d2 = medirDistancia(trigPin2, echoPin2, f2);
  d3 = medirDistancia(trigPin3, echoPin3, f3);
  d4 = medirDistancia(trigPin4, echoPin4, f4);

  Serial.println("📏 Lecturas de sensores:");
  Serial.print("S1: "); Serial.println(d1);
  Serial.print("S2: "); Serial.println(d2);
  Serial.print("S3: "); Serial.println(d3);
  Serial.print("S4: "); Serial.println(d4);

  if (d1 < 0 || d2 < 0 || d3 < 0 || d4 < 0) {
    Serial.println("⚠ Error en sensor, reintentando...");
    delay(5000);
    return;
  }

  promedio = (d1 + d2 + d3 + d4) / 4.0;
  Serial.print("🌿 Promedio: "); Serial.print(promedio); Serial.println(" cm");

  digitalWrite(ledPin, promedio < 20 ? HIGH : LOW);
  enviarDatos(d1, d2, d3, d4);

  delay(20000);  // 20 segundos entre envíos
}
