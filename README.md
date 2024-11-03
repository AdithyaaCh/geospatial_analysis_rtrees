# geospatial_analysis_rtrees

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



