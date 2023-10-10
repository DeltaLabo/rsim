from math import ceil, floor

period = 1.0
delay = 0.4
#period = 0.125
#delay = 0.052
tolerance = 0.1

for n in range(1, 20):
    if abs(ceil(delay * n) - delay * n) <= abs(floor(delay * n) - delay * n):
        if abs(period * ceil(delay * n / period) - delay * n) <= tolerance:
            print(f"n = {n}, period * k = {period * ceil(delay * n / period)}, delay * n = {delay * n}, error = {abs(period * ceil(delay * n / period) - delay * n)}")
            break
    elif abs(ceil(delay * n) - delay * n) > abs(floor(delay * n) - delay * n):
        if abs(period * floor(delay * n / period) - delay * n) <= tolerance:
            print(f"n = {n}, period * k = {period * floor(delay * n / period)}, delay * n = {delay * n}, error = {abs(period * floor(delay * n / period) - delay * n)}")
            break