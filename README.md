# Parallel Octree Construction with MPI

This project demonstrates the parallel construction of an Octree using MPI (Message Passing Interface). The C code distributes a set of 3D points among multiple processes, and each process builds an Octree for its local data. A Python script is included to visualize the results.

## Algorithm

The parallel Octree construction algorithm is implemented as follows:

### 1. Data Distribution

- The root process (rank 0) generates a set of random 3D points.
- These points are then scattered among all available MPI processes using `MPI_Scatter`. Each process receives an equal portion of the data.

### 2. Local Tree Construction

- Each process constructs an Octree for its local set of points. The tree is built recursively:
    - If the number of points in a node exceeds a certain threshold (`MAX_POINTS_PER_NODE`), the node is subdivided into eight children (octants).
    - The points in the parent node are then redistributed among the children based on their spatial location.
    - This process continues until all points are stored in leaf nodes that do not exceed the point capacity.

### 3. Combining Local Trees (Conceptual)

- After each process has built its local Octree, the next step would be to merge these local trees into a single, globally consistent Octree. This is a complex process that is not fully implemented in this demonstration and is left as a conceptual placeholder (`merge_local_trees` function).
- A complete implementation would involve:
    - **Identifying Overlapping Regions:** Determining which nodes in the local trees represent the same spatial region.
    - **Exchanging Node Information:** Communicating node and point data between processes for overlapping regions.
    - **Stitching the Tree:** Connecting the distributed tree parts to form a unified global tree.

## Basic Usage

1. **Compile the C code:**

   ```bash
   make
   ```

2. **Run the parallel executable:**

   ```bash
   mpirun -np 4 ./octree_parallel
   ```

   This will generate data files (`points_data_rank_*.txt` and `octree_data_rank_*.txt`) for each process.

3. **Generate the visualization:**

   ```bash
   python3 plot.py
   ```

   This will create a 3D plot of the distributed data and the local Octrees, saved as `octree_visualization.png`.

4. **Clean up generated files:**

   ```bash
   make clean
   ```
