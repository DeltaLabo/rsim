# Frequencies taken from IEC 61672-1 and rounded to one decimal place
iec61672_freqs = (
    50.1, 63.1, 79.4, 100.0, 125.9, 158.5, 199.5, 251.2, 316.2, 
    398.1, 501.2, 631.0, 794.3, 1000.0, 1258.9, 1584.9, 1995.3, # Up to 2 kHz at 1/3-octave
    2238.7, 2511.9, 2818.4, 3162.3, 3548.1, 3984.1, 4466.8, 5011.9, 5623.4, 6309.6, 
    7079.5, 7943.3, # 2 to 8 kHz at 1/6-octave
    8414.0, 8912.5, 9440.6, 10000.0, 10593.0, 11220.0, 11885.0, 12589.0, 13335.0, 14125.0,
    14962.0, 15849.0, 16788.0, 17783.0, 18836.0, 19953.0 # Up to 20 kHz at 1/12-octave
)

linearity_test_waves = {
    "frequencies": (31.5,),
    "volumes": (0.5,)
}

freq_response_waves = {
    "frequencies": iec61672_freqs,
    "volumes": (0.1,)
}

freq_sweep_waves = {
    "frequencies": iec61672_freqs,
    "volumes": (0.1,)
}

freq_weighting_waves = {
    "frequencies": iec61672_freqs,
    "volumes": (0.1,)
}