#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <math.h>


#define MAX_ENTRIES 4  // Maximum number of children per node

#define MIN_ENTRIES (MAX_ENTRIES / 2) // Minimum number of children per non-root node



// Define a bounding box to cover sensor points
typedef struct BoundingBox {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
} BoundingBox;

// Define a sensor point with environmental data
typedef struct SensorNode {
    int x;
    int y;
    int humidity;
    int PollutionLevel;
    int temperature;
} SensorNode;


// Define an R-tree node (internal or leaf)
typedef struct RTreeNode {
    int is_leaf;              // 1 if leaf, 0 if internal node
    BoundingBox bbox;         // Bounding box covering all children/sensors
    struct RTreeNode **children;  // Child nodes (for internal nodes)
    SensorNode **sensors;     // Sensors (for leaf nodes)
    int num_entries;          // Number of children/sensors in the node
    struct RTreeNode *parent; // Parent node (for backtracking during splits)
} RTreeNode;

// Function prototypes
SensorNode *searchSensorInRTree(RTreeNode *node, SensorNode *target);
BoundingBox *createBoundingBox(int min_x, int min_y, int max_x, int max_y);
BoundingBox createBoundingBoxForSensor(SensorNode *sensor);
RTreeNode *createRTreeNode(int is_leaf);
int area(BoundingBox *bbox);
int overlaps(BoundingBox *a, BoundingBox *b);
void expandToInclude(BoundingBox *a, BoundingBox *b);
void updateBoundingBox(RTreeNode *node);
void insertSensorIntoRTree(RTreeNode *root, SensorNode *sensor);
void splitNode(RTreeNode *node);
void rangeQuery(RTreeNode *node, BoundingBox *query_box, void (*callback)(SensorNode *), int *count);
void printSensor(SensorNode *sensor);
void deleteSensorFromRTree(RTreeNode *node, SensorNode *sensor);

// Create a bounding box for a sensor point
BoundingBox createBoundingBoxForSensor(SensorNode *sensor) {
    BoundingBox bbox;
    bbox.min_x = sensor->x;
    bbox.min_y = sensor->y;
    bbox.max_x = sensor->x;
    bbox.max_y = sensor->y;
    return bbox;
}

// Create a bounding box with specified coordinates
BoundingBox *createBoundingBox(int min_x, int min_y, int max_x, int max_y) {
    BoundingBox *bbox = (BoundingBox *)malloc(sizeof(BoundingBox));
    bbox->min_x = min_x;
    bbox->min_y = min_y;
    bbox->max_x = max_x;
    bbox->max_y = max_y;
    return bbox;
}


