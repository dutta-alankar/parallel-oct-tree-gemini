import matplotlib.pyplot as plt
import numpy as np
import glob

def plot_quadtree(ax, nodes, points_list, colors):
    for node in nodes:
        center_x, center_y, size, rank = node
        color = colors[int(rank)]
        half_size = size / 2.0
        x = center_x - half_size
        y = center_y - half_size

        # Draw the square
        ax.add_patch(plt.Rectangle((x, y), size, size, fill=False, color=color, linestyle=':'))

    for i, points in enumerate(points_list):
        ax.scatter(points[:, 0], points[:, 1], c=[colors[i] for _ in range(len(points))], s=10)


fig, ax = plt.subplots(figsize=(10, 10))

colors = ['r', 'g', 'b', 'c']

points_list = []
for i in range(4):
    points_files = glob.glob(f'points_data_rank_{i}.txt')
    if not points_files:
        print(f"Points data file for rank {i} not found.")
        continue
    points_list.append(np.loadtxt(points_files[0]))

quadtree_files = glob.glob('quadtree_data_global.txt')
if not quadtree_files:
    print("Global Quadtree data file not found.")
else:
    nodes = np.loadtxt(quadtree_files[0])
    plot_quadtree(ax, nodes, points_list, colors)

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_title('Parallel Quadtree Construction - Global Tree')
ax.set_aspect('equal', adjustable='box')
plt.savefig('quadtree_visualization_global.png')
plt.show()
