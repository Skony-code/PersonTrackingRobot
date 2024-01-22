import requests 
import time
 
req_count=0
URL_CONTROL = "http://192.168.0.148:82/control"

while(True):
    start = time.time()
    time.sleep(0.1)
    req_count=req_count+1
    print("Sending request with image data"+str(req_count))
    PARAMS = {'x':round(2/2),'y':round(2/2)}
    try:
        requests.post(url = URL_CONTROL, params = PARAMS,timeout=0.05)
    except: 
        pass
    print("Got response. Time taken: "+str(time.time()-start))