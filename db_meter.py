import tkinter as tk
import serial
import threading

# Upper limit of the "green" category
MAX_GREEN = 50.0 # dB
# Upper limit of the "yellow" category
MAX_YELLOW = 60.0

# Set the serial port and baud rate here
serial_port = "COM3"  # Replace with your desired serial port
baudrate = 112500

data = 0
units = ""

def noise2color(data):
    if data < MAX_GREEN:
        label.config(fg="green")
    elif MAX_GREEN <= data < MAX_YELLOW:
        label.config(fg="yellow")
    else:
        label.config(fg="red")

def read_serial_data(serial_port, baudrate, text_var):
    try:
        with serial.Serial(serial_port, baudrate=baudrate, timeout=1) as ser:
            while True:
                line = ser.readline().decode().strip()

                try:
                    # Split the data and the units
                    data, units = line.split(" ")
                    # Convert the input string to a float
                    data = float(data)
                    text_var.set(line)
                    # Change the text color based on the value
                    noise2color(data)
                # Catch the error that throws when the input is occasionally corrupted
                except ValueError:
                    data = 0.0

    except serial.SerialException as e:
        text_var.set("Error: " + str(e))

# Set the desired text color here
text_color = "black"

root = tk.Tk()
root.title("Serial Data Reader")

text_var = tk.StringVar()
label = tk.Label(root, textvariable=text_var, wraplength=600, fg="black", font=("Arial", 150))  # Increase the font size here
label.pack(padx=10, pady=10)

# Start reading serial data in a separate thread
serial_thread = threading.Thread(target=read_serial_data, args=(serial_port, baudrate, text_var))
serial_thread.daemon = True
serial_thread.start()

root.mainloop()
