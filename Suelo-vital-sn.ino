#include <SoftwareSerial.h>

// ---------------- CONFIGURACIÓN ESP ----------------
SoftwareSerial esp(2, 3); // RX Arduino(2) <-- TX ESP | TX Arduino(3) --> RX ESP
const char* WIFI_SSID = "Familia Gomez -2.4GHZ";
const char* WIFI_PASS = "2219427800";
const char* SERVER_IP = "192.168.0.95";  // IP de tu PC donde corre Flask
const int SERVER_PORT = 5000;

// ---------------- PINES SENSORES ----------------
const int trigPin1 = 9, echoPin1 = 10;
const int trigPin2 = 11, echoPin2 = 12;
const int trigPin3 = 5,  echoPin3 = 6;
const int trigPin4 = 13, echoPin4 = 4;
const int ledPin = 7;

// ---------------- VARIABLES ----------------
float distancia1, distancia2, distancia3, distancia4, promedio;
const float factor1 = 1.018, factor2 = 1.038, factor3 = 1.038, factor4 = 1.038;

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

// Intenta conectar al WiFi
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

// Envía datos al servidor Flask
void enviarDatos(float altura) {
  // Codificar espacios en nombre y dirección
  String cliente = "Juan Perez";
  String direccion = "Calle 201";
  cliente.replace(" ", "%20");
  direccion.replace(" ", "%20");

  // Crear solicitud HTTP
  String url = "GET /update?cliente=" + cliente +
               "&dir=" + direccion +
               "&altura=" + String(altura, 2) +
               " HTTP/1.1\r\nHost: " + SERVER_IP + "\r\n\r\n";

  // Conexión TCP
  String cmd = String("AT+CIPSTART=\"TCP\",\"") + SERVER_IP + "\"," + SERVER_PORT;
  String resp = enviarAT(cmd, 5000);
  if (resp.indexOf("CONNECT") == -1) {
    Serial.println("⚠ Error al conectar al servidor");
    return;
  }

  // Enviar longitud
  enviarAT("AT+CIPSEND=" + String(url.length()), 2000);

  // Enviar solicitud GET
  enviarAT(url, 4000);

  // Cerrar conexión
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
  // Medir sensores
  distancia1 = medirDistancia(trigPin1, echoPin1, factor1);
  distancia2 = medirDistancia(trigPin2, echoPin2, factor2);
  distancia3 = medirDistancia(trigPin3, echoPin3, factor3);
  distancia4 = medirDistancia(trigPin4, echoPin4, factor4);

  // Mostrar mediciones individuales
  Serial.println("📏 Lecturas de sensores ultrasónicos:");
  Serial.print("Sensor 1: "); Serial.print(distancia1, 2); Serial.println(" cm");
  Serial.print("Sensor 2: "); Serial.print(distancia2, 2); Serial.println(" cm");
  Serial.print("Sensor 3: "); Serial.print(distancia3, 2); Serial.println(" cm");
  Serial.print("Sensor 4: "); Serial.print(distancia4, 2); Serial.println(" cm");
  Serial.println("----------------------------------------");

  // Validar mediciones
  if (distancia1 < 0 || distancia2 < 0 || distancia3 < 0 || distancia4 < 0) {
    Serial.println("⚠ Error de sensor, repitiendo en 5s");
    digitalWrite(ledPin, HIGH);
    delay(5000);
    return;
  }

  // Promedio
  promedio = (distancia1 + distancia2 + distancia3 + distancia4) / 4.0;
  Serial.print("🌿 Altura promedio: ");
  Serial.print(promedio, 2);
  Serial.println(" cm");

  // LED indicador
  digitalWrite(ledPin, promedio < 20 ? HIGH : LOW);

  // Enviar al servidor
  enviarDatos(promedio);

  Serial.println(); // línea en blanco para separar mediciones
  delay(5000); // Espera entre mediciones
}