// Create a new R-tree node (leaf or internal)
RTreeNode *createRTreeNode(int is_leaf) {
    RTreeNode *node = (RTreeNode *)malloc(sizeof(RTreeNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for RTreeNode.\n");
        exit(EXIT_FAILURE);
    }

    node->is_leaf = is_leaf;
    node->num_entries = 0;
    node->parent = NULL;

    // Allocate memory for children or sensors based on node type
    if (is_leaf) {
        node->children = NULL;  // No children for leaf nodes
        node->sensors = (SensorNode **)malloc(MAX_ENTRIES * sizeof(SensorNode *));
        if (!node->sensors) {
            fprintf(stderr, "Memory allocation failed for sensors in leaf node.\n");
            free(node);
            exit(EXIT_FAILURE);
        }
    } else {
        node->sensors = NULL;  // No sensors for internal nodes
        node->children = (RTreeNode **)malloc(MAX_ENTRIES * sizeof(RTreeNode *));
        if (!node->children) {
            fprintf(stderr, "Memory allocation failed for children in internal node.\n");
            free(node);
            exit(EXIT_FAILURE);
        }
    }
    return node;
}

// Calculate the area of a bounding box
int area(BoundingBox *bbox) {
    return (bbox->max_x - bbox->min_x) * (bbox->max_y - bbox->min_y);
}

// Check if two bounding boxes overlap
int overlaps(BoundingBox *a, BoundingBox *b) {
    return !(a->min_x > b->max_x || a->max_x < b->min_x || a->min_y > b->max_y || a->max_y < b->min_y);
}

// Expand bounding box 'a' to include bounding box 'b'
void expandToInclude(BoundingBox *a, BoundingBox *b) {
    a->min_x = a->min_x < b->min_x ? a->min_x : b->min_x;
    a->min_y = a->min_y < b->min_y ? a->min_y : b->min_y;
    a->max_x = a->max_x > b->max_x ? a->max_x : b->max_x;
    a->max_y = a->max_y > b->max_y ? a->max_y : b->max_y;
}

// Update the bounding box of a node to cover all its children/sensors
void updateBoundingBox(RTreeNode *node) {
    if (node->num_entries == 0) {
        node->bbox = (BoundingBox){INT_MAX, INT_MAX, INT_MIN, INT_MIN};
        return;
    }

    node->bbox = node->is_leaf ? createBoundingBoxForSensor(node->sensors[0]) : node->children[0]->bbox;
    for (int i = 1; i < node->num_entries; i++) {
        if (node->is_leaf) {
            BoundingBox sensor_bbox = createBoundingBoxForSensor(node->sensors[i]);
            expandToInclude(&node->bbox, &sensor_bbox);
        } else {
            expandToInclude(&node->bbox, &node->children[i]->bbox);
        }
    }
}

// Insert a sensor into the R-tree and handle splitting if necessary
void insertSensorIntoRTree(RTreeNode *root, SensorNode *sensor) {
    if (!sensor) {
        fprintf(stderr, "Error: Attempting to insert a NULL sensor.\n");
        return;
    }

    RTreeNode *node = root;

    // Traverse the tree to find the appropriate leaf node
    while (!node->is_leaf) {
        RTreeNode *best_child = node->children[0];
        int min_enlargement = INT_MAX;

        for (int i = 0; i < node->num_entries; i++) {
            BoundingBox expanded_bbox = node->children[i]->bbox;
            BoundingBox sensor_bbox = createBoundingBoxForSensor(sensor);
            expandToInclude(&expanded_bbox, &sensor_bbox);
            int enlargement = area(&expanded_bbox) - area(&node->children[i]->bbox);

            if (enlargement < min_enlargement) {
                min_enlargement = enlargement;
                best_child = node->children[i];
            }
        }

        node = best_child;
    }

    
    if (!node->sensors) {
        fprintf(stderr, "Error: Sensor array is NULL in the leaf node.\n");
        return;
    }

    // Insert sensor into the leaf node if it does not already exist
    node->sensors[node->num_entries] = sensor;
    node->num_entries++;
    updateBoundingBox(node);

    // Handle splitting if the leaf node is full
    if (node->num_entries > MAX_ENTRIES) {
        splitNode(node);
    }
}

//Greene's Split Algorithm for node splitting
void splitNode(RTreeNode *node) {
    // Step 1: Select two seeds (most distant entries)
    int seed1 = 0, seed2 = 1;
    int max_distance = 0;
    for (int i = 0; i < node->num_entries; i++) {
        for (int j = i + 1; j < node->num_entries; j++) {
            int distance = abs(node->sensors[i]->x - node->sensors[j]->x) +
                           abs(node->sensors[i]->y - node->sensors[j]->y);
            if (distance > max_distance) {
                max_distance = distance;
                seed1 = i;
                seed2 = j;
            }
        }
    }
    // Step 2: Create two new nodes and assign seeds
    RTreeNode *node1 = createRTreeNode(node->is_leaf);
    RTreeNode *node2 = createRTreeNode(node->is_leaf);
    node1->sensors[node1->num_entries++] = node->sensors[seed1];
    node2->sensors[node2->num_entries++] = node->sensors[seed2];

    // Step 3: Distribute the remaining entries
    for (int i = 0; i < node->num_entries; i++) {
        if (i == seed1 || i == seed2) continue;
        BoundingBox bbox1 = node1->bbox, bbox2 = node2->bbox;
        BoundingBox sensor_bbox = createBoundingBoxForSensor(node->sensors[i]);

        expandToInclude(&bbox1, &sensor_bbox);
        expandToInclude(&bbox2, &sensor_bbox);

        int enlargement1 = area(&bbox1) - area(&node1->bbox);
        int enlargement2 = area(&bbox2) - area(&node2->bbox);

        if (enlargement1 < enlargement2) {
            node1->sensors[node1->num_entries++] = node->sensors[i];
            node1->bbox = bbox1;
        } else {
            node2->sensors[node2->num_entries++] = node->sensors[i];
            node2->bbox = bbox2;
        }
    }

    // Step 4: Update parent or create a new root if necessary
    if (node->parent) {
        RTreeNode *parent = node->parent;
        parent->children[parent->num_entries++] = node1;
        parent->children[parent->num_entries++] = node2;
        updateBoundingBox(parent);
    } else {
        RTreeNode *new_root = createRTreeNode(0);
        new_root->children[0] = node1;
        new_root->children[1] = node2;
        new_root->num_entries = 2;
        node1->parent = new_root;
        node2->parent = new_root;
    }
}

// Print sensor data during range queries
void printSensor(SensorNode *sensor) {
    printf("Sensor at (%d, %d): Humidity = %d, Pollution Level = %d, Temperature = %d\n",
           sensor->x, sensor->y, sensor->humidity, sensor->PollutionLevel, sensor->temperature);
}
// Perform a range query to find sensors that overlap with the query box
void rangeQuery(RTreeNode *node, BoundingBox *query_box, void (*callback)(SensorNode *), int *count) {
    if (!overlaps(&node->bbox, query_box)) {
        return;  // Skip nodes that don't overlap
    }

    if (node->is_leaf) {
        for (int i = 0; i < node->num_entries; i++) {
            BoundingBox sensor_bbox = createBoundingBoxForSensor(node->sensors[i]);
            if (overlaps(&sensor_bbox, query_box)) {
                callback(node->sensors[i]);
                (*count)++;  // Increment the count for each sensor found
            }
        }
    } else {
        for (int i = 0; i < node->num_entries; i++) {
            rangeQuery(node->children[i], query_box, callback, count);
        }
    }
}

// Helper function to find a sibling node
RTreeNode* findSibling(RTreeNode *node) {
    if (!node->parent) return NULL;
    for (int i = 0; i < node->parent->num_entries; i++) {
        if (node->parent->children[i] != node) {
            return node->parent->children[i];
        }
    }
    return NULL;
}

// Updated delete function with underflow handling
void deleteSensorFromRTree(RTreeNode *node, SensorNode *sensor) {
    if (node->is_leaf) {
        // Find and remove the sensor in the leaf node
        int found = 0;
        for (int i = 0; i < node->num_entries; i++) {
            if (node->sensors[i]->x == sensor->x && node->sensors[i]->y == sensor->y) {
                found = 1;
                // Free the sensor node if dynamically allocated
                free(node->sensors[i]);
                // Shift sensors to fill the gap
                for (int j = i; j < node->num_entries - 1; j++) {
                    node->sensors[j] = node->sensors[j + 1];
                }
                node->num_entries--;
                break;
            }
        }

        if (!found) {
            printf("Sensor not found in the tree.\n");
            return;
        }

        // Update the bounding box after deletion
        updateBoundingBox(node);

        // Check for underflow
        if (node->num_entries < MIN_ENTRIES && node->parent != NULL) {
            // Attempt to borrow from siblings
            RTreeNode *sibling = findSibling(node);
            if (sibling && sibling->num_entries > MIN_ENTRIES) {
                // Borrow a sensor from the sibling
                node->sensors[node->num_entries++] = sibling->sensors[sibling->num_entries - 1];
                sibling->num_entries--;
                updateBoundingBox(node);
                updateBoundingBox(sibling);
            } else if (sibling) {
                // Merge with sibling
                for (int i = 0; i < sibling->num_entries; i++) {
                    node->sensors[node->num_entries++] = sibling->sensors[i];
                }
                node->parent->num_entries--;
                // Remove sibling from parent
                for (int i = 0; i < node->parent->num_entries; i++) {
                    if (node->parent->children[i] == sibling) {
                        for (int j = i; j < node->parent->num_entries; j++) {
                            node->parent->children[j] = node->parent->children[j + 1];
                        }
                        break;
                    }
                }
                free(sibling);
                updateBoundingBox(node->parent);
            }
        }
    } else {
        // Traverse internal nodes to find the sensor in leaf nodes
        for (int i = 0; i < node->num_entries; i++) {
            BoundingBox sensor_bbox = createBoundingBoxForSensor(sensor);
            if (overlaps(&node->children[i]->bbox, &sensor_bbox)) {
                deleteSensorFromRTree(node->children[i], sensor);

                // After deletion, check if the child has underflowed
                RTreeNode *child = node->children[i];
                if (child->num_entries < MIN_ENTRIES) {
                    RTreeNode *sibling = findSibling(child);
                    if (sibling && sibling->num_entries > MIN_ENTRIES) {
                        // Borrow a child from the sibling
                        node->children[i] = sibling->children[sibling->num_entries - 1];
                        sibling->children[sibling->num_entries - 1]->parent = node;
                        sibling->num_entries--;
                        updateBoundingBox(child);
                        updateBoundingBox(sibling);
                        updateBoundingBox(node);
                    } else if (sibling) {
                        // Merge child with sibling
                        for (int j = 0; j < sibling->num_entries; j++) {
                            node->children[i]->children[node->children[i]->num_entries++] = sibling->children[j];
                            sibling->children[j]->parent = node->children[i];
                        }
                        node->num_entries--;
                        // Remove sibling from parent
                        for (int j = i; j < node->num_entries; j++) {
                            node->children[j] = node->children[j + 1];
                        }
                        free(sibling);
                        updateBoundingBox(node->children[i]);
                        updateBoundingBox(node);
                    }
                }

                return;
            }
        }
    }
}
// Search for a sensor with specific coordinates in the R-tree
SensorNode *searchSensorInRTree(RTreeNode *node, SensorNode *target) {
    
    BoundingBox targetBBox = createBoundingBoxForSensor(target);
    if (!overlaps(&node->bbox, &targetBBox)) {
        return NULL;  // Skip nodes that don't overlap
   
    }
    if (node->is_leaf) {
        // Look for a matching sensor in the leaf node
        for (int i = 0; i < node->num_entries; i++) {
            if (node->sensors[i]->x == target->x && node->sensors[i]->y == target->y) {
                return node->sensors[i];  
            }
        }
    } else {
        // Traverse child nodes for internal nodes
        for (int i = 0; i < node->num_entries; i++) {
            SensorNode *result = searchSensorInRTree(node->children[i], target);
            if (result != NULL) {
                return result;  // Found sensor in a child node
            }
        }
    }
    return NULL;  // Sensor not found in this branch
}

// Function to write sensor nodes to a file for heatmap plotting
void writeSensorNodesToFile(RTreeNode *node, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file %s for writing sensor nodes.\n", filename);
        return;
    }

    if (node->is_leaf) {
        for (int i = 0; i < node->num_entries; i++) {
            fprintf(file, "%d %d %d\n", node->sensors[i]->x, node->sensors[i]->y, node->sensors[i]->temperature);
        }
    } else {
        for (int i = 0; i < node->num_entries; i++) {
            writeSensorNodesToFile(node->children[i], filename);
        }
    }

    fclose(file);
}

