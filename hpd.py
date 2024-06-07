import tkinter as tk
from tkinter import messagebox
import cv2
import mediapipe as mp
import numpy as np

mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_pose = mp.solutions.pose



def GetPosePoints(file):
  temp_array = []
  with mp_pose.Pose(
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5) as pose:

    image = cv2.imread(file)

    image_height, image_width, _ = image.shape

    results = pose.process(cv2.cvtColor(image, cv2.COLOR_BGR2RGB))
    if not results.pose_landmarks:
      temp_array.append(-1.0)
    else:
      results.pose_landmarks.landmark
      for i in mp_pose.PoseLandmark:
        temp_array.append(results.pose_landmarks.landmark[mp_pose.PoseLandmark(i)].x * image_width)
        temp_array.append(results.pose_landmarks.landmark[mp_pose.PoseLandmark(i)].y * image_height)

  return temp_array