import cv2
import numpy as np
import time
from sklearn.cluster import DBSCAN


def angle_difference(angle1, angle2):
    # Calculate the absolute difference in angles, normalized between 0 and 180 degrees
    diff = np.abs(angle1 - angle2) % 180
    if diff > 90:
        diff = 180 - diff
    return diff


def custom_distance(point1, point2):
    # Example distance calculation that heavily penalizes perpendicularity
    # Assume point1 and point2 include coordinates and angle: (x, y, angle)
    spatial_distance = np.sqrt((point1[0] - point2[0]) ** 2 + (point1[1] - point2[1]) ** 2)
    angle_diff = angle_difference(point1[2], point2[2])

    # Penalize perpendicular lines heavily
    if angle_diff > 70 and angle_diff < 110:  # Adjust thresholds as needed
        return spatial_distance * 10  # Example penalty factor
    return spatial_distance + angle_diff  # Combine spatial and angular differences


def cluster_lines(lines):
    if lines is None:
        return [], []

    # Prepare the data for clustering: Use midpoints and angles of lines
    data = []
    for line in lines:
        x1, y1, x2, y2 = line[0]
        midpoint = [(x1 + x2) / 2, (y1 + y2) / 2]
        angle = np.arctan2(y2 - y1, x2 - x1)
        data.append(midpoint + [angle])

    data = np.array(data)

    # Apply DBSCAN clustering for spatial proximity
    spatial_dbscan = DBSCAN(eps=20, min_samples=1).fit(data[:, :2])
    spatial_labels = spatial_dbscan.labels_

    # Refine clusters based on orientation similarity using custom_distance
    refined_labels = []
    for label in np.unique(spatial_labels):
        cluster_points = data[spatial_labels == label]
        if len(cluster_points) > 1:
            orientation_dbscan = DBSCAN(eps=150, min_samples=1, metric=custom_distance).fit(cluster_points)
            refined_labels.extend(orientation_dbscan.labels_ + max(refined_labels, default=-1) + 1)
        else:
            refined_labels.extend([label])

    return data, refined_labels


def detect_lines_and_cluster(frame):
    # Convert to grayscale, apply Gaussian blur, and then Canny edge detector
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blur, 50, 150, apertureSize=3)

    # Use HoughLinesP to detect lines
    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=25, minLineLength=120, maxLineGap=30)

    # Cluster the lines and color each cluster differently
    data, labels = cluster_lines(lines)
    unique_labels = set(labels)
    colors = [tuple(np.random.randint(0, 255, 3).tolist()) for _ in unique_labels]

    if lines is not None:
        for line, label in zip(lines, labels):
            if label != -1:  # Ignore noise
                x1, y1, x2, y2 = line[0]
                color = colors[label]
                cv2.line(frame, (x1, y1), (x2, y2), color, 2)

    return frame

process_interval = 0.2  # seconds, process frames every 0.2 seconds or 5 FPS
last_time = 0

# Replace the below URL with your ESP32-CAM video stream URL
stream_url = 'http://192.168.1.7:81/stream'
cap = cv2.VideoCapture(stream_url)

while True:
    ret, frame = cap.read()

    current_time = time.time()
    if current_time - last_time < process_interval:
        continue  # Skip this iteration of the loop if interval not passed
    last_time = current_time

    if ret:
        # Process each frame for line detection
        frame_with_lines = detect_lines_and_cluster(frame)
        cv2.imshow('ESP32-CAM Stream with Lines', frame_with_lines)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:  # 27 is the ASCII value for the escape key
            break
    else:
        print("Failed to grab frame")
        break

cap.release()
cv2.destroyAllWindows()