import cv2
import requests
import numpy as np
import time
import threading
import queue

URL_CONTROL = "http://192.168.0.148:82/control"
URL_STREAM = "http://192.168.0.148:81/stream"

frame_limiter = 20

def get_output_layers(net):
    
    layer_names = net.getLayerNames()
    try:
        output_layers = [layer_names[i - 1] for i in net.getUnconnectedOutLayers()]
    except:
        output_layers = [layer_names[i[0] - 1] for i in net.getUnconnectedOutLayers()]

    return output_layers


def draw_person(img, x, y, x_plus_w, y_plus_h):
    label = "person"

    color = (0,255,0)
    cv2.rectangle(img, (x,y), (x_plus_w,y_plus_h), color, 2)
    cv2.putText(img, label, (x-10,y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
    
frames = queue.Queue()

def read_frames():
    while(True):
        ret, thread_frame = cap.read()
        if not ret:
            print("Last frame")
            break
        else:
             if not frames.empty():
                try:
                  frames.get_nowait()
                except queue.Empty:
                  pass
        frames.put(thread_frame)

 
scale = 0.00392
frame_counter = 0

x = 0
y = 0
w = 0
h = 0

    
print("Loading NN")
net = cv2.dnn.readNet("yolov3.weights", "yolov3.cfg")
print("NN loaded")
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)
    
print("Starting capturing video stream")
cap = cv2.VideoCapture(URL_STREAM)
ret, temp_frame = cap.read()
print("Video stream is being captured")

t1 = threading.Thread(target=read_frames)
t1.start()

req_count=0;
while(True):
    frame = frames.get()
        
    start = time.time()
    print("Started NN analysis")
    blob = cv2.dnn.blobFromImage(frame, scale, (416,416), (0,0,0), True, crop=False)
    net.setInput(blob)
    outs = net.forward(get_output_layers(net))
    print("Ended NN analysis. Time taken: "+str(time.time()-start))


    Width = frame.shape[1]
    Height = frame.shape[0]
    conf_threshold = 0.5
    nms_threshold = 0.4
    confidences = []
    boxes = []

    for out in outs:
        for detected in out:
            scores = detected[5:]
            class_id = np.argmax(scores)
            if(class_id == 0):
                confidence = scores[class_id]
                if confidence > conf_threshold:
                    box_center_x = int(detected[0] * Width)
                    box_center_y = int(detected[1] * Height)
                    box_w = int(detected[2] * Width)
                    box_h = int(detected[3] * Height)
                    box_x = box_center_x - box_w / 2
                    box_y = box_center_y - box_h / 2
                    confidences.append(float(confidence))
                    boxes.append([box_x, box_y, box_w, box_h])

    indices = cv2.dnn.NMSBoxes(boxes, confidences, conf_threshold, nms_threshold)

    if len(indices) != 0:
        try:
            box = boxes[indices[0]]
        except:
            box = boxes[indices[0][0]]
        
        x = box[0]
        y = box[1]
        w = box[2]
        h = box[3]
        
        start = time.time()
        req_count=req_count+1
        print("Sending request with image data"+str(req_count))
        PARAMS = {'x':round(x+w/2),'h':round(h)}
        try:
            requests.post(url = URL_CONTROL, params = PARAMS,timeout=0.05)
        except: 
            pass
        print("Got response. Time taken: "+str(time.time()-start))
    
    draw_person(frame, round(x), round(y), round(x+w), round(y+h))
    cv2.imshow("Person detection", frame)
    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        break

cv2.destroyAllWindows()