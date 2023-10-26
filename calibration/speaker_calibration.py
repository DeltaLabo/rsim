import numpy as np
import sounddevice as sd
from ctypes import cast, POINTER
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

# Function to play audio sine waves
def play_audio_sine_wave():
    # Get the default audio playback device
    devices = AudioUtilities.GetSpeakers()
    interface = devices.Activate(
        IAudioEndpointVolume._iid_, CLSCTX_ALL, None)

    # Create a volume control interface
    volume = cast(interface, POINTER(IAudioEndpointVolume))

    # Set the volume (0.5 means 50% of max volume)
    volume.SetMasterVolumeLevelScalar(0.5, None)

    duration = 5  # Duration of each sine wave in seconds
    sample_rate = 88200  # Sample rate in Hz
    t = np.arange(0, duration, 1 / sample_rate)
    sine_wave = np.sin(2 * np.pi * 1000 * t)  # 1 kHz sine wave

    sd.play(sine_wave, sample_rate, blocking=True)

play_audio_sine_wave()