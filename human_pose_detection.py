import cv2
import mediapipe as mp
import numpy as np

import win32pipe
import win32file

mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_pose = mp.solutions.pose

# Am besten Bildschirm mit OBS aufnehmen und als Standard-Kamera verwenden, um Bilder/Videos zu übertragen
cap = cv2.VideoCapture(0)
count = 0
with mp_pose.Pose(
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5) as pose:

  if cap != None:
      cap_width = int(cap.get(3))
      cap_height = int(cap.get(4))
      print(
      f'Width: {cap_width}\n'
      f'Height: {cap_height}'
      )

  windowWidth = 1920
  windowHeight = 1080

  cv2.namedWindow("MediaPipe Pose", cv2.WINDOW_NORMAL)

  cv2.resizeWindow("MediaPipe Pose", windowWidth, windowHeight)

  while cap.isOpened():
    success, image = cap.read()

    if not success:
      print("Ignoring empty camera frame.")
      continue


    image.flags.writeable = False
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    results = pose.process(image)

    if not results.pose_landmarks:
      continue

    print(
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_SHOULDER].x * cap_width}\n'
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_SHOULDER].y * cap_height}\n'
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_ELBOW].x * cap_width}\n'
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_ELBOW].y * cap_height}\n'
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_HIP].x * cap_width}\n'
      f'{results.pose_landmarks.landmark[mp_pose.PoseLandmark.LEFT_HIP].y * cap_height}\n'
    )

    image.flags.writeable = True
    image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    mp_drawing.draw_landmarks(
        image,
        results.pose_landmarks,
        mp_pose.POSE_CONNECTIONS,
        landmark_drawing_spec=mp_drawing_styles.get_default_pose_landmarks_style())
    cv2.imshow('MediaPipe Pose', image)
    count += 1
    if cv2.waitKey(5) & 0xFF == 27:
      break
cap.release()