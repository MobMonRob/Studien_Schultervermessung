import open3d as o3d
import mediapipe as mp
import numpy as np

__name = "__main"

filename = "pc_ascii5.ply"
file = (r'C:\Dateipfad_zur_.Ply-Datei' + filename)

def read_ply(filename):
    pcd = o3d.io.read_point_cloud(filename)
    return pcd

def visualize_point_cloud(pcd):
    o3d.visualization.draw_geometries([pcd])

if __name == "__main":
    point_cloud = read_ply(file)
    visualize_point_cloud(point_cloud)
