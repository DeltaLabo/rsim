import asyncio
from bleak import BleakClient

async def read_ble_characteristic(address, service_uuid, characteristic_uuid):
    async with BleakClient(address) as client:
        # Read the value of the specified characteristic
        value = await client.read_gatt_char(characteristic_uuid)
        print(f"Characteristic Value: {value}")

# Replace these with your BLE device information
device_address = "48:27:E2:E6:DC:85"
service_uuid = "4d2b9a73-a822-487f-a846-3933abdcfcd9"  # Example UUID for Battery Service
characteristic_uuid = "5d4bb853-b89f-48b8-9a38-4fd51ab116f7"  # Example UUID for Battery Level

# Run the asyncio event loop
loop = asyncio.get_event_loop()
loop.run_until_complete(read_ble_characteristic(device_address, service_uuid, characteristic_uuid))
