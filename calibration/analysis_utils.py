import numpy as np
import matplotlib.pyplot as plt
from ordered_set import OrderedSet

# Custom imports
from audio_constants import ambient_volumes

# Data container for the tests that perform a sweep across multiple frequencies and volumes
class DataContainer(dict):
    # Format the data container for the specified test waves
    def __init__(self, test_waves, *args, **kwargs):
        super(DataContainer, self).__init__(*args, **kwargs)

        # Create iterable lists of all the frequencies and volumes contained
        self.frequencies = OrderedSet()
        self.volumes = OrderedSet()

        # Populate the iterables
        for freq in test_waves["frequencies"]:
            self.frequencies.add(freq)
            self[freq] = {}

            for volume in test_waves["volumes"]:
                self.volumes.add(volume)
                self[freq][volume] = np.array([])

# Plot the sound level progression with respect to time
def plot_time_progression(measurements, reference, units, title, TIME_PERIOD):
    try:
        # Iterate through the container's frequency list
        for freq in measurements.frequencies:
            # Arrays to store all measurements taken at a certain frequency,
            # across multiple volumes
            # Measurements refers to the data collected with the Device Under Test,
            # while the reference is an independently calibrated device
            measurements_freq_all = np.array([])
            reference_freq_all = np.array([])

            # Accumulate the data points across all volumes
            for volume in measurements.volumes:
                measurements_freq_all = np.append(measurements_freq_all, measurements[freq][volume])
                reference_freq_all = np.append(reference_freq_all, reference[freq][volume])

            # Create x-axis values
            x_axis = [x * TIME_PERIOD for x in range(len(measurements_freq_all))]

            # Create a plot
            plt.plot(x_axis, measurements_freq_all, label="ESP")
            plt.plot(x_axis, reference_freq_all, label="SLM")

            # Add labels and a title
            plt.xlabel("s")
            plt.ylabel(units)
            plt.title(f"Nivel de sonido con respecto al tiempo @ {freq} Hz" + title)
            plt.legend()

            plt.tight_layout()  # Adjust layout for better spacing
            # Show the plot
            plt.show()
    # Some data containers store data belonging to a single frequency and volume,
    # so iterating through the frequency list will fail
    except AttributeError:
        # Create x-axis values
        # In this case we don't need to accumulate the values for all volumes
        x_axis = [x * TIME_PERIOD for x in range(len(measurements))]

        # Create a plot
        if reference is not None:
            plt.plot(x_axis, measurements, label="ESP")
            plt.plot(x_axis, reference, label="SLM")
            plt.legend()
        # Some data sets do not have a reference
        else:
            plt.plot(x_axis, measurements)

        # Add labels and a title
        plt.xlabel("s")
        plt.ylabel(units)
        plt.title(f"Nivel de sonido con respecto al tiempo" + title)

        plt.tight_layout()  # Adjust layout for better spacing
        # Show the plot
        plt.show()

# Applies the function to the measurements and reference data containers, and
# creates a color map of the results
def color_map(measurements, reference, function, title, function_units, weighted):
    # Create an empty matrix to store values
    # Set the size so there's a matrix entry for every frequency-volume pair
    matrix = np.zeros((len(measurements.frequencies), len(measurements.volumes)))

    # Apply functoin to calculate value for all pairs
    for i, freq in enumerate(measurements.frequencies):
        for j, volume in enumerate(measurements.volumes):
            matrix[i, j] = function(
                measurements[freq][volume],
                reference[freq][volume]
            )

    # Plot the MAE matrix as a colormap
    img = plt.imshow(matrix, cmap='viridis', interpolation='nearest')
    # Add x- and y-axis ticks to show the corresponding frequencies and volumes
    plt.xticks(range(len(measurements.volumes)), measurements.volumes)
    plt.yticks(range(len(measurements.frequencies)), measurements.frequencies)
    # Add units as axis labels
    plt.xlabel("dBA" if weighted else "dB")
    plt.ylabel("Hz")
    # Add a color bar to interpret the values
    cbar = plt.colorbar()
    # Add the corresponding units to the color bar
    cbar.set_label(function_units)
    # Add the title
    plt.title(title)
    # Show the color map
    plt.show()

# Calculate Mean Average Error (MAE) between two data sets
def calculate_mae(measurements, reference):
    # The sets need to be the same size to calculate the MAE
    if len(measurements) != len(reference):
        # Raise an error if they have different sizes
        raise ValueError("Both sets must have the same length")

    # Calculate the MAE
    mae = np.mean(np.abs(measurements - reference))
    return mae

# Calculate Root Mean Square Error (RMSE) between two data sets
def calculate_rmse(measurements, reference):
    # The sets need to be the same size to calculate the MAE
    if len(measurements) != len(reference):
        # Raise an error if they have different sizes
        raise ValueError("Both sets must have the same length")

    # Calculate the squared differences
    squared_diff = (measurements - reference) ** 2
    # Calculate the mean of squared differences
    mean_squared_diff = np.mean(squared_diff)
    # Calculate the square root of the mean squared differences (RMSE)
    rmse = np.sqrt(mean_squared_diff)
    return rmse

# Calculate mean gain between two data sets
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

    plt.xscale("log")
    plt.xticks([100.0, 251.2, 631.0, 1000.0, 5011.9, 10000.0, 14962.0, 17783.0], ["100", "251", "631", "1k", "5k", "10k", "15k", "18k"])
    plt.tight_layout()  # Adjust layout for better spacing
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

    plt.xscale("log")
    plt.xticks([100.0, 251.2, 631.0, 1000.0, 5011.9, 10000.0, 14962.0, 17783.0], ["100", "251", "631", "1k", "5k", "10k", "15k", "18k"])
    plt.tight_layout()  # Adjust layout for better spacing
    # Show the plot
    plt.show()

# Calculate the standard deviation of a set of measurements
def calculate_stdev(measurements):
    return np.std(measurements)

# Plot and print the results of the ambient noise test
def plot_ambient_results(measurements, reference, TIME_PERIOD):
    for volume in (70, 80):
        plot_time_progression(measurements[volume], reference[volume], "dBA", f" @ {volume} dB, T={'F' if TIME_PERIOD == 0.125 else 'S'}", TIME_PERIOD)

def print_ambient_results(measurements, reference, TIME_PERIOD):
    for volume in (70, 80):
        print(f"Volumen: {volume} dB, MAE: {calculate_mae(measurements[volume], reference[volume])} dBA, RMSE: {calculate_rmse(measurements[volume], reference[volume])} dBA")

def calculate_offset(measurements, reference):
    if len(measurements) != len(reference):
        raise ValueError("Both sets must have the same length")
    offset = np.mean(reference - measurements)
    return offset