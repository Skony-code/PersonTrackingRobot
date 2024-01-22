# PersonTrackingRobot
## Odpalenie robota
### Arduino
Należy edytować te dwie linie na odpowiadające własnej sieci WiFi
```
//Replace with your network credentials
const char* ssid = "robot_test_2400";
const char* password = "12345";
```
Po podłączeniu Serial Monitora do ESP32 CAM poinformuje on o udanym połączeniu z sięcią WiFi i poda swój ades ip w tej sieci.
### Python
#### Wymagane paczki
```
import cv2
import requests
import numpy
import time
import threading
import queue
```

Należy zmienić adresy ip w tych dwóch liniach na odpowiadające adresowi ESP32 CAM po podłączeniu do sieci WiFi. Porty pozostawić takie same.
```
URL_CONTROL = "http://192.168.0.148:82/control"
URL_STREAM = "http://192.168.0.148:81/stream"
```

### OpenCV CUDA(opcjonalne)
Aby przyśpieszyć analizę obrazu przez sieć neuronową najlepiej jest skorzystać z CUDA. Niestety aby to zrobić, trzeba samodzielnie zbudować OpenCV dla pythona. Tutaj tutorial:
https://www.youtube.com/watch?v=d8Jx6zO1yw0
Konieczne jest też posiadanie karty graficznej Nvidia

### Procedura odpalenia
Po wykonaniu powyższych instrukcji należy upewnić się że zarówno robot jak i urządzenie na którym działa sieć nuronowa znajdują się w tej samej sieci lokalnej. Dalsze kroki jakie należy podjąć:
1. Włączyć robota
2. Poczekać na podwójne mignięcie białą diodą led
3. Na urządzeniu zewnętrznym odpalić kod pythonowy poniższą komendą
```
python opencvread.py
```
