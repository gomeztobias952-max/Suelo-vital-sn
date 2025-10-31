# 🌿 Suelo Vital SN

Proyecto de monitoreo de altura de pasto mediante sensores ultrasónicos, utilizando un **Arduino UNO** y un **módulo WiFi ESP-01** para enviar los datos a **ThingSpeak**.

---

## 🧩 Hardware utilizado
- Arduino UNO  
- Módulo WiFi ESP-01  
- 4 sensores ultrasónicos HC-SR04  
- LED indicador  
- Protoboard + cables de conexión  

---

## ⚙️ Funcionamiento
El sistema mide la altura del pasto en **cuatro puntos distintos**, calcula un **promedio** y lo envía a la nube mediante el módulo ESP-01.  
Cada lectura también se muestra en el **Monitor Serial** del Arduino.

Si la altura promedio es menor a un valor determinado (por ejemplo, 20 cm), se **enciende un LED** como alerta visual.

---

## 🌐 Conectividad WiFi y ThingSpeak
El módulo ESP-01 se conecta a tu red WiFi con las credenciales definidas en el código:

```cpp
const char* WIFI_SSID = "TuSSID";
const char* WIFI_PASS = "TuContraseña";
