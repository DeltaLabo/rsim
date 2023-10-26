from math import ceil, floor

period = 1.0
delay = 0.4
#period = 0.125
#delay = 0.052
tolerance = 0.2
N = 0
number_of_measurements = 500

for n in range(1, 20):
    if abs(ceil(delay * n) - delay * n) <= abs(floor(delay * n) - delay * n):
        if abs(period * ceil(delay * n / period) - delay * n) <= tolerance:
            print(f"n = {n}, period * m = {period * ceil(delay * n / period)}, delay * n = {delay * n}, error = {abs(period * ceil(delay * n / period) - delay * n)}")
            N = n
            break
    elif abs(ceil(delay * n) - delay * n) > abs(floor(delay * n) - delay * n):
        if abs(period * floor(delay * n / period) - delay * n) <= tolerance:
            print(f"n = {n}, period * m = {period * floor(delay * n / period)}, delay * n = {delay * n}, error = {abs(period * floor(delay * n / period) - delay * n)}")
            N = n
            break

print(f"time multiplier: {(1+N) - N/number_of_measurements}, total time = {period*number_of_measurements*((1+N)-N/number_of_measurements)}")