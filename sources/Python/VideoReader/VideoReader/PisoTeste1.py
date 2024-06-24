import cv2
import requests
import numpy as np
import time
from enum import Enum
import threading

# Replace the below URL with your ESP32-CAM video stream URL
ip = '192.168.1.8'
stream_url = 'http://'+ ip +':82/stream'
post_url = 'http://'+ ip +':81/comando'

_grupos = []
_lastGrupos = []
_lastId = 1
_threshold = 50

up = False
down = False
left = False
right = False
passouPrimeiraLinha = False
primeiraPassada = True

class Comandos(Enum):
    UP = 'UP'
    DOWN = 'DOWN'
    LEFT = 'LEFT'
    RIGHT = 'RIGHT'
    DLEFT = 'DLEFT'
    DRIGHT = 'DRIGHT'
    STOP = 'STOP'
    CORRECT = 'CORRECT'

def calcular_media_dos_y_do_grupo(grupo):
    # Inicialize uma lista para armazenar as médias dos valores de y de cada linha
    medias_y = []

    # Itere sobre as linhas no grupo
    for linha in grupo[4]:  # grupo[4] contém a lista de linhas
        x1, y1, x2, y2, _ = linha  # Ignorando "group" porque não precisamos dela aqui
        # Calcule a média dos valores de y da linha atual e adicione à lista
        media_y_linha = (y1 + y2) / 2
        medias_y.append(media_y_linha)

    # Calcule a média das médias dos valores de y das linhas
    media_geral_y = sum(medias_y) / len(medias_y)

    grupo[2] = media_geral_y
    return media_geral_y

def calcular_media_dos_x_do_grupo(grupo):
    # Inicialize uma lista para armazenar as médias dos valores de x de cada linha
    medias_x = []

    # Itere sobre as linhas no grupo
    for linha in grupo[4]:  # grupo[4] contém a lista de linhas
        x1, y1, x2, y2, _ = linha  # Ignorando "group" porque não precisamos dela aqui
        # Calcule a média dos valores de x da linha atual e adicione à lista
        media_x_linha = (x1 + x2) / 2
        medias_x.append(media_x_linha)

    # Calcule a média das médias dos valores de x das linhas
    media_geral_x = sum(medias_x) / len(medias_x)

    grupo[2] = media_geral_x
    return media_geral_x

