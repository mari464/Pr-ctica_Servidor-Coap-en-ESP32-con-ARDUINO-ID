# led.py - Prende o apaga el LED de la ESP32 por CoAP
# Uso:  python led.py <IP> ON
#       python led.py <IP> OFF
import asyncio
import sys
from aiocoap import Context, Message, POST


async def main(ip, orden):
    ctx = await Context.create_client_context()
    peticion = Message(code=POST, uri=f"coap://{ip}/led", payload=orden.encode())
    respuesta = await asyncio.wait_for(ctx.request(peticion).response, 10)
    print("Código:", respuesta.code)
    print("Respuesta:", respuesta.payload.decode())


if len(sys.argv) != 3 or sys.argv[2].upper() not in ("ON", "OFF"):
    print("Uso: python led.py <IP> ON|OFF")
else:
    asyncio.run(main(sys.argv[1], sys.argv[2].upper()))
