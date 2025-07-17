import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import glob

def plot_octree(ax, nodes, points_list, colors):
    for node in nodes:
        center_x, center_y, center_z, size, rank = node
        color = colors[int(rank)]
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

    for i, points in enumerate(points_list):
        ax.scatter(points[:, 0], points[:, 1], points[:, 2], c=[colors[i] for _ in range(len(points))], s=10)


fig = plt.figure(figsize=(12, 12))
ax = fig.add_subplot(111, projection='3d')

colors = ['r', 'g', 'b', 'c']

points_list = []
for i in range(4):
    points_files = glob.glob(f'points_data_rank_{i}.txt')
    if not points_files:
        print(f"Points data file for rank {i} not found.")
        continue
    points_list.append(np.loadtxt(points_files[0]))

octree_files = glob.glob('octree_data_global.txt')
if not octree_files:
    print("Global Octree data file not found.")
else:
    nodes = np.loadtxt(octree_files[0])
    plot_octree(ax, nodes, points_list, colors)

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_title('Parallel Octree Construction - Global Tree')
plt.savefig('octree_visualization_global.png')
plt.show()