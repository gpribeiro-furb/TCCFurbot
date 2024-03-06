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

    # Apply DBSCAN clustering
    # These parameters (eps and min_samples) might need adjustment based on your specific scenario
    dbscan = DBSCAN(eps=150, min_samples=1).fit(data)
    labels = dbscan.labels_

    # Number of clusters in labels, ignoring noise if present
    n_clusters_ = len(set(labels)) - (1 if -1 in labels else 0)

    return data, labels

def separate_clusters_by_angle(data, labels, angle_threshold=50):
    separated_labels = np.copy(labels)

    for label in np.unique(labels):
        if label == -1:  # Skip noise points
            continue

        cluster_points = data[labels == label]
        cluster_angles = cluster_points[:, 2]

        # Identify lines within the cluster that have more than angle_threshold degrees of difference
        angle_diff_matrix = np.abs(np.subtract.outer(cluster_angles, cluster_angles))
        separated_indices = np.any(angle_diff_matrix > angle_threshold, axis=0)

        # Create a new label for the separated lines
        new_label = max(separated_labels) + 1
        separated_labels[labels == label] = np.where(separated_indices, new_label, label)

    return separated_labels

def detect_lines_and_cluster(frame):
    # Convert to grayscale, apply Gaussian blur, and then Canny edge detector
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blur, 50, 150, apertureSize=3)

    # Use HoughLinesP to detect lines
    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=25, minLineLength=120, maxLineGap=30)

    # Cluster the lines and color each cluster differently
    data, labels = cluster_lines(lines)
    separated_labels = separate_clusters_by_angle(data, labels, angle_threshold=50)

    unique_labels = set(separated_labels)
    colors = [tuple(np.random.randint(0, 255, 3).tolist()) for _ in unique_labels]

    if lines is not None:
        for line, label in zip(lines, separated_labels):
            if label != -1:  # Ignore noise
                x1, y1, x2, y2 = line[0]
                color = colors[label]
                cv2.line(frame, (x1, y1), (x2, y2), color, 2)

    return frame

# Rest of the code remains the same

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
        # Process each frame for line detection and clustering
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
