import asyncio
from bleak import BleakClient

# BLE device UUIDs
device_address = "48:27:E2:E6:DC:85"
service_uuid = "4d2b9a73-a822-487f-a846-3933abdcfcd9"
characteristic_uuid = "5d4bb853-b89f-48b8-9a38-4fd51ab116f7"

async def notification_handler(sender: int, data: bytearray):
    print(str(data))

async def receive_ble_notifications(address, service_uuid, characteristic_uuid):
    async with BleakClient(address) as client:
        # Enable notifications for the specified characteristic
        await client.start_notify(characteristic_uuid, notification_handler)

        # Keep the script running to receive notifications
        await asyncio.sleep(2)  # Await for 2 seconds

        # Stop notifications when done
        await client.stop_notify(characteristic_uuid)

# Run the asyncio event loop
loop = asyncio.get_event_loop()
loop.run_until_complete(receive_ble_notifications(device_address, service_uuid, characteristic_uuid))
