import numpy as np
import sounddevice as sd
import time

# Function to continously play audio sine waves
def play_audio_sine_wave():
    frequency = 1000 # Hz
    duration = 600  # Duration of each sine wave in seconds
    sample_rate = 88200  # Sample rate in Hz
    t = np.arange(0, duration, 1 / sample_rate)
    sine_wave = np.sin(2 * np.pi * frequency * t)  # 1 kHz sine wave

    while True:
        sd.play(sine_wave, sample_rate, blocking=True)

play_audio_sine_wave()