// ============================================================
// Servidor CoAP en ESP32 - Práctica de laboratorio
// Basado en el código del Prof. Saúl Isaí Soto Ortiz, con:
//   - Respuestas que devuelven el Token de la petición (aiocoap
//     descarta respuestas cuyo Token no coincide).
//   - Reto 1 incluido: /led enciende/apaga el LED del GPIO 2.
// Recursos: GET /sensor  y  POST /led (payload ON / OFF)
// Librería: CoAP simple library (hirotakaster)
// ============================================================
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

// ---- CAMBIA ESTO: red Wi-Fi de 2.4 GHz ----
const char* ssid     = "Mary";
const char* password = "12345678";

// LED del Reto 1: se elige solo según la placa seleccionada.
#if defined(CONFIG_IDF_TARGET_ESP32C3)
const int LED_PIN = 4;    // LED externo en GPIO 4 (con resistencia de 220 ohm a GND)
const int LED_ON  = HIGH; // se enciende con HIGH
#else
const int LED_PIN = 2;          // ESP32 DevKit clásica: GPIO 2
const int LED_ON  = HIGH;
#endif
const int LED_OFF = !LED_ON;
bool ledEncendido = false;

WiFiUDP udp;
Coap coap(udp);

// Envía texto plano con código 2.05 Content, devolviendo el Token.
void responder(CoapPacket &packet, IPAddress ip, int port, const char* texto) {
  coap.sendResponse(ip, port, packet.messageid,
                    texto, strlen(texto),
                    COAP_CONTENT, COAP_TEXT_PLAIN,
                    packet.token, packet.tokenlen);
}

// ---------------- /led ----------------
void callback_led(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("Petición CoAP recibida en /led");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0';
  String payloadStr(p);
  payloadStr.trim();

  Serial.print("Payload recibido: ");
  Serial.println(payloadStr);

  // Reto 1: actuar sobre el LED real
  if (payloadStr.equalsIgnoreCase("ON")) {
    ledEncendido = true;
  } else if (payloadStr.equalsIgnoreCase("OFF")) {
    ledEncendido = false;
  }
  digitalWrite(LED_PIN, ledEncendido ? LED_ON : LED_OFF);

  String r = "Estado: OK desde ESP32 CoAP Server (LED ";
  r += ledEncendido ? "ON)" : "OFF)";
  responder(packet, ip, port, r.c_str());
}

// ---------------- /sensor ----------------
void callback_sensor(CoapPacket &packet, IPAddress ip, int port) {
  float temperaturaSimulada = 25.4 + (random(0, 50) / 10.0);  // 25.4 a 30.3 °C
  char respuesta[24];
  snprintf(respuesta, sizeof(respuesta), "Temp: %.1f C", temperaturaSimulada);

  Serial.print("GET /sensor -> ");
  Serial.println(respuesta);
  responder(packet, ip, port, respuesta);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);
  randomSeed(esp_random());   // valores distintos en cada arranque

  WiFi.mode(WIFI_STA);

  // Diagnóstico: redes de 2.4 GHz que la placa alcanza a ver
  Serial.println("Redes 2.4 GHz visibles:");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    Serial.printf("  %-32s canal %2d  %d dBm\n",
                  WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
  }
  if (n <= 0) Serial.println("  (ninguna)");

  WiFi.begin(ssid, password);
  Serial.printf("Conectando a \"%s\"", ssid);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++intentos % 40 == 0) {          // cada 20 s explica el motivo
      wl_status_t st = WiFi.status();
      Serial.println();
      if (st == WL_NO_SSID_AVAIL)
        Serial.println("No encuentro esa red: ¿es de 5 GHz o el nombre está mal escrito?");
      else if (st == WL_CONNECT_FAILED)
        Serial.println("La red rechazó la conexión: revisa la contraseña.");
      else
        Serial.printf("Sigo intentando (estado %d)\n", st);
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  Serial.println("\nConectado a la red");
  Serial.print("Dirección IP del Servidor CoAP: ");
  Serial.println(WiFi.localIP());

  coap.server(callback_led, "led");
  coap.server(callback_sensor, "sensor");
  coap.start();   // puerto UDP 5683
  Serial.println("Servidor CoAP escuchando en el puerto 5683");
}

void loop() {
  coap.loop();    // nada de delay() aquí
}
