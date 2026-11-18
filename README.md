# Pr-ctica_Servidor-Coap-en-ESP32-con-ARDUINO-ID
Instituto Técnologico superior del occidente del Estado de Hidalgo
<br>
Evidencias de equipo
[Evidencias](https://drive.google.com/drive/folders/15O4SZONXACXFswED1CB_16I621au7X_w?usp=drive_link).
# Servidor CoAP en ESP32-C3 (`/sensor` y `/led`)

Práctica de laboratorio en la que una **ESP32-C3** funciona como **servidor CoAP** (RFC 7252) sobre **UDP, puerto 5683**, con dos recursos: `/sensor` (temperatura simulada) y `/led` (control de un LED real). Un cliente en **Python con `aiocoap`** valida la comunicación y el tráfico se analiza con **Wireshark**.

**Instituto Tecnológico Superior del Occidente del Estado de Hidalgo**
División de Ingeniería en Tecnologías de la Información y Comunicaciones
**Profesor:** Saúl Isaí Soto Ortiz · **Fecha:** 18 de noviembre de 2026

---

## Índice

- [Objetivos](#objetivos)
- [Estructura del repositorio](#estructura-del-repositorio)
- [1. CoAP vs. HTTP sobre TCP](#1-coap-vs-http-sobre-tcp)
- [2. Anatomía de un mensaje CoAP](#2-anatomía-de-un-mensaje-coap)
- [3. Requisitos](#3-requisitos)
- [4. Servidor (ESP32)](#4-servidor-esp32)
- [5. Cliente (Python)](#5-cliente-python)
- [6. Resultados obtenidos](#6-resultados-obtenidos)
- [7. Análisis con Wireshark](#7-análisis-con-wireshark)
- [8. Reto 1: LED real controlado por CoAP](#8-reto-1-led-real-controlado-por-coap)
- [9. Seguridad de credenciales](#9-seguridad-de-credenciales)
- [10. Diagnóstico de fallos típicos](#10-diagnóstico-de-fallos-típicos)
- [11. Evidencia de funcionamiento](#11-evidencia-de-funcionamiento)
- [12. Preguntas y respuestas](#12-preguntas-y-respuestas)
- [Conclusiones](#conclusiones)
- [Referencias](#referencias)

---

## Objetivos

1. Comprender el funcionamiento del protocolo CoAP (RFC 7252) en arquitecturas IoT.
2. Configurar una placa ESP32 como servidor CoAP con Arduino IDE.
3. Implementar los recursos `/sensor` (GET) y `/led` (POST).
4. Validar la comunicación mediante peticiones desde un cliente externo.
5. Analizar el tráfico real con Wireshark y diagnosticar fallos comunes.

---

## Estructura del repositorio

```
Servidor-CoAP/
├── README.md
├── .gitignore
├── Servidor_CoAP_ESP32/
│   └── Servidor_CoAP_ESP32.ino   # Firmware del servidor CoAP (ESP32-C3)
├── pruebas_coap.py               # Cliente de pruebas P1–P4 (aiocoap)
├── resultados_pruebas.md         # Resultados generados por el cliente
└── Evidencias/                   # Capturas del Monitor Serie, cliente, Wireshark y fotos
```

---

## 1. CoAP vs. HTTP sobre TCP

| Aspecto | CoAP (UDP) | HTTP/1.1 (TCP) |
|---|---|---|
| Conexión | Sin conexión: 2 datagramas por intercambio | Handshake de 3 paquetes + ACKs + cierre |
| Cabecera | 4 bytes fijos | Cabeceras de texto de 150–300 bytes |
| Tamaño medido | Petición 13 B, respuesta 22 B (CoAP) | Varias veces mayor solo en cabeceras |
| Fiabilidad | Se elige por mensaje: CON (con ACK) o NON | Siempre la impone TCP |
| Consumo | Ideal para nodos con batería | Mayor uso de radio y memoria |
| Modelo | REST (GET, POST, PUT, DELETE) | REST |

---

## 2. Anatomía de un mensaje CoAP

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Ver| T |  TKL  |      Code     |          Message ID           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Token (0–8 bytes) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Opciones ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|1 1 1 1 1 1 1 1|    Payload ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Decodificación real de la petición `GET /sensor` capturada (`42 01 74 2d 91 a8 b6 73 65 6e 73 6f 72`):

| Bytes | Campo | Significado |
|---|---|---|
| `42` | Ver / Tipo / TKL | Versión 1 · Tipo 0 (CON) · TKL = 2 |
| `01` | Código | 0.01 = GET |
| `74 2d` | Message ID | 0x742D = 29741 |
| `91 a8` | Token | Identifica la petición a nivel de aplicación |
| `b6` + `73 65 6e 73 6f 72` | Opción 11 (Uri-Path) | Longitud 6: `"sensor"` |

---

## 3. Requisitos

### Hardware

| Elemento | Detalle |
|---|---|
| Placa | ESP32-C3 (perfil **ESP32C3 Dev Module**, USB nativo) |
| LED | LED externo + resistencia de **220 Ω** en el **GPIO 4** |
| Red | Punto de acceso de **2.4 GHz** compartido por la PC y la ESP32 |

### Software

| Elemento | Detalle |
|---|---|
| IDE | Arduino IDE 2.3.10 con soporte **esp32** de Espressif |
| Librería | **CoAP simple library** v1.4.0 (hirotakaster) |
| Cliente | Python 3.13 con **aiocoap** |
| Análisis | Wireshark sobre la interfaz Wi-Fi, filtro `coap` |

---

## 4. Servidor (ESP32)

1. Instalar el soporte de placas **esp32** de Espressif en Arduino IDE.
2. Instalar la librería **CoAP simple library** desde el Gestor de Librerías.
3. Abrir `Servidor_CoAP_ESP32/Servidor_CoAP_ESP32.ino` y escribir el SSID y contraseña de una red de **2.4 GHz**.
4. En *Herramientas* seleccionar:
   - Placa: **ESP32C3 Dev Module**
   - **USB CDC On Boot: Enabled** (si no, el Monitor Serie queda vacío)
   - Puerto: el que asigne Windows a la placa (en nuestro caso **COM14**)
5. Cargar el sketch y abrir el Monitor Serie a 115200 baudios para leer la **IP** asignada.

### Recursos registrados

| Recurso | Método | Payload | Respuesta |
|---|---|---|---|
| `/sensor` | GET | — | `Temp: XX.X C` (simulada entre 25.4 y 30.3 °C) |
| `/led` | POST | `ON` / `OFF` | `Estado: OK desde ESP32 CoAP Server (LED ON/OFF)` |
| Cualquier otra ruta | — | — | `4.04 Not Found` |

> **Nota:** el sketch se ajustó para que las respuestas devuelvan el **Token** de la petición, ya que `aiocoap` descarta las respuestas cuyo Token no coincide.

---

## 5. Cliente (Python)

```bash
python -m pip install aiocoap
python pruebas_coap.py <IP_DE_LA_ESP32>
```

Ejemplo:

```bash
python pruebas_coap.py 10.240.24.241
```

El script ejecuta las pruebas P1 a P4 y guarda los resultados en `resultados_pruebas.md`.

| Prueba | Petición | Resultado esperado |
|---|---|---|
| P1 | `GET coap://IP/sensor` | `2.05 Content` con la temperatura |
| P2 | `POST coap://IP/led` con payload `ON` | `2.05 Content` y el LED se enciende |
| P3 | `GET coap://IP/temperatura` | `4.04 Not Found` |
| P4 | `GET /sensor` × 5 | Cinco valores distintos dentro del rango |

> La IP puede cambiar cada vez que la placa se reconecta al punto de acceso; léela siempre del Monitor Serie.

---

## 6. Resultados obtenidos

| Prueba | Resultado obtenido | ¿Correcto? |
|---|---|---|
| P1 | `2.05 Content` · `Temp: 28.6 C` · 96.7 ms | ✅ |
| P2 | `2.05 Content` · `Estado: OK desde ESP32 CoAP Server (LED ON)` · Monitor Serie: `Payload recibido: ON` | ✅ |
| P3 | `4.04 Not Found` · 81.6 ms | ✅ |
| P4 | Cinco valores distintos dentro del rango | ✅ |

**P4 – Cinco lecturas consecutivas de `/sensor`:**

| # | Temperatura (°C) | Tiempo de respuesta (ms) |
|---|---|---|
| 1 | 25.7 | 17.0 |
| 2 | 30.3 | 77.4 |
| 3 | 27.4 | 10.2 |
| 4 | 26.9 | 9.0 |
| 5 | 27.8 | 9.8 |

**Interpretación de P3:** el `4.04` no es una falla del código. En `setup()` solo se registraron `led` y `sensor` con `coap.server(...)`; al no existir un *callback* para `temperatura`, la librería responde `4.04 Not Found` (equivalente al 404 de HTTP). CoAPSimple 1.4.0 envía este 4.04 **sin Token**, por lo que `aiocoap` lo ignora; para esta prueba el cliente construye el datagrama byte a byte:

| Mensaje | Bytes | Significado |
|---|---|---|
| Petición (16 B) | `40 01 50 c5 bb 74 65 6d 70 65 72 61 74 75 72 61` | CON, 0.01 GET, MID 0x50C5, Uri-Path `"temperatura"` |
| Respuesta (7 B) | `60 84 50 c5 c2 ff ff` | ACK, código 0x84 = 4.04, **mismo MID 0x50C5** |

---

## 7. Análisis con Wireshark

Captura de la interfaz Wi-Fi con el filtro `coap` (IP de la placa: `10.253.229.241`). Se observan **16 paquetes = 8 pares CON → ACK** (P1, P2, P3 y las cinco lecturas de P4).

| Campo | Petición (frame 39156) | Respuesta (frame 39490) |
|---|---|---|
| Origen → destino | 10.253.229.4 → 10.253.229.241:5683 | 10.253.229.241:5683 → 10.253.229.4 |
| Tipo | CON (Confirmable) | ACK (Acknowledgement) |
| Código | 0.01 GET | 2.05 Content |
| Message ID | 29741 (0x742D) | 29741 (0x742D) — **mismo** |
| Token | `91 a8` | `91 a8` — **mismo** |
| Uri-Path | `sensor` | — |
| Payload | — | `Temp: 29.4 C` (12 bytes, text/plain) |
| Tamaño de trama | 55 bytes | 64 bytes |
| Tamaño CoAP | **13 bytes** | **22 bytes** |

Otras observaciones:
- Cada ACK reutiliza el MID y el Token de su CON (MID 29741 a 29747, tokens `91 a8` a `91 ae`).
- El 4.04 de `/temperatura` viaja sin Token (49 bytes de trama, 7 de CoAP).
- Wireshark identifica la tarjeta de red de la placa como `Espressif_52:64:b8`.

---

## 8. Reto 1: LED real controlado por CoAP

En `callback_led()` se compara el payload: con `ON` se enciende el LED y con `OFF` se apaga; la respuesta informa el estado real.

**Conexión:**

```
GPIO 4 → resistencia 220 Ω → ánodo (pata larga) → cátodo (pata corta) → GND
```

- El LED enciende con `HIGH`; el pin se configura con `pinMode(LED_PIN, OUTPUT)` en `setup()`.
- Se evitó el **GPIO 8** porque, junto con el 2 y el 9, es un pin de arranque (*strapping*) de la ESP32-C3 y una carga a GND puede interferir con el encendido o la carga de código.

---

## 9. Seguridad de credenciales

- No subas el SSID ni la contraseña reales de tu red al repositorio.
- Antes de hacer `commit`, reemplázalos en el `.ino` por valores de ejemplo (`"TU_SSID"`, `"TU_PASSWORD"`), o muévelos a un archivo `secrets.h` e inclúyelo en `.gitignore`:

```
# .gitignore
secrets.h
__pycache__/
```

---

## 10. Diagnóstico de fallos típicos

| Síntoma | Causa | Solución |
|---|---|---|
| El puerto COM12 no existe | La ESP32-C3 usa USB nativo y Windows le asignó otro puerto | Seleccionar el puerto correcto (COM14) en *Herramientas → Puerto* |
| Monitor Serie vacío | *USB CDC On Boot* desactivado: `Serial` sale por los pines TX/RX | Activar **USB CDC On Boot: Enabled** y volver a cargar |
| La placa no se conecta al Wi-Fi | Las redes de la escuela solo transmitían en **5 GHz**, banda que la ESP32 no soporta | Usar un punto de acceso de **2.4 GHz** para la placa y la PC |
| Timeout desde el cliente aunque la placa esté conectada | Aislamiento entre clientes en redes escolares | Usar una red propia (hotspot) compartida por PC y placa |
| `aiocoap` no recibe la respuesta | La respuesta no devuelve el Token de la petición | Copiar el Token de la petición en la respuesta del sketch |
| P3 (`4.04`) no llega al cliente | CoAPSimple envía el 4.04 sin Token | Construir el datagrama CoAP manualmente sin Token |
| IP distinta en cada sesión | El punto de acceso asigna otra dirección al reconectar | Leer la IP del Monitor Serie antes de cada prueba |

---

## 11. Evidencia de funcionamiento

**Integrantes del equipo:**
- Irving Maldonado Olguin
- Alejandro Cruz Martinez
- Maria Guadalupe Gress Ugarte
- Brayan Hernandez Martinez

**Monitor Serie – sesión 1** (servidor en `10.240.24.241:5683`, peticiones a `/sensor` y payload `ON` en `/led`):

![Monitor Serie sesión 1](Evidencias/monitor_serie_sesion1.png)

**Monitor Serie – sesión 2** (red de 2.4 GHz, IP `10.253.229.241`):

![Monitor Serie sesión 2](Evidencias/monitor_serie_sesion2.png)

**Cliente CoAP en VS Code** (resultados P1 a P4):

![Cliente CoAP](Evidencias/cliente_pruebas.png)

**Wireshark** (pares CON/ACK, Message ID y Token):

![Wireshark captura](Evidencias/wireshark_captura.png)

**Wireshark – detalle de la respuesta** (ACK 2.05 Content, mismo MID y Token, payload `Temp: 29.4 C`):

![Wireshark detalle](Evidencias/wireshark_detalle.png)

**LED externo en el GPIO 4 encendido tras `POST /led ON`:**

![LED encendido](Evidencias/led_encendido.jpg)

---

## 12. Preguntas y respuestas

**1. ¿Por qué CoAP puede permitirse usar UDP si UDP no garantiza la entrega?**
Porque la fiabilidad la aporta la propia capa CoAP y solo cuando hace falta. Un mensaje **CON** exige un **ACK**; si no llega, el cliente retransmite con espera exponencial (2–3 s iniciales, hasta 4 reintentos por defecto) usando el mismo Message ID para que el servidor detecte duplicados. Con mensajes **NON** se envía y se olvida, útil en lecturas periódicas. Así se evitan el handshake, la conexión abierta y el control de flujo de TCP.

**2. ¿La respuesta de `/sensor` fue un ACK independiente o *piggyback*?**
Fue **piggyback**: la respuesta es un único **ACK** que ya contiene el código **2.05 Content** y el payload `Temp: 29.4 C`. Una respuesta separada mostraría primero un ACK vacío (0.00) y luego un nuevo CON con el 2.05. CoAPSimple responde dentro del mismo callback, así que siempre usa piggyback.

**3. ¿Qué campo permitió saber que la respuesta correspondía a nuestra petición?**
El **Message ID** (29741), que empareja ACK y CON a nivel de mensaje, y el **Token** (`91a8`), que empareja respuesta y petición a nivel de aplicación. Ambos son idénticos en los frames 39156 y 39490; Wireshark lo indica con «Request In: 39156». Además, la respuesta va dirigida a la IP y puerto UDP de origen del cliente (54774).

**4. Tamaño del mensaje CoAP de respuesta comparado con HTTP.**
La respuesta mide 64 bytes en el aire; restando Ethernet (14), IP (20) y UDP (8) quedan **22 bytes de CoAP**: 4 de cabecera + 2 de Token + 3 de Content-Format + 1 del marcador `0xFF` + 12 de payload. La petición ocupa **13 bytes**. Una respuesta HTTP/1.1 mínima suma 150–300 bytes solo de cabeceras, más el handshake, ACKs y cierre de TCP. CoAP resulta entre **7 y 14 veces** más pequeño y usa unas 5 veces menos paquetes.

**5. Bomba de riego con batería que reporta la humedad cada minuto: ¿CON o NON?**
**NON** para el reporte de humedad: perder una lectura importa poco porque la siguiente llega en 60 s, y el nodo puede transmitir y volver a dormir sin esperar ACK. La **orden de encender o apagar la bomba** sí debe ir como **CON**, porque perderla tiene consecuencias físicas.

**6. ¿Qué riesgo implica un servidor CoAP sin DTLS en una red compartida?**
Todo viaja en texto plano y sin autenticación: cualquier equipo de la red puede **leer** las respuestas con Wireshark y **enviar** un `POST /led OFF` para controlar la placa, o suplantar respuestas. Un servidor CoAP expuesto también puede usarse en ataques de amplificación UDP. Mitigaciones: DTLS u OSCORE, autenticación de clientes y una red IoT segmentada.

---

## Conclusiones

- CoAP conserva el modelo REST pero sobre UDP: cada intercambio fueron solo dos datagramas, de 13 y 22 bytes, sin establecer conexión.
- Con CoAPSimple, registrar un recurso es asociar un nombre a un callback; las rutas no registradas devuelven `4.04` automáticamente (P3).
- El Message ID y el Token permitieron emparejar cada respuesta con su petición sin necesidad de conexión.
- La fiabilidad en CoAP se decide por mensaje (CON o NON), no la impone el transporte.
- La infraestructura importa tanto como el código: la banda de 5 GHz, el USB nativo de la ESP32-C3 y el manejo del Token fueron los principales obstáculos.
- Sin DTLS, el tráfico es legible y la placa puede ser controlada por cualquier equipo de la red: aceptable en laboratorio, no en producción.

---

## Referencias

- Shelby, Z., Hartke, K. y Bormann, C. (2014). *The Constrained Application Protocol (CoAP)*, RFC 7252. IETF.
- Soto Ortiz, S. I. *Práctica de Laboratorio: Servidor CoAP en ESP32 con Arduino IDE*. Repositorio GitHub ssoto123.
- Hirotaka Niisato. *CoAP simple library for Arduino*, v1.4.0.
