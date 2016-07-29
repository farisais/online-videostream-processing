import cv2
import numpy as np
import json

frame = 0
cascPath = 'processing/haarcascade_frontalface_alt2.xml'
faceCascade = cv2.CascadeClassifier(cascPath)

def process(data):
	nparr = np.frombuffer(data, np.uint8)
	img_np = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
	edges = cv2.Canny(img_np, 100,200)

	param = [int(cv2.IMWRITE_PXM_BINARY),1]
	ret, buf = cv2.imencode(".ppm", edges, param)
	resimg = buf.tobytes()
	return bytearray(resimg)