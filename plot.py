
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import glob

def plot_octree(ax, nodes, points, color):
    for node in nodes:
        center_x, center_y, center_z, size = node
        half_size = size / 2.0
        x = center_x - half_size
        y = center_y - half_size
        z = center_z - half_size

        # Draw the cube
        xx, yy = np.meshgrid([x, x + size], [y, y + size])
        ax.plot_wireframe(xx, yy, np.full_like(xx, z), color=color, linestyle=':')
        ax.plot_wireframe(xx, yy, np.full_like(xx, z + size), color=color, linestyle=':')
        ax.plot_wireframe(np.full_like(xx, x), xx, yy, color=color, linestyle=':')
        ax.plot_wireframe(np.full_like(xx, x + size), xx, yy, color=color, linestyle=':')

    ax.scatter(points[:, 0], points[:, 1], points[:, 2], c=[color for _ in range(len(points))], s=10)


fig = plt.figure(figsize=(12, 12))
ax = fig.add_subplot(111, projection='3d')

colors = ['r', 'g', 'b', 'c']

for i in range(4):
    points_files = glob.glob(f'points_data_rank_{i}.txt')
    octree_files = glob.glob(f'octree_data_rank_{i}.txt')

    if not points_files or not octree_files:
        print(f"Data files for rank {i} not found.")
        continue

    points = np.loadtxt(points_files[0])
    nodes = np.loadtxt(octree_files[0])

    plot_octree(ax, nodes, points, colors[i])

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_title('Parallel Octree Construction')
plt.savefig('octree_visualization.png')
plt.show()
