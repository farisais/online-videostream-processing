import json
import cv2
import numpy as np

frame = 0
cascPath = 'processing/haarcascade_frontalface_alt2.xml'
faceCascade = cv2.CascadeClassifier(cascPath)

def process(data):
	nparr = np.frombuffer(data, np.uint8)
	img_np = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
	gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)

	faces = faceCascade.detectMultiScale(
		gray,
		scaleFactor=1.1,
		minNeighbors=10,
		minSize=(50, 50),
		flags=cv2.cv.CV_HAAR_SCALE_IMAGE
	)
	for (x, y, w, h) in faces:
		cv2.rectangle(img_np, (x, y), (x+w, y+h), (0, 200, 100), 10)
	
	param = [int(cv2.IMWRITE_PXM_BINARY),1]
	ret, buf = cv2.imencode(".ppm", img_np, param)
	resimg = buf.tobytes()
	return bytearray(resimg)