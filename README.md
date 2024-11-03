# R-Tree Sensor Data Monitoring Program

## File Structure

- **main.c**: The primary code file containing the entire R-tree implementation and user interaction code.
- **sensors**: A folder containing sensor data files (e.g., `sensors_1.txt`, `sensors_2.txt`), each representing sensor readings at different time instants.
- **sensor_nodes.dat** and **bounding_boxes.dat**: Temporary files generated for visualization, containing sensor data and bounding box coordinates.

## Prerequisites

- **C Compiler**: GCC or any other C compiler.
- **Gnuplot**: Used for visualizing sensor data and bounding boxes as heatmaps.

### To Install Gnuplot:

**Ubuntu:**
```bash
sudo apt-get install gnuplot
```

**macOS:**
```bash
brew install gnuplot
``` 

## Compilation and Execution
1. **Compile the code:**
```bash
gcc main.c -o rtree -lm
```
2. **Run the program:**
```bash
./rtree
```

# Using the Application

Upon running the program, you will be presented with a series of interactive options to perform operations such as range queries, fire detection, and data updates. Below is a detailed guide on each option.

# Menu Options

    1.  Perform a Range Query (Option A):
    •   Retrieves environmental data (humidity, pollution level, temperature) within a user-defined rectangular region.
    •   Example: Enter A, then provide coordinates like 0 0 500 500 to query data within this rectangle.
    2.  Detect Fire in a Specified Area (Option B):
    •   Monitors temperature and humidity in a specified circular area centered at a given point, simulating fire spread detection.
    Important Note - The synthetic data is a representation of a real life fire spread scenario which starts at 500, 500 and spreads radially over time.
    •   Example: Enter B, then specify a center point (x, y) and a radius to define the detection area. The program will search for any high-temperature readings within this area and display the results. This also triggers a visualization in gnuplot.
    3.  Update R-tree (Insert/Delete Sensor) (Option C):
    •   Add or delete a sensor in the tree.
    •   Insert: Enter C, then I. Provide coordinates (x, y) and values for humidity, pollution level, and temperature.
    •   Delete: Enter C, then D. Specify coordinates (x, y) of the sensor you wish to remove.
    4.  Load Next Dataset File (Option N):
    •   Loads the next dataset file from the sensors folder (e.g., sensors_2.txt, sensors_3.txt).
    •   This feature allows time-based updates for real-time monitoring.
    •   Example: After loading sensors_1.txt, entering N will load sensors_2.txt, updating the tree with new sensor data.
    5.  Quit Program (Option Q):
    •   Exits the application.

Important Note- The sensors_x files are just used to show that the program can handle multiple datasets. For convenience, when we insert or delete a sensor, changes will not be reflected in the next dataset file as they serve a different purpose. The program is designed to handle real-time monitoring and updates, not historical data changes.

# Data Format for Sensor Files

Each dataset file in the sensors folder (e.g., sensors_1.txt) should follow this format:
- x y humidity pollution_level temperature


# Runthrough

    1.  Run the Program:
    •   Compile and execute as described above.
    •   The program will automatically load sensors_1.txt from the sensors folder.
    2.  Perform a Range Query:
    •   Select option A for a range query.
    •   Input 0 0 500 500 to retrieve sensor data within this bounding box.
    3.  Detect Fire:
    •   Select option B to detect potential fire spread.
    •   Enter center coordinates 500 500 and radius 200. The program will show sensors in this area, and it will generate a heatmap plot.
    4.  Insert a New Sensor:
    •   Select option C and choose I to insert a sensor.
    •   Enter coordinates 150 150 and values humidity=55, pollution_level=65, temperature=85.
    •   The program will update the tree with this new sensor data.
    5.  Load Next Dataset:
    •   Select option N to load sensors_2.txt, updating the R-tree with the next dataset.