def calcular_media_dos_angulos_do_grupo(grupo, target_x=420, target_y=250):
    # Inicialize variáveis para armazenar a menor distância e a linha correspondente
    menor_distancia = float('inf')
    linha_mais_proxima = None

    # Itere sobre as linhas no grupo
    for linha in grupo[4]:  # grupo[4] contém a lista de linhas
        x1, y1, x2, y2, _ = linha  # Ignorando "group" porque não precisamos dela aqui
        # Calcule o ponto médio da linha
        ponto_medio_x = (x1 + x2) / 2
        ponto_medio_y = (y1 + y2) / 2
        # Calcule a distância euclidiana do ponto médio ao ponto alvo
        distancia = ((ponto_medio_x - target_x) ** 2 + (ponto_medio_y - target_y) ** 2) ** 0.5

        # Se a distância atual é menor que a menor distância registrada, atualize a menor distância e a linha correspondente
        if distancia < menor_distancia:
            menor_distancia = distancia
            linha_mais_proxima = linha

    x1, y1, x2, y2, _ = linha_mais_proxima
    angle = calculate_angle(x1, y1, x2, y2)
    grupo[3] = angle
    return angle

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
        angle = calculate_angle(x1, y1, x2, y2)
        if mediaLinha > 330:
            if get_group_by_id(-1) is None:
                _grupos.append([-1, group, mediaLinha, angle, [line]])
            else:
                grupoFora = get_group_by_id(-1)
                id, direcao, media, angle, linhas = grupoFora
                linhas.append(line)
                calcular_media_dos_y_do_grupo(grupoFora)
        else:
            for i in range(_lastGrupos.__len__()):
                idLastGrupo, direcaoLastGrupo, mediaLastGrupo, angleLastGrupo, linhasLastGrupo = _lastGrupos[i]
                if direcaoLastGrupo == "horizontal":
                    if (mediaLastGrupo - _threshold < mediaLinha < mediaLastGrupo + _threshold):
                        for i in range(_grupos.__len__()):
                            id, direcao, media, angle, linhas = _grupos[i]
                            if id == idLastGrupo:
                                linhas.append(line)
                                _grupos[i] = [id, direcao, media, angle, linhas]
                                calcular_media_dos_y_do_grupo(_grupos[i])
                                calcular_media_dos_angulos_do_grupo(_grupos[i])
                                return
                        _grupos.append([idLastGrupo, group, mediaLinha, angle, [line]])
                        return

            for i in range(_grupos.__len__()):
                id, direcao, media, angle, linhas = _grupos[i]
                if direcao == "horizontal":
                    if(media - _threshold < mediaLinha < media + _threshold):
                        linhas.append(line)
                        _grupos[i] = [id, direcao, media, angle, linhas]
                        calcular_media_dos_y_do_grupo(_grupos[i])
                        calcular_media_dos_angulos_do_grupo(_grupos[i])
                        return

            _grupos.append([_lastId, group, mediaLinha, angle, [line]])
            _lastId = _lastId + 1

    elif(group == "vertical"):
        mediaLinha = (x1 + x2) / 2
        angle = calculate_angle(x1, y1, x2, y2)
        for i in range(_lastGrupos.__len__()):
            idLastGrupo, direcaoLastGrupo, mediaLastGrupo, angleLastGrupo, linhasLastGrupo = _lastGrupos[i]
            if direcaoLastGrupo == "vertical":
                if (mediaLastGrupo - _threshold < mediaLinha < mediaLastGrupo + _threshold):
                    for i in range(_grupos.__len__()):
                        id, direcao, media, angle, linhas = _grupos[i]
                        if id == idLastGrupo:
                            linhas.append(line)
                            _grupos[i] = [id, direcao, media, angle, linhas]
                            calcular_media_dos_x_do_grupo(_grupos[i])
                            calcular_media_dos_angulos_do_grupo(_grupos[i])
                            return
                    _grupos.append([idLastGrupo, group, mediaLinha, angle, [line]])
                    return

        for i in range(_grupos.__len__()):
            id, direcao, media, angle, linhas = _grupos[i]
            if direcao == "vertical":
                if (media - _threshold < mediaLinha < media + _threshold):
                    linhas.append(line)
                    _grupos[i] = [id, direcao, media, angle, linhas]
                    calcular_media_dos_x_do_grupo(_grupos[i])
                    calcular_media_dos_angulos_do_grupo(_grupos[i])
                    return

        _grupos.append([_lastId, group, mediaLinha, angle, [line]])
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

def any_group_media_greater_than_300():
    for group in _grupos:
        if group[1] == "horizontal" and group[2] > 250 and group[0] > -1:
            return True
    return False

def corrigir_angulo(grupo):
    angle = grupo[3]
    if angle > -1 and angle < 1:
        return
    if angle < 0:
        send_request(Comandos.LEFT, angle)
    else:
        send_request(Comandos.RIGHT, angle)

def corrigir_meio():
    time.sleep(3)
    grupoEsquerda = []
    grupoDireita = []
    for grupo in _grupos:
        if grupo[0] > -1 and grupo[1] == "vertical":
            diferenca = grupo[2] - 398
            if diferenca < 0:
                if len(grupoEsquerda) == 0 or abs(diferenca) < abs((grupoEsquerda[2] - 398)):
                    grupoEsquerda = grupo
            else:
                if len(grupoDireita) == 0 or diferenca < abs((grupoDireita[2] - 398)):
                    grupoDireita = grupo

    if len(grupoEsquerda) > 0 and len(grupoDireita) > 0:
        mediaCorrecao = (grupoEsquerda[2] + grupoDireita[2])/2
        diferenca = mediaCorrecao - 398
        if abs(diferenca) > 5:
            if(diferenca > 0):
                send_request(Comandos.DRIGHT, diferenca)
            else:
                send_request(Comandos.DLEFT, diferenca)

