import cv2
import requests
import numpy as np
import time

# Replace the below URL with your ESP32-CAM video stream URL
stream_url = 'http://192.168.1.12:82/stream'

_grupos = []
_lastGrupos = []
_lastId = 1
_threshold = 50

def calculate_angle(x1, y1, x2, y2):
    """
    Calculate the angle (in degrees) of a line segment with endpoints (x1, y1) and (x2, y2).
    """
    angle_radians = np.arctan2(y2 - y1, x2 - x1)
    angle_degrees = np.degrees(angle_radians)
    return angle_degrees

def color_by_group(group):
    if group == "horizontal":
        return (0, 255, 0)
    return (0, 0, 255)


def agrupaLinha(line):
    global _lastId
    x1, y1, x2, y2, group = line
    if(group == "horizontal"):
        mediaLinha = (y1+y2)/2
        if(_grupos.__len__() == 0):
            _grupos.append([_lastId, group, mediaLinha, [line]])
            _lastId = _lastId + 1
        else:
            for i in range(_grupos.__len__()):
                id, direcao, media, linhas = _grupos[i]
                if(media - _threshold < mediaLinha < media + _threshold):
                    linhas.append(line)
                    _grupos[i] = [id, direcao, media, linhas]


#   Grupo terá: {
#   id: 1
#   direcao: horizontal|vertical
#   media: 50 (média do valor X caso vertical, média do Y caso horizontal)
#   linhas: lista das linhas
#   }
#
#   1. Passar _grupos para _lastGrupos
#   2. Adicionar linhas atuais no _grupos
#       1-  Passar pelos grupos da direção correta
#       2-  Verificar se tem alguma média que está perto (threshold) do valor médio do Y da linha atual
#       3-  Adicionar linha no grupo, recaclcular média do grupo


def detect_lines(frame):

    _lastGrupos.clear()
    _lastGrupos.append(_grupos)
    _grupos.clear()

    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    # Apply Gaussian blur
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    # Apply Canny edge detector
    edges = cv2.Canny(blur, 70, 150, apertureSize=3)
    cv2.imshow('Ee', edges)

    # Use HoughLinesP to detect lines
    # These parameters can be adjusted to better detect lines in your specific setting
    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=100, minLineLength=50, maxLineGap=50)

    lines_aux = []
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            angle = calculate_angle(x1, y1, x2, y2)
            if (-45 >= angle >= -135) or (45 <= angle <= 135):
                group = "vertical"
            else:
                group = "horizontal"
            lines_aux.append([x1, y1, x2, y2, group])

    if lines_aux is not None:
        for line in lines_aux:
            x1, y1, x2, y2, group = line
            cv2.line(frame, (x1, y1), (x2, y2), color_by_group(group), 2)
            agrupaLinha(line)

    print(_grupos)

    return frame


process_interval = 0.2  # seconds, process frames every 0.2 seconds or 5 FPS
last_time = 0

cap = cv2.VideoCapture(stream_url)

while True:
    ret, frame = cap.read()

    current_time = time.time()
    if current_time - last_time < process_interval:
        continue  # Skip this iteration of the loop if interval not passed
    last_time = current_time

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
