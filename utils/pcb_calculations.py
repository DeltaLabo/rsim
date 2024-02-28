from math import floor

V_BAT = 24.0
h = 7
V_F = 2.9
RDS_on = 3
I = 0.005

R = (V_BAT - h * V_F - RDS_on * I) / I
print(f"R = {R}")

w = 250
h = 300
d = 40

n_x = floor(w/d) + 1
n_y = floor(h/d) + 1

s_x = (w - d * (n_x - 1)) / 2
s_y = (h - d * (n_y - 1)) / 2

print(f"s_x = {s_x}, s_y = {s_y}")
print(f"n_x = {n_x}, n_y = {n_y}, n = {n_x * n_y}")