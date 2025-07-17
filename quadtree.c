#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_POINTS_PER_NODE 1

// Represents a 2D point
typedef struct {
    double x, y;
} Point;

// Represents a node in the Quadtree
typedef struct QuadtreeNode {
    Point center;
    double size;
    Point* points;
    int num_points;
    struct QuadtreeNode* children[4];
    int rank; // For visualization
} QuadtreeNode;

// Represents a serialized node for communication
typedef struct {
    Point center;
    double size;
    int rank;
    int child_indices[4];
} SerializedNode;

// Forward declarations
void insert_point(QuadtreeNode* node, Point point, int rank);
void subdivide_node(QuadtreeNode* node, int rank);
void serialize_tree(QuadtreeNode* node, SerializedNode** serialized_nodes, int* count, int* capacity);
void reconstruct_global_tree(SerializedNode* all_nodes, int total_nodes, QuadtreeNode** global_tree);
void write_global_tree_to_file(QuadtreeNode* node, FILE* file);

// Function to create a new Quadtree node
QuadtreeNode* create_node(Point center, double size, int rank) {
    QuadtreeNode* node = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
    node->center = center;
    node->size = size;
    node->points = (Point*)malloc(MAX_POINTS_PER_NODE * sizeof(Point));
    node->num_points = 0;
    for (int i = 0; i < 4; i++) {
        node->children[i] = NULL;
    }
    node->rank = rank;
    return node;
}

// Function to determine which child a point belongs to
int get_quadrant(Point center, Point point) {
    int quadrant = 0;
    if (point.x >= center.x) quadrant |= 1;
    if (point.y >= center.y) quadrant |= 2;
    return quadrant;
}

// Function to insert a point into the Quadtree
void insert_point(QuadtreeNode* node, Point point, int rank) {
    if (node->children[0] != NULL) {
        int quadrant = get_quadrant(node->center, point);
        insert_point(node->children[quadrant], point, rank);
        return;
    }

    if (node->num_points < MAX_POINTS_PER_NODE) {
        node->points[node->num_points++] = point;
        return;
    }

    subdivide_node(node, rank);
    for (int i = 0; i < node->num_points; i++) {
        int quadrant = get_quadrant(node->center, node->points[i]);
        insert_point(node->children[quadrant], node->points[i], rank);
    }
    int quadrant = get_quadrant(node->center, point);
    insert_point(node->children[quadrant], point, rank);

    free(node->points);
    node->points = NULL;
    node->num_points = 0;
}

// Function to subdivide a node into 4 children
void subdivide_node(QuadtreeNode* node, int rank) {
    double child_size = node->size / 2.0;
    for (int i = 0; i < 4; i++) {
        Point child_center = node->center;
        child_center.x += (i & 1) ? child_size / 2.0 : -child_size / 2.0;
        child_center.y += (i & 2) ? child_size / 2.0 : -child_size / 2.0;
        node->children[i] = create_node(child_center, child_size, rank);
    }
}

// Function to serialize the tree into a flat array
void serialize_tree(QuadtreeNode* node, SerializedNode** serialized_nodes, int* count, int* capacity) {
    if (node == NULL) {
        return;
    }

    if (*count >= *capacity) {
        *capacity *= 2;
        *serialized_nodes = (SerializedNode*)realloc(*serialized_nodes, *capacity * sizeof(SerializedNode));
    }

    int current_index = (*count)++;
    (*serialized_nodes)[current_index].center = node->center;
    (*serialized_nodes)[current_index].size = node->size;
    (*serialized_nodes)[current_index].rank = node->rank;

    for (int i = 0; i < 4; i++) {
        if (node->children[i] != NULL) {
            (*serialized_nodes)[current_index].child_indices[i] = *count;
            serialize_tree(node->children[i], serialized_nodes, count, capacity);
        } else {
            (*serialized_nodes)[current_index].child_indices[i] = -1;
        }
    }
}

// Function to reconstruct the global tree from the serialized data
void reconstruct_global_tree(SerializedNode* all_nodes, int total_nodes, QuadtreeNode** global_tree) {
    if (total_nodes == 0) {
        *global_tree = NULL;
        return;
    }

    QuadtreeNode** nodes = (QuadtreeNode**)malloc(total_nodes * sizeof(QuadtreeNode*));
    for (int i = 0; i < total_nodes; i++) {
        nodes[i] = create_node(all_nodes[i].center, all_nodes[i].size, all_nodes[i].rank);
    }

    for (int i = 0; i < total_nodes; i++) {
        for (int j = 0; j < 4; j++) {
            int child_index = all_nodes[i].child_indices[j];
            if (child_index != -1) {
                nodes[i]->children[j] = nodes[child_index];
            }
        }
    }

    *global_tree = nodes[0];
    free(nodes);
}

