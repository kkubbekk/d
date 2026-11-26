# import asyncio\
# import struct
# from bleak import BleakScanner
# from bleak import BleakClient


# ESP_BLUETOOTH_NAME = "hihi"
# ESP_ADRESS = ''
# WRITE_UIID = "4A02"

# async def scan_for_device():
#     devices = await BleakScanner.discover(timeout=5.0)

#     for d in devices:
       
#         if d.name and ESP_BLUETOOTH_NAME in d.name:
#             print(f"Znaleziono obiekt: {d.name} [{d.address}]")
#             return d.address
            

# async def write_to_esp(address,UIID):
#     async with BleakClient(address) as client:
#         await client.write_gatt_char(UIID,struct.pack('f',100.0))
#         print(f"Model Number: {model_number.decode()}")


# async def main():
#     ESP_ADRESS = await scan_for_device()

#     await write_to_esp(ESP_ADRESS,WRITE_UIID)







# MODEL_NBR_UUID = "2A24"


# asyncio.run(main())


# chat wyugenerwoal do testow

import asyncio
import struct
from bleak import BleakScanner, BleakClient



ESP_BLUETOOTH_NAME = "hihi"
YOLO_UUID = "4A02"
HOMEGPS_UUID = "4A03"

async def scan_for_device():
    devices = await BleakScanner.discover(timeout=5.0)
    for d in devices:
        if d.name and ESP_BLUETOOTH_NAME in d.name:
            print(f"Znaleziono obiekt: {d.name} [{d.address}]")
            return d.address
    return None

# async def run_test_scenario(address):
#     async with BleakClient(address) as client:
#         print("1. [WAIT_FOR_CALIB -> IDLE] Wysylam kalibracje (base_azimuth = 45.0 deg)...")
#         calib_pkt = struct.pack('<fff?', 50.0600, 19.9400, 45.0, True)
#         await client.write_gatt_char(HOMEGPS_UUID, calib_pkt)
#         await asyncio.sleep(0.2)

#         print("2. [IDLE -> MOVING] Wysylam ruch z YOLO (+10 deg co 100ms, flaga = 0x00)...")
#         for _ in range(15):
#             yolo_pkt = struct.pack('<fB', 10.0, 0x00) # bit 7 = 0 (widzi cel)
#             await client.write_gatt_char(YOLO_UUID, yolo_pkt)
#             await asyncio.sleep(0.1)

#         print("3. [MOVING -> LOST_VISSION] Symuluje zgubienie celu przez YOLO (flaga = 0x80)...")
#         for _ in range(15):
#             yolo_lost_pkt = struct.pack('<fB', 0.0, 0x80) # bit 7 = 1 (zgubil cel!)
#             await client.write_gatt_char(YOLO_UUID, yolo_lost_pkt)
#             await asyncio.sleep(0.1)

#         print("4. [LOST_VISSION -> IDLE] YOLO odzyskuje cel (flaga = 0x00, kat w deadzone 0.1)...")
#         for _ in range(10):
#             yolo_back_pkt = struct.pack('<fB', 0.1, 0x00)
#             await client.write_gatt_char(YOLO_UUID, yolo_back_pkt)
#             await asyncio.sleep(0.1)

#         print("5. [IDLE -> ERROR] Przerywam nadawanie na 1.5 sekundy (timeout > 500ms)...")
#         await asyncio.sleep(1.5)

#         print("6. [ERROR -> IDLE] Wznawiam nadawanie (Auto-Recovery z ERROR)...")
#         for _ in range(10):
#             yolo_recover_pkt = struct.pack('<fB', 5.0, 0x00)
#             await client.write_gatt_char(YOLO_UUID, yolo_recover_pkt)
#             await asyncio.sleep(0.1)

#         print("Test zakonczony!")

import math

async def run_test_scenario(address):
    async with BleakClient(address) as client:
        print("0. Kalibracja (start = 45.0 deg)...")
        calib_pkt = struct.pack('<fff?', 50.0600, 19.9400, 45.0, True)
        await client.write_gatt_char(HOMEGPS_UUID, calib_pkt)
        await asyncio.sleep(0.3)

        # print("1. [8 sekund] Ciagly, plynny ruch w PRAWO (+3.0 deg co 40ms)...")
        # for _ in range(200):
        #     pkt = struct.pack('<fB', 3.0, 0x00)
        #     await client.write_gatt_char(YOLO_UUID, pkt)
        #     await asyncio.sleep(0.04)

        # print("2. [8 sekund] Ciagly, plynny ruch w LEWO (-3.0 deg co 40ms)...")
        # for _ in range(200):
        #     pkt = struct.pack('<fB', -3.0, 0x00)
        #     await client.write_gatt_char(YOLO_UUID, pkt)
        #     await asyncio.sleep(0.04)

        print("3. [10 sekund] Plynna sinusoida prawo <-> lewo (jak idacy czlowiek)...")
        for i in range(250):
            angle_err = 16.0 * math.sin(i * 0.05)
            pkt = struct.pack('<fB', angle_err, 0x00)
            await client.write_gatt_char(YOLO_UUID, pkt, response = False)
            await asyncio.sleep(0.04)

        print("Koniec testu ruchu!")

async def main():
    addr = await scan_for_device()
    if addr:
        await run_test_scenario(addr)
    else:
        print("Nie znaleziono ESP32!")

asyncio.run(main())