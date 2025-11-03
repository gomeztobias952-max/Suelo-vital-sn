#include <SoftwareSerial.h>

// ---------------- CONFIGURACIÓN ESP ----------------
SoftwareSerial esp(2, 3); // RX, TX

String ssid = "Familia Gomez -2.4GHz";          // <-- Cambiar
String password = "2219427800";  // <-- Cambiar
String apiKey = "8XBJK1GAW61UV11Q"; // API Write Key
String server = "api.thingspeak.com";

// ---------------- SENSORES ----------------
#define TRIG1 4
#define ECHO1 5
#define TRIG2 6
#define ECHO2 7
#define TRIG3 8
#define ECHO3 9
#define TRIG4 10
#define ECHO4 11

float medirDistancia(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duracion = pulseIn(echo, HIGH);
  float distancia = duracion * 0.0343 / 2;
  return distancia;
}

void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT); pinMode(ECHO4, INPUT);

  Serial.println("🌐 Iniciando ESP8266...");
  conectarWiFi();
}

void loop() {
  float s1 = medirDistancia(TRIG1, ECHO1);
  float s2 = medirDistancia(TRIG2, ECHO2);
  float s3 = medirDistancia(TRIG3, ECHO3);
  float s4 = medirDistancia(TRIG4, ECHO4);
  float promedio = (s1 + s2 + s3 + s4) / 4.0;

  Serial.println("📏 Lecturas de sensores ultrasónicos:");
  Serial.print("Sensor 1: "); Serial.print(s1); Serial.println(" cm");
  Serial.print("Sensor 2: "); Serial.print(s2); Serial.println(" cm");
  Serial.print("Sensor 3: "); Serial.print(s3); Serial.println(" cm");
  Serial.print("Sensor 4: "); Serial.print(s4); Serial.println(" cm");
  Serial.println("----------------------------------------");
  Serial.print("🌿 Altura promedio: "); Serial.print(promedio); Serial.println(" cm\n");

  enviarThingSpeak(promedio, s1, s2, s3, s4);
  delay(20000); // enviar cada 20 segundos
}

void conectarWiFi() {
  enviarAT("AT+RST", 2000);
  enviarAT("AT+CWMODE=1", 1000);
  enviarAT("AT+CWJAP=\"" + ssid + "\",\"" + password + "\"", 6000);
}

void enviarThingSpeak(float promedio, float s1, float s2, float s3, float s4) {
  String cmd = "AT+CIPSTART=\"TCP\",\"" + server + "\",80";
  if (enviarAT(cmd, 3000).indexOf("Error") == -1) {
    String getStr = "GET /update?api_key=" + apiKey +
                    "&field1=" + String(promedio, 2) +
                    "&field2=" + String(s1, 2) +
                    "&field3=" + String(s2, 2) +
                    "&field4=" + String(s3, 2) +
                    "&field5=" + String(s4, 2) + "\r\n\r\n";

    String sendCmd = "AT+CIPSEND=" + String(getStr.length());
    if (enviarAT(sendCmd, 2000).indexOf(">") != -1) {
      esp.print(getStr);
      Serial.println("✅ Datos enviados correctamente a ThingSpeak!");
    }
  }
  enviarAT("AT+CIPCLOSE", 1000);
}

String enviarAT(String cmd, const int timeout) {
  String respuesta = "";
  esp.println(cmd);
  long int t = millis();
  while ((t + timeout) > millis()) {
    while (esp.available()) {
      char c = esp.read();
      respuesta += c;
    }
  }
  Serial.println(respuesta);
  return respuesta;
}