// Function to write the global Quadtree data to a file
void write_global_tree_to_file(QuadtreeNode* node, FILE* file) {
    if (node == NULL) {
        return;
    }
    fprintf(file, "%f %f %f %d\n", node->center.x, node->center.y, node->size, node->rank);
    for (int i = 0; i < 4; i++) {
        write_global_tree_to_file(node->children[i], file);
    }
}

// Function to free the memory of the Quadtree
void free_quadtree(QuadtreeNode* node) {
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < 4; i++) {
        free_quadtree(node->children[i]);
    }
    if (node->points) {
        free(node->points);
    }
    free(node);
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
        for (int i = 0; i < total_points; i++) {
            all_points[i] = (Point){drand48(), drand48()};
        }
    }

    int points_per_process = total_points / size;
    Point* local_points = (Point*)malloc(points_per_process * sizeof(Point));

    MPI_Datatype mpi_point_type;
    MPI_Type_contiguous(2, MPI_DOUBLE, &mpi_point_type);
    MPI_Type_commit(&mpi_point_type);

    MPI_Scatter(all_points, points_per_process, mpi_point_type,
                local_points, points_per_process, mpi_point_type,
                0, MPI_COMM_WORLD);

    char points_filename[50];
    sprintf(points_filename, "points_data_rank_%d.txt", rank);
    FILE* points_file = fopen(points_filename, "w");
    for (int i = 0; i < points_per_process; i++) {
        fprintf(points_file, "%f %f\n", local_points[i].x, local_points[i].y);
    }
    fclose(points_file);

    QuadtreeNode* local_tree = create_node((Point){0.5, 0.5}, 1.0, rank);
    for (int i = 0; i < points_per_process; i++) {
        insert_point(local_tree, local_points[i], rank);
    }

    int capacity = 100;
    int count = 0;
    SerializedNode* serialized_nodes = (SerializedNode*)malloc(capacity * sizeof(SerializedNode));
    serialize_tree(local_tree, &serialized_nodes, &count, &capacity);

    int* recvcounts = NULL;
    int* displs = NULL;
    SerializedNode* all_serialized_nodes = NULL;
    int total_serialized_nodes = 0;

    if (rank == 0) {
        recvcounts = (int*)malloc(size * sizeof(int));
    }

    MPI_Gather(&count, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        displs = (int*)malloc(size * sizeof(int));
        displs[0] = 0;
        total_serialized_nodes = recvcounts[0];
        for (int i = 1; i < size; i++) {
            total_serialized_nodes += recvcounts[i];
            displs[i] = displs[i - 1] + recvcounts[i - 1];
        }
        all_serialized_nodes = (SerializedNode*)malloc(total_serialized_nodes * sizeof(SerializedNode));
    }

    MPI_Datatype mpi_serialized_node_type;
    MPI_Type_contiguous(sizeof(SerializedNode), MPI_BYTE, &mpi_serialized_node_type);
    MPI_Type_commit(&mpi_serialized_node_type);

    MPI_Gatherv(serialized_nodes, count, mpi_serialized_node_type,
                all_serialized_nodes, recvcounts, displs, mpi_serialized_node_type,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        QuadtreeNode* global_tree = NULL;
        reconstruct_global_tree(all_serialized_nodes, total_serialized_nodes, &global_tree);

        char octree_filename[50];
        sprintf(octree_filename, "quadtree_data_global.txt");
        FILE* octree_file = fopen(octree_filename, "w");
        write_global_tree_to_file(global_tree, octree_file);
        fclose(octree_file);

        free_quadtree(global_tree);
        free(recvcounts);
        free(displs);
        free(all_serialized_nodes);
    }

    free(serialized_nodes);
    if (rank == 0) {
        free(all_points);
    }
    free(local_points);
    free_quadtree(local_tree);
    MPI_Type_free(&mpi_point_type);
    MPI_Type_free(&mpi_serialized_node_type);

    MPI_Finalize();
    return 0;
}
