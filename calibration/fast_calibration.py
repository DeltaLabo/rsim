import threading
import time
import csv
import numpy as np
import sounddevice as sd
import serial
from ctypes import cast, POINTER
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

SLM_COM_PORT = "COMX"
SLM_BAUDRATE = 115200
SLM_PERIOD = 0.125

# Function to receive serial data and store it
def receive_serial_data(data):
    ser = serial.Serial(SLM_COM_PORT, SLM_BAUDRATE)  # Replace 'COM1' with your serial port
    while True:
        received_data = float(ser.readline().decode().strip())
        data.append(received_data)

# Function to play audio sine waves
def play_audio_sine_wave():
    # Get the default audio playback device
    devices = AudioUtilities.GetSpeakers()
    interface = devices.Activate(
        IAudioEndpointVolume._iid_, CLSCTX_ALL, None)

    # Create a volume control interface
    volume_controller = cast(interface, POINTER(IAudioEndpointVolume))
    volume_controller.SetMasterVolumeLevelScalar(volume, None)
    
    duration = 10  # Duration of each sine wave in seconds
    sample_rate = 88200  # Sample rate in Hz
    t = np.arange(0, duration, 1 / sample_rate)
    sine_wave = 0.5 * np.sin(2 * np.pi * frequency * t)  # 440 Hz sine wave as an example

    while True:
        print("Playing audio sine wave for 10 seconds")
        sd.play(sine_wave, sample_rate)
        sd.wait()
        time.sleep(10)  # Pause for 10 seconds

# Create a list to store received serial data
serial_data = []

# Create the serial data receiving thread
serial_thread = threading.Thread(target=receive_serial_data, args=(serial_data,))
serial_thread.daemon = True

# Create the audio playback thread
audio_thread = threading.Thread(target=play_audio_sine_wave)
audio_thread.daemon = True

# Start both threads
serial_thread.start()
audio_thread.start()

try:
    while True:
        time.sleep(1)
        # You can save serial_data to a CSV file or process it as needed
except KeyboardInterrupt:
    print("Program terminated.")
