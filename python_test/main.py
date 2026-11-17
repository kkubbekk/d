import asyncio\
import struct
from bleak import BleakScanner
from bleak import BleakClient


ESP_BLUETOOTH_NAME = "hihi"
ESP_ADRESS = ''
WRITE_UIID = "4A02"

async def scan_for_device():
    devices = await BleakScanner.discover(timeout=5.0)

    for d in devices:
       
        if d.name and ESP_BLUETOOTH_NAME in d.name:
            print(f"Znaleziono obiekt: {d.name} [{d.address}]")
            return d.address
            

async def write_to_esp(address,UIID):
    async with BleakClient(address) as client:
        await client.write_gatt_char(UIID,struct.pack('f',100.0))
        print(f"Model Number: {model_number.decode()}")


async def main():
    ESP_ADRESS = await scan_for_device()

    await write_to_esp(ESP_ADRESS,WRITE_UIID)







MODEL_NBR_UUID = "2A24"


asyncio.run(main())