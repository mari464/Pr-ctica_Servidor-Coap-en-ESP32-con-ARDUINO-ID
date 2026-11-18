# pruebas_coap.py - Ejecuta las pruebas P1-P4 de la práctica CoAP
# Uso:   python pruebas_coap.py 192.168.1.50
# Requiere: pip install aiocoap
# Guarda la evidencia en resultados_pruebas.md
import asyncio
import sys
import time
from aiocoap import Context, Message, GET, POST

TIMEOUT = 10  # segundos


async def pedir(ctx, code, uri, payload=b""):
    inicio = time.perf_counter()
    msg = Message(code=code, uri=uri, payload=payload)
    resp = await asyncio.wait_for(ctx.request(msg).response, TIMEOUT)
    ms = (time.perf_counter() - inicio) * 1000
    return resp, ms


def get_crudo(ip, ruta):
    """GET CON sin Token, construido byte a byte (sección 3.2 del README)."""
    import random, socket
    mid = random.randint(0, 0xFFFF)
    path = ruta.encode()
    # Ver=1, T=CON(0), TKL=0 -> 0x40 | Code 0.01 GET | Message ID
    pkt = bytes([0x40, 0x01, mid >> 8, mid & 0xFF])
    # Opción Uri-Path (número 11), longitud < 13
    pkt += bytes([(11 << 4) | len(path)]) + path
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(TIMEOUT)
    inicio = time.perf_counter()
    s.sendto(pkt, (ip, 5683))
    try:
        data, _ = s.recvfrom(1024)
    except socket.timeout:
        data = None
    finally:
        s.close()
    return pkt, data, (time.perf_counter() - inicio) * 1000


async def main(ip):
    ctx = await Context.create_client_context()
    base = f"coap://{ip}"
    lineas = [f"# Resultados de pruebas CoAP — servidor {ip}\n"]

    def log(texto=""):
        print(texto)
        lineas.append(texto)

    # P1
    log("## P1 — GET /sensor")
    r, ms = await pedir(ctx, GET, f"{base}/sensor")
    log(f"- Código: `{r.code}`  |  Payload: `{r.payload.decode()}`  |  Tiempo: {ms:.1f} ms\n")

    # P2
    log("## P2 — POST /led (payload ON)")
    r, ms = await pedir(ctx, POST, f"{base}/led", b"ON")
    log(f"- Código: `{r.code}`  |  Payload: `{r.payload.decode()}`  |  Tiempo: {ms:.1f} ms")
    log("- Revisa el Monitor Serie: debe decir `Payload recibido: ON`\n")

    # P3
    # CoAPSimple responde el 4.04 SIN Token y aiocoap descarta esa
    # respuesta; por eso P3 se arma a mano, sin Token, por UDP crudo.
    log("## P3 — GET /temperatura (ruta inexistente)")
    peticion, respuesta, ms = await asyncio.to_thread(get_crudo, ip, "temperatura")
    if respuesta is None:
        log("- Sin respuesta (timeout)\n")
    else:
        c = respuesta[1]
        log(f"- Código: `{c >> 5}.{c & 0x1F:02d}`  (esperado 4.04 Not Found)  |  Tiempo: {ms:.1f} ms")
        log(f"- Petición ({len(peticion)} bytes): `{peticion.hex(' ')}`")
        log(f"- Respuesta ({len(respuesta)} bytes): `{respuesta.hex(' ')}`\n")

    # P4
    log("## P4 — GET /sensor x5")
    log("| # | Payload | Tiempo (ms) |")
    log("|---|---|---|")
    for i in range(1, 6):
        r, ms = await pedir(ctx, GET, f"{base}/sensor")
        log(f"| {i} | {r.payload.decode()} | {ms:.1f} |")

    await ctx.shutdown()
    with open("resultados_pruebas.md", "w", encoding="utf-8") as f:
        f.write("\n".join(lineas) + "\n")
    print("\nEvidencia guardada en resultados_pruebas.md")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python pruebas_coap.py <IP_de_tu_ESP32>")
        sys.exit(1)
    try:
        asyncio.run(main(sys.argv[1]))
    except asyncio.TimeoutError:
        print("Timeout: revisa que la PC y la ESP32 estén en la misma red "
              "y que el firewall permita UDP 5683 (sección 9 del README).")
