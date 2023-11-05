import numpy as np
import matplotlib.pyplot as plt
from ordered_set import OrderedSet

class DataContainer(dict):
    # Format the data container for the specified test waves
    def __init__(self, test_waves, *args, **kwargs):
        super(DataContainer, self).__init__(*args, **kwargs)

        self.frequencies = OrderedSet()
        self.volumes = OrderedSet()
        self["all"] = np.array([])

        for freq in test_waves["frequencies"]:
            self.frequencies.add(freq)
            self[freq] = {}
            self[freq]["freq_all"] = np.array([])

            for volume in test_waves["volumes"]:
                self.volumes.add(volume)
                self[freq][volume] = np.array([])

# Plot the sound level progression with respect to time, for each frequency
def plot_time_progression(measurements, reference, units, title, TIME_PERIOD):
    for freq in measurements.frequencies:
        # Create x-axis values
        x_axis = [x * TIME_PERIOD for x in range(len(measurements[freq]["freq_all"]))]

        # Create a plot
        plt.plot(x_axis, measurements[freq]["freq_all"], label="ESP")
        plt.plot(x_axis, reference[freq]["freq_all"], label="SLM")

        # Add labels and a title (optional)
        plt.xlabel("s")
        plt.ylabel(units)
        plt.title(f"Sound level time progression @ {freq} Hz" + title)
        plt.legend()

        # Show the plot
        plt.show()

# Plot color map
def color_map(measurements, reference, function, title, function_units, weighted):
    # Create an empty matrix to store MAE values
    matrix = np.zeros((len(measurements.frequencies), len(measurements.volumes)))

    # Calculate MAE for all pairs of sets
    for i, freq in enumerate(measurements.frequencies):
        for j, volume in enumerate(measurements.volumes):
            matrix[i, j] = function(
                measurements[freq][volume],
                reference[freq][volume]
            )

    # Plot the MAE matrix as a colormap
    img = plt.imshow(matrix, cmap='viridis', interpolation='nearest')
    plt.xticks(range(len(measurements.volumes)), measurements.volumes)
    plt.yticks(range(len(measurements.frequencies)), measurements.frequencies)
    plt.xlabel("dBA" if weighted else "dB")
    plt.ylabel("Hz")
    cbar = plt.colorbar()
    cbar.set_label(function_units)
    plt.title(title)
    plt.show()

def calculate_mae(measurements, reference):
    if len(measurements) != len(reference):
        raise ValueError("Both sets must have the same length")
    mae = np.mean(np.abs(measurements - reference))
    return mae

def calculate_rmse(measurements, reference):
    if len(measurements) != len(reference):
        raise ValueError("Input arrays must have the same length.")
    # Calculate the squared differences
    squared_diff = (measurements - reference) ** 2
    # Calculate the mean of squared differences
    mean_squared_diff = np.mean(squared_diff)
    # Calculate the square root of the mean squared differences (RMSE)
    rmse = np.sqrt(mean_squared_diff)
    return rmse

def calculate_gain(measurements, reference):
    if len(measurements) != len(reference):
        raise ValueError("Both sets must have the same length")
    gain = np.mean(measurements - reference)
    return gain

# Plot the frequency response of the measurements with respect to the reference, for the reference volume level
def plot_ref_gain(measurements, reference, ref_volume, units, title):
    gain = np.array([])

    for freq in measurements.frequencies:
        gain = np.append(gain, calculate_gain(measurements[freq][ref_volume], reference[freq][ref_volume]))

    # Create x-axis values
    x_axis = measurements.frequencies

    # Create a plot
    plt.plot(x_axis, gain)

    # Add labels and a title (optional)
    plt.xlabel("Hz")
    plt.xticks(measurements.frequencies, measurements.frequencies)
    plt.ylabel(f"Gain ({units})")
    plt.title(title)

    # Show the plot
    plt.show()

# Plot the MAE and RMSE of the measurements with respect to the reference, for the reference volume level
def plot_ref_mae_rmse(measurements, reference, ref_volume, units, title):
    mae = np.array([])
    rmse = np.array([])

    for freq in measurements.frequencies:
        mae = np.append(mae, calculate_mae(measurements[freq][ref_volume], reference[freq][ref_volume]))
        rmse = np.append(rmse, calculate_rmse(measurements[freq][ref_volume], reference[freq][ref_volume]))

    # Create x-axis values
    x_axis = measurements.frequencies

    # Create a plot
    plt.plot(x_axis, mae, label="MAE")
    plt.plot(x_axis, rmse, label="RMSE")

    # Add labels and a title (optional)
    plt.xlabel("Hz")
    plt.xticks(measurements.frequencies, measurements.frequencies)
    plt.ylabel(units)
    plt.title(title)
    plt.legend()

    # Show the plot
    plt.show()

# Calculate the standard deviation of a set of measurements
def calculate_stdev(measurements):
    return np.std(measurements)

# Plot and print the results of the ambient noise test
def plot_ambient_results(measurements, reference, TIME_PERIOD):
    for volume in ambient_volumes:
        plot_time_progression(measurements[volume], reference[volume], "dBA", f" @ {volume} dB", TIME_PERIOD)
        print(f"Volume: {volume} dB, MAE: {calculate_mae(measurements[volume], reference[volume])} dBA, RMSE: {calculate_rmse(measurements[volume], reference[volume])} dBA")

def calculate_offset(measurements, reference):
    if len(measurements) != len(reference):
        raise ValueError("Both sets must have the same length")
    offset = np.mean(measurements - reference)
    return offset