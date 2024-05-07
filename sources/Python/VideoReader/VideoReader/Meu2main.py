import cv2
import requests
import numpy as np
import time

# Replace the below URL with your ESP32-CAM video stream URL
stream_url = 'http://192.168.1.12:82/stream'

def calculate_angle(x1, y1, x2, y2):
    """
    Calculate the angle (in degrees) of a line segment with endpoints (x1, y1) and (x2, y2).
    """
    angle_radians = np.arctan2(y2 - y1, x2 - x1)
    angle_degrees = np.degrees(angle_radians)
    return angle_degrees

def color_by_group(group):
    if group == 1:
        return (0, 255, 0)
    return (0, 0, 255)


def detect_lines(frame):
    # Define the target color and the range around it
    target_color = np.array([100, 100, 105])
    color_range = 25  # Define the range around the target color

    # Convert the frame to the LAB color space
    lab = cv2.cvtColor(frame, cv2.COLOR_BGR2LAB)

    # Compute the lower and upper bounds for the color range
    lower_bound = np.array([max(0, target_color[0] - color_range), max(0, target_color[1] - color_range), max(0, target_color[2] - color_range)])
    upper_bound = np.array([min(255, target_color[0] + color_range), min(255, target_color[1] + color_range), min(255, target_color[2] + color_range)])

    # Create a mask to filter out only the colors within the specified range
    mask = cv2.inRange(lab, lower_bound, upper_bound)

    # Apply the mask to the grayscale image
    gray = cv2.bitwise_and(frame, frame, mask=mask)
    gray = cv2.cvtColor(gray, cv2.COLOR_BGR2GRAY)

    # Apply Gaussian blur
    blur = cv2.GaussianBlur(gray, (5, 5), 0)

    # Apply Canny edge detector
    edges = cv2.Canny(blur, 50, 150, apertureSize=3)

    # Use HoughLinesP to detect lines
    # These parameters can be adjusted to better detect lines in your specific setting
    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=25, minLineLength=50, maxLineGap=30)

    groups = []
    lines_aux = []
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            angle = calculate_angle(x1, y1, x2, y2)
            group = -1
            if (-45 >= angle >= -135) or (45 <= angle <= 135):
                group = 0
            else:
                group = 1
            lines_aux.append([x1, y1, x2, y2, group])

    if lines_aux is not None:
        for line in lines_aux:
            x1, y1, x2, y2, group = line
            cv2.line(frame, (x1, y1), (x2, y2), color_by_group(group), 2)

    # Display the grayscale image
    cv2.imshow('Grayscale Image', gray)

    return frame



process_interval = 0.2  # seconds, process frames every 0.2 seconds or 5 FPS
last_time = 0

cap = cv2.VideoCapture(stream_url)

while True:
    ret, frame = cap.read()

    # current_time = time.time()
    # if current_time - last_time < process_interval:
    #     continue  # Skip this iteration of the loop if interval not passed
    # last_time = current_time

    if ret:
        # Process each frame for line detection
        frame_with_lines = detect_lines(frame)
        cv2.imshow('ESP32-CAM Stream with Lines', frame_with_lines)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:  # 27 is the ASCII value for the escape key
            break
    else:
        print("Failed to grab frame")
        break

cap.release()
cv2.destroyAllWindows()