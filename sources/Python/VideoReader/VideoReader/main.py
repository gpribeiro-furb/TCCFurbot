import cv2
import requests
import numpy as np

# Replace the below URL with your ESP32-CAM video stream URL
stream_url = 'http://192.168.1.7:81/stream'


cap = cv2.VideoCapture(stream_url)

while True:
    ret, frame = cap.read()
    if ret:
        cv2.imshow('ESP32-CAM Stream', frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    else:
        print("ret", ret)
        break
cap.release()

# stream = requests.get(stream_url, stream=True)
#
# byte_stream = bytes()
# for chunk in stream.iter_content(chunk_size=1024):
#     byte_stream += chunk
#     a = byte_stream.find(b'\xff\xd8')
#     b = byte_stream.find(b'\xff\xd9')
#     if a != -1 and b != -1:
#         jpg = byte_stream[a:b+2]
#         byte_stream = byte_stream[b+2:]
#         frame = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)
#         if frame is not None:
#             cv2.imshow('ESP32-CAM Stream', frame)
#         if cv2.waitKey(1) == ord('q'):
#             break

