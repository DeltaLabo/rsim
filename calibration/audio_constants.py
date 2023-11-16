# Frequencies taken from IEC 61672-1 and rounded to one decimal place
iec61672_freqs = (
    50.2, 63.1, 79.4, 100.0, 125.9, 158.5, 199.5, 251.2, 316.2, 
    398.1, 501.2, 631.0, 794.3, 1000.0, 1258.9, 1584.9, 1995.3, # Up to 2 kHz at 1/3-octave
    2238.7, 2511.9, 2818.4, 3162.3, 3548.1, 3984.1, 4466.8, 5011.9, 5623.4, 6309.6, 
    7079.5, 7943.3, # 2 to 8 kHz at 1/6-octave
    8414.0, 8912.5, 9440.6, 10000.0, 10593.0, 11220.0, 11885.0, 12589.0, 13335.0, 14125.0,
    14962.0, 15849.0, 16788.0, 17783.0, 18836.0, 19953.0 # Up to 20 kHz at 1/12-octave
)

# Subset of most representative IEC 61672 frequencies
representative_freqs = (
    79.4, 125.9, 158.5, 199.5, 251.2, 631.0, 1000.0, 1995.3, 5011.9, 7943.3,
    10000.0, 14962.0, 17783.0
)

ten_dB_spaced_volumes = (50, 60, 70, 80, 90) # 50 to 90 dB, in 10 dB intervals
ambient_volumes = (60, 70, 80) # dB
ref_volume = 80 # dB

# Used to convert a dB value to the system volume level requiered to generate it
dB_to_scalar = {
    50: 0.06,
    60: 0.13,
    70: 0.26,
    80: 0.40,
    90: 0.100,
    #TODO: Delete
    5: 0.05,
    6: 0.06,
    7: 0.07
}

freq_response_waves = {
    "frequencies": representative_freqs,
    "volumes": ambient_volumes
}

freq_sweep_waves = {
    "frequencies": representative_freqs,
    "volumes": ambient_volumes
}

freq_weighting_waves = {
    "frequencies": representative_freqs,
    "volumes": ambient_volumes
}