def corrigir_geral(id, direcao, media, angle, linhas):
    time.sleep(8)
    grupo = [id, direcao, media, angle, linhas]
    corrigir_angulo(grupo)
    thread = threading.Thread(target=corrigir_meio)
    thread.start()


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
    # cv2.imshow('thresh', thresh)

    # Apply Canny edge detector
    edges = cv2.Canny(thresh, 50, 150)
    # cv2.imshow('edges', edges)

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

    # print()
    # print("========")
    # for grupo in _grupos:
    #     print("Id: ", grupo[0], " Direção: ", grupo[1], " Média: ", grupo[2], " Ângulo: ", grupo[3], " Qntd. linhas: ", len(grupo[4]))

    global up
    global left
    global right
    global passouPrimeiraLinha
    global primeiraPassada

    if up:
        if any_group_media_greater_than_300() and primeiraPassada:
            passouPrimeiraLinha = False
        if not any_group_media_greater_than_300():
            passouPrimeiraLinha = True

        for grupo in _grupos:
            if grupo[0] > -1 and grupo[1] == "horizontal":
                if len(grupo[4]) > 1:
                    if grupo[2] > 250:
                        if passouPrimeiraLinha:
                            up = False
                            send_request(Comandos.STOP)
                            corrigir_angulo(grupo)
                            thread = threading.Thread(target=corrigir_meio)
                            thread.start()
        primeiraPassada = False

    if left:
        for grupo in _grupos:
            if grupo[0] > -1 and grupo[1] == "horizontal":
                if len(grupo[4]) > 1:
                    if grupo[2] > 250:
                        thread = threading.Thread(target=corrigir_geral, args=grupo)
                        thread.start()
        left = False

    if right:
        for grupo in _grupos:
            if grupo[0] > -1 and grupo[1] == "horizontal":
                if len(grupo[4]) > 1:
                    if grupo[2] > 250:
                        thread = threading.Thread(target=corrigir_geral, args=grupo)
                        thread.start()
        right = False


    return frame




process_interval = 0.2  # seconds, process frames every 0.2 seconds or 5 FPS
last_time = 0

cap = cv2.VideoCapture(stream_url)
paused = False  # Variable to track pause state

def send_request(comando, valor=0):
    print(comando.value)
    # print(comando)
    response = "";
    if valor == 0:
        response = requests.post(post_url, data=comando.value)
    else:
        if comando == Comandos.LEFT or comando == Comandos.RIGHT:
            valor = abs(valor)
            formatted_float = f"{valor:.3f}"
            data = f"{comando.value}_{formatted_float}"
            response = requests.post(post_url, data=data)
        elif comando == Comandos.DLEFT or comando == Comandos.DRIGHT:
            valor = abs(valor)
            formatted_float = f"{valor:.3f}"
            data = f"{comando.value}_{formatted_float}"
            response = requests.post(post_url, data=data)
    # print(response)


def start_command(comando):
    global up
    global down
    global left
    global right
    global passouPrimeiraLinha
    global primeiraPassada
    if comando == Comandos.UP:
        up = True
        passouPrimeiraLinha = False
        primeiraPassada = True
    if comando == Comandos.DOWN:
        down = True
    if comando == Comandos.LEFT:
        left = True
    if comando == Comandos.RIGHT:
        right = True
    if comando == Comandos.CORRECT:
        for grupo in _grupos:
            if grupo[0] > -1:
                if len(grupo[4]) > 1:
                    if grupo[2] > 250:
                        corrigir_angulo(grupo)
                        thread = threading.Thread(target=corrigir_meio(), args=grupo)
                        thread.start()
                        return
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
        elif key == ord('x'):  # Corrigir
            start_command(Comandos.CORRECT)

        if not paused:  # Process each frame for line detection if not paused
            frame_with_lines = detect_lines(frame)
            cv2.imshow('ESP32-CAM Stream with Lines', frame_with_lines)
    else:
        print("Failed to grab frame")
        break

cap.release()
cv2.destroyAllWindows()