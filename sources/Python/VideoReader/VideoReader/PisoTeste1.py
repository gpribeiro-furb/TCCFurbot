import cv2
import requests
import numpy as np
import time
from enum import Enum

# Replace the below URL with your ESP32-CAM video stream URL
ip = '192.168.1.14'
stream_url = 'http://'+ ip +':82/stream'
post_url = 'http://'+ ip +':81/test'

_grupos = []
_lastGrupos = []
_lastId = 1
_threshold = 50

up = False
down = False
left = False
right = False

class Comandos(Enum):
    UP = 'UP'
    DOWN = 'DOWN'
    LEFT = 'LEFT'
    RIGHT = 'RIGHT'
    STOP = 'STOP'

def calcular_media_dos_y_do_grupo(grupo):
    # Inicialize uma lista para armazenar as médias dos valores de y de cada linha
    medias_y = []

    # Itere sobre as linhas no grupo
    for linha in grupo[3]:  # grupo[3] contém a lista de linhas
        x1, y1, x2, y2, _ = linha  # Ignorando "group" porque não precisamos dela aqui
        # Calcule a média dos valores de y da linha atual e adicione à lista
        media_y_linha = (y1 + y2) / 2
        medias_y.append(media_y_linha)

    # Calcule a média das médias dos valores de y das linhas
    media_geral_y = sum(medias_y) / len(medias_y)

    grupo[2] = media_geral_y
    return media_geral_y

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

def get_group_by_id(group_id):
    for group in _grupos:
        if group[0] == group_id:
            return group
    return None

def remove_from_last_group_by_id(group_id):
    global _lastGrupos  # Declare _grupos as global so that it can be modified within the function
    _lastGrupos = [group for group in _lastGrupos if group[0] != group_id]

def agrupaLinha(line):
    global _lastId
    x1, y1, x2, y2, group = line
    if(group == "horizontal"):
        mediaLinha = (y1+y2)/2
        if mediaLinha > 330:
            if get_group_by_id(-1) is None:
                _grupos.append([-1, group, mediaLinha, [line]])
            else:
                grupoFora = get_group_by_id(-1)
                id, direcao, media, linhas = grupoFora
                linhas.append(line)
                calcular_media_dos_y_do_grupo(grupoFora)
        else:
            for i in range(_lastGrupos.__len__()):
                idLastGrupo, direcaoLastGrupo, mediaLastGrupo, linhasLastGrupo = _lastGrupos[i]
                if (mediaLastGrupo - _threshold < mediaLinha < mediaLastGrupo + _threshold):
                    for i in range(_grupos.__len__()):
                        id, direcao, media, linhas = _grupos[i]
                        if id == idLastGrupo:
                            linhas.append(line)
                            _grupos[i] = [id, direcao, media, linhas]
                            calcular_media_dos_y_do_grupo(_grupos[i])
                            return
                    _grupos.append([idLastGrupo, group, mediaLinha, [line]])
                    return

            for i in range(_grupos.__len__()):
                id, direcao, media, linhas = _grupos[i]
                if(media - _threshold < mediaLinha < media + _threshold):
                    linhas.append(line)
                    _grupos[i] = [id, direcao, media, linhas]
                    calcular_media_dos_y_do_grupo(_grupos[i])
                    return

            _grupos.append([_lastId, group, mediaLinha, [line]])
            _lastId = _lastId + 1



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
#           2.1 - Usa os grupos antigos para ter as médias, caso não tenha um grupo antigo perto da linha atual, cria um grupo
#       3-  Adicionar linha no grupo, recalcular média do grupo
#


def detect_lines(frame):
    global _grupos
    _lastGrupos.clear()
    if len(_grupos) > 0:
        _lastGrupos.extend(_grupos)
    _grupos = []
    remove_from_last_group_by_id(-1)

    # # Convert to grayscale
    # gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    # # Apply Gaussian blur
    # blur = cv2.GaussianBlur(gray, (5, 5), 0)
    # # Apply Canny edge detector
    # edges = cv2.Canny(blur, 70, 150, apertureSize=3)
    # cv2.imshow('Ee', edges)
    #
    # # Use HoughLinesP to detect lines
    # # These parameters can be adjusted to better detect lines in your specific setting
    # lines = cv2.HoughLinesP(edges, 1, np.pi / 180, threshold=100, minLineLength=50, maxLineGap=50)

    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Preprocess the image to remove noise
    gray = cv2.GaussianBlur(gray, (5, 5), 0)

    # Apply adaptive thresholding
    thresh = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 15, 8)
    cv2.imshow('thresh', thresh)

    # Apply Canny edge detector
    edges = cv2.Canny(thresh, 50, 150)
    cv2.imshow('edges', edges)

    # Use HoughLinesP to detect lines
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

    print()
    print("========")
    for grupo in _grupos:
        print("Id: ", grupo[0], " Média: ", grupo[2], " Qntd. linhas: ", len(grupo[3]))

    global up

    if up:
        for grupo in _grupos:
            if grupo[0] > -1:
                if grupo[2] > 300 and len(grupo[3]) > 1:
                    up = False
                    send_request(Comandos.STOP)

    return frame


process_interval = 0.2  # seconds, process frames every 0.2 seconds or 5 FPS
last_time = 0

cap = cv2.VideoCapture(stream_url)
paused = False  # Variable to track pause state

def send_request(comando):
    print(comando.value)
    print(comando)
    # response = requests.post(post_url, data=comando.value)
    # print(response)


def start_command(comando):
    global up
    global down
    global left
    global right
    if comando == Comandos.UP:
        up = True
    if comando == Comandos.DOWN:
        down = True
    if comando == Comandos.LEFT:
        left = True
    if comando == Comandos.RIGHT:
        right = True
    send_request(comando)


while True:
    ret, frame = cap.read()

    current_time = time.time()
    if current_time - last_time < process_interval:
        continue  # Skip this iteration of the loop if interval not passed
    last_time = current_time

    if ret:
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:  # 27 is the ASCII value for the escape key
            break
        elif key == ord(' '):  # Toggle pause if spacebar is pressed
            paused = not paused
            send_request(Comandos.STOP)
        elif key == ord('w'):  # UP - Frente
            start_command(Comandos.UP)
        elif key == ord('s'):  # DOWN - Trás
            start_command(Comandos.DOWN)
        elif key == ord('a'):  # LEFT - Gira esquerda
            start_command(Comandos.LEFT)
        elif key == ord('d'):  # RIGHT - Gira direita
            start_command(Comandos.RIGHT)

        if not paused:  # Process each frame for line detection if not paused
            frame_with_lines = detect_lines(frame)
            cv2.imshow('ESP32-CAM Stream with Lines', frame_with_lines)
    else:
        print("Failed to grab frame")
        break

cap.release()
cv2.destroyAllWindows()