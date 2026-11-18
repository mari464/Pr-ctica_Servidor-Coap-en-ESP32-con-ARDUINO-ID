# Resultados de pruebas CoAP — servidor 10.253.229.241

## P1 — GET /sensor
- Código: `2.05 Content`  |  Payload: `Temp: 29.7 C`  |  Tiempo: 166.5 ms

## P2 — POST /led (payload ON)
- Código: `2.05 Content`  |  Payload: `Estado: OK desde ESP32 CoAP Server (LED ON)`  |  Tiempo: 115.8 ms
- Revisa el Monitor Serie: debe decir `Payload recibido: ON`

## P3 — GET /temperatura (ruta inexistente)
- Código: `4.04`  (esperado 4.04 Not Found)  |  Tiempo: 85.4 ms
- Petición (16 bytes): `40 01 b4 fe bb 74 65 6d 70 65 72 61 74 75 72 61`
- Respuesta (7 bytes): `60 84 b4 fe c2 ff ff`

## P4 — GET /sensor x5
| # | Payload | Tiempo (ms) |
|---|---|---|
| 1 | Temp: 28.7 C | 3191.7 |
| 2 | Temp: 28.6 C | 96.6 |
| 3 | Temp: 28.0 C | 6.2 |
| 4 | Temp: 27.0 C | 86.3 |
| 5 | Temp: 25.9 C | 10.0 |
