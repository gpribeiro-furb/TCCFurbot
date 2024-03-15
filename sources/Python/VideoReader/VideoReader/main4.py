import cv2
import numpy as np
import time
from sklearn.cluster import DBSCAN


def calculate_angle(x1, y1, x2, y2):
    return np.arctan2(y2 - y1, x2 - x1) * 180 / np.pi


def cluster_lines(lines):
    if lines is None:
        return [], []

    data = []
    for line in lines:
        x1, y1, x2, y2 = line[0]
        midpoint = [(x1 + x2) / 2, (y1 + y2) / 2]
        angle = calculate_angle(x1, y1, x2, y2)
        data.append(midpoint + [angle])

    data = np.array(data)

    # Adjust the eps value if necessary to fine-tune the clustering sensitivity
    dbscan = DBSCAN(eps=50, min_samples=1, metric='euclidean').fit(
        data[:, :2])  # Cluster based on midpoint only initially
    labels = dbscan.labels_

    # Attempt to separate lines further based on angle difference
    unique_labels = set(labels)
    final_labels = labels.copy()
    label_correction = max(labels) + 1
    for label in unique_labels:
        if label == -1: continue  # Skip noise
        indices = np.where(labels == label)[0]
        if len(indices) <= 1: continue  # No need to process single-line clusters

        angles = data[indices, 2]
        mean_angle = np.mean(angles)

        # Separate lines based on angle deviation from the cluster mean
        for i, angle in enumerate(angles):
            if abs(angle - mean_angle) > 45:  # Allow some tolerance
                final_labels[indices[i]] = label_correction
                label_correction += 1

    return data, final_labels


def detect_lines_and_cluster(frame):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blur, 50, 150, apertureSize=3)
    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=50, minLineLength=30, maxLineGap=10)

    data, labels = cluster_lines(lines)

    unique_labels = set(labels)
    colors = [tuple(np.random.randint(0, 255, 3).tolist()) for _ in unique_labels]

    if lines is not None:
        for line, label in zip(lines, labels):
            if label >= 1:
                x1, y1, x2, y2 = line[0]
                cv2.line(frame, (x1, y1), (x2, y2), colors[label-1], 2)

    return frame


process_interval = 0.2
last_time = 0

stream_url = 'http://192.168.1.7:81/stream'
cap = cv2.VideoCapture(stream_url)

while True:
    ret, frame = cap.read()
    current_time = time.time()
    if current_time - last_time < process_interval:
        continue
    last_time = current_time

    if ret:
        frame_with_lines = detect_lines_and_cluster(frame)
        cv2.imshow('ESP32-CAM Stream with Lines', frame_with_lines)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
    else:
        print("Failed to grab frame")
        break

cap.release()
cv2.destroyAllWindows()
