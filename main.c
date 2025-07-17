#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_POINTS_PER_NODE 4

// Represents a 3D point
typedef struct {
    double x, y, z;
} Point;

// Represents a node in the Octree
typedef struct OctreeNode {
    Point center;
    double size;
    Point* points;
    int num_points;
    struct OctreeNode* children[8];
} OctreeNode;

// Forward declarations
void insert_point(OctreeNode* node, Point point);
void subdivide_node(OctreeNode* node);
void write_octree_to_file(OctreeNode* node, FILE* file);

// Function to create a new Octree node
OctreeNode* create_node(Point center, double size) {
    OctreeNode* node = (OctreeNode*)malloc(sizeof(OctreeNode));
    node->center = center;
    node->size = size;
    node->points = (Point*)malloc(MAX_POINTS_PER_NODE * sizeof(Point));
    node->num_points = 0;
    for (int i = 0; i < 8; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Function to determine which child a point belongs to
int get_octant(Point center, Point point) {
    int octant = 0;
    if (point.x >= center.x) octant |= 1;
    if (point.y >= center.y) octant |= 2;
    if (point.z >= center.z) octant |= 4;
    return octant;
}

// Function to insert a point into the Octree
void insert_point(OctreeNode* node, Point point) {
    if (node->children[0] != NULL) {
        int octant = get_octant(node->center, point);
        insert_point(node->children[octant], point);
        return;
    }

    if (node->num_points < MAX_POINTS_PER_NODE) {
        node->points[node->num_points++] = point;
        return;
    }

    subdivide_node(node);
    for (int i = 0; i < node->num_points; i++) {
        int octant = get_octant(node->center, node->points[i]);
        insert_point(node->children[octant], node->points[i]);
    }
    int octant = get_octant(node->center, point);
    insert_point(node->children[octant], point);

    free(node->points);
    node->points = NULL;
    node->num_points = 0;
}

// Function to subdivide a node into 8 children
void subdivide_node(OctreeNode* node) {
    double child_size = node->size / 2.0;
    for (int i = 0; i < 8; i++) {
        Point child_center = node->center;
        child_center.x += (i & 1) ? child_size / 2.0 : -child_size / 2.0;
        child_center.y += (i & 2) ? child_size / 2.0 : -child_size / 2.0;
        child_center.z += (i & 4) ? child_size / 2.0 : -child_size / 2.0;
        node->children[i] = create_node(child_center, child_size);
    }
}

// Function to write Octree data to a file for visualization
void write_octree_to_file(OctreeNode* node, FILE* file) {
    if (node == NULL) {
        return;
    }
    fprintf(file, "%f %f %f %f\n", node->center.x, node->center.y, node->center.z, node->size);
    for (int i = 0; i < 8; i++) {
        write_octree_to_file(node->children[i], file);
    }
}

// Function to free the memory of the Octree
void free_octree(OctreeNode* node) {
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        free_octree(node->children[i]);
    }
    if (node->points) {
        free(node->points);
    }
    free(node);
}

// Placeholder for the complex logic of merging local Octrees
void merge_local_trees(OctreeNode* local_tree, int rank, int size) {
    // This is a complex process that involves exchanging node information
    // between processes to build a globally consistent Octree.
    if (rank == 0) {
        printf("Placeholder: Merging of local trees would happen here.\n");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Point* all_points = NULL;
    int total_points = 1000;

    if (rank == 0) {
        all_points = (Point*)malloc(total_points * sizeof(Point));
        printf("Generating %d random points on the root process.\n", total_points);
        for (int i = 0; i < total_points; i++) {
            all_points[i] = (Point){drand48(), drand48(), drand48()};
        }
    }

    int points_per_process = total_points / size;
    Point* local_points = (Point*)malloc(points_per_process * sizeof(Point));

    MPI_Datatype mpi_point_type;
    MPI_Type_contiguous(3, MPI_DOUBLE, &mpi_point_type);
    MPI_Type_commit(&mpi_point_type);

    MPI_Scatter(all_points, points_per_process, mpi_point_type,
                local_points, points_per_process, mpi_point_type,
                0, MPI_COMM_WORLD);

    printf("Process %d received %d points.\n", rank, points_per_process);

    // Write local points to a file
    char points_filename[50];
    sprintf(points_filename, "points_data_rank_%d.txt", rank);
    FILE* points_file = fopen(points_filename, "w");
    for (int i = 0; i < points_per_process; i++) {
        fprintf(points_file, "%f %f %f\n", local_points[i].x, local_points[i].y, local_points[i].z);
    }
    fclose(points_file);

    OctreeNode* local_tree = create_node((Point){0.5, 0.5, 0.5}, 1.0);
    for (int i = 0; i < points_per_process; i++) {
        insert_point(local_tree, local_points[i]);
    }

    printf("Process %d finished building its local Octree.\n", rank);

    // Write local Octree to a file
    char octree_filename[50];
    sprintf(octree_filename, "octree_data_rank_%d.txt", rank);
    FILE* octree_file = fopen(octree_filename, "w");
    write_octree_to_file(local_tree, octree_file);
    fclose(octree_file);

    merge_local_trees(local_tree, rank, size);

    if (rank == 0) {
        free(all_points);
    }
    free(local_points);
    free_octree(local_tree);
    MPI_Type_free(&mpi_point_type);

    MPI_Finalize();
    return 0;
}