void writeBoundingBoxesToFile(RTreeNode *node, const char *filename) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("Error opening file %s for writing bounding boxes.\n", filename);
        return;
    }

    if (node->is_leaf || node->num_entries > 0) {
        
        fprintf(file, "%d %d\n", node->bbox.min_x, node->bbox.min_y);  // Bottom-left
        fprintf(file, "%d %d\n", node->bbox.max_x, node->bbox.min_y);  // Bottom-right
        fprintf(file, "%d %d\n", node->bbox.max_x, node->bbox.max_y);  // Top-right
        fprintf(file, "%d %d\n", node->bbox.min_x, node->bbox.max_y);  // Top-left
        fprintf(file, "%d %d\n\n", node->bbox.min_x, node->bbox.min_y);  // Close the rectangle
    }

    if (!node->is_leaf) {
        for (int i = 0; i < node->num_entries; i++) {
            writeBoundingBoxesToFile(node->children[i], filename);
        }
    }

    fclose(file);
}


// Main function
int main() {
    // Initialize R-Tree root node (leaf node by default)
    RTreeNode *root = createRTreeNode(1);

    // File loading setup
    const char *folder = "sensors";
    int file_index = 1;
    char file_path[256];
    sprintf(file_path, "%s/sensors_%d.txt", folder, file_index);

    // Attempt to load the first dataset
    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open initial dataset file %s.\n", file_path);
        return EXIT_FAILURE;
    }

    // Load sensor data into R-Tree
    int x, y, humidity, PollutionLevel, temperature;
    printf("Loading data from %s...\n", file_path);
    while (fscanf(file, "%d %d %d %d %d", &x, &y, &humidity, &PollutionLevel, &temperature) != EOF) {
        SensorNode *sensor = (SensorNode *)malloc(sizeof(SensorNode));
        if (!sensor) {
            fprintf(stderr, "Memory allocation failed for sensor node.\n");
            fclose(file);
            return EXIT_FAILURE;
        }
        sensor->x = x;
        sensor->y = y;
        sensor->humidity = humidity;
        sensor->PollutionLevel = PollutionLevel;
        sensor->temperature = temperature;
        insertSensorIntoRTree(root, sensor);
    }
    fclose(file);
    printf("Data loaded successfully.\n");

    // Interactive menu loop
    char option;
    while (1) {
        // Display menu options
        printf("\nOptions:\n");
        printf("  A - Perform a range query\n");
        printf("  B - Detect fire in a specified area\n");
        printf("  C - Update R-tree (Insert/Delete sensor)\n");
        printf("  N - Load the next dataset file\n");
        printf("  Q - Quit the program\n");
        printf("Enter your choice: ");
        scanf(" %c", &option);

        // Process user option
        if (option == 'Q') {
            printf("Exiting program.\n");
            break;
        }
        else if (option == 'A') {
        // Perform a range query
        int min_x, min_y, max_x, max_y;
        printf("Enter range query coordinates (min_x min_y max_x max_y): ");
        scanf("%d %d %d %d", &min_x, &min_y, &max_x, &max_y);
        BoundingBox *query_box = createBoundingBox(min_x, min_y, max_x, max_y);
        printf("Performing range query...\n");

        int count = 0; 
        rangeQuery(root, query_box, printSensor, &count);

        if (count == 0) {
            printf("No nodes in the range given.\n");
        }
    
        free(query_box);
        }
        else if (option == 'B') {
            // Fire detection in a specified area
            int center_x, center_y, radius;
            printf("Enter center coordinates (x, y) and detection radius: ");
            scanf("%d %d %d", &center_x, &center_y, &radius);

            // Define the bounding box based on user input
            BoundingBox *query_box = createBoundingBox(center_x - radius, center_y - radius, center_x + radius, center_y + radius);
            int count=0;
            // Perform the range query and visualize
            printf("Detecting fire in area (%d, %d) with radius %d...\n", center_x, center_y, radius);
            rangeQuery(root, query_box, printSensor,&count);

            // Output query bounding box to file for visualization
            FILE *file = fopen("bounding_boxes.dat", "w");
            if (file) {
                fprintf(file, "%d %d\n", query_box->min_x, query_box->min_y);  // Bottom-left
                fprintf(file, "%d %d\n", query_box->max_x, query_box->min_y);  // Bottom-right
                fprintf(file, "%d %d\n", query_box->max_x, query_box->max_y);  // Top-right
                fprintf(file, "%d %d\n", query_box->min_x, query_box->max_y);  // Top-left
                fprintf(file, "%d %d\n\n", query_box->min_x, query_box->min_y);  // Close the rectangle
                fclose(file);
            }

            // Write sensor nodes to file and display plot
            writeSensorNodesToFile(root, "sensor_nodes.dat");
            system("gnuplot -persist -e \"set terminal qt; \
    set xlabel 'X'; \
    set ylabel 'Y'; \
    set cblabel 'Temperature (°C)'; \
    set cbrange [30:100]; \
    set palette defined (30 'blue', 40 'cyan', 60 'green', 80 'yellow', 100 'red'); \
    set xrange [0:1000]; \
    set yrange [0:1000]; \
    plot 'sensor_nodes.dat' using 1:2:3 with points pt 7 ps 1 palette notitle, \
    'bounding_boxes.dat' using 1:2 with lines lw 2 lc rgb 'black' notitle\""); 
            free(query_box);
        }
        else if (option == 'C') {
            // Update R-tree: Insert or delete a sensor
            char update_option;
            printf("Enter I to insert a new sensor or D to delete an existing sensor: ");
            scanf(" %c", &update_option);

            if (update_option == 'I') {
                SensorNode *sensor = (SensorNode *)malloc(sizeof(SensorNode));
                if (!sensor) {
                    fprintf(stderr, "Memory allocation failed for new sensor.\n");
                    continue;
                }
                printf("Enter coordinates (x, y) of the new sensor: ");
                scanf("%d %d", &sensor->x, &sensor->y);
                printf("Enter humidity, pollution level, and temperature: ");
                scanf("%d %d %d", &sensor->humidity, &sensor->PollutionLevel, &sensor->temperature);
                insertSensorIntoRTree(root, sensor);
                printf("Sensor inserted successfully.\n");
            }
            else if (update_option == 'D') {
                SensorNode sensor;
                printf("Enter coordinates (x, y) of the sensor to delete: ");
                scanf("%d %d", &sensor.x, &sensor.y);
                deleteSensorFromRTree(root, &sensor);
                printf("Sensor deleted successfully (if it existed).\n");
            }
            else {
                printf("Invalid option. Please enter 'I' or 'D'.\n");
            }
        }
        else if (option == 'N') {
            // Load the next dataset file
            file_index++;
            sprintf(file_path, "%s/sensors_%d.txt", folder, file_index);
            file = fopen(file_path, "r");

            if (!file) {
                printf("No more files available.\n");
                file_index--;              }
            else {
                printf("Loading data from %s...\n", file_path);
                while (fscanf(file, "%d %d %d %d %d", &x, &y, &humidity, &PollutionLevel, &temperature) != EOF) {
                    SensorNode temp_sensor = {x, y, humidity, PollutionLevel, temperature};
                    SensorNode *existing_sensor = searchSensorInRTree(root, &temp_sensor);
                    if (existing_sensor) {
                        existing_sensor->humidity = humidity;
                        existing_sensor->PollutionLevel = PollutionLevel;
                        existing_sensor->temperature = temperature;
                    }
                }
                fclose(file);
                printf("Data from %s loaded successfully.\n", file_path);
            }
        }
        else {
            printf("Invalid option. Please enter a valid command.\n");
        }
    }

    return EXIT_SUCCESS;
}

