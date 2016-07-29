import cv2
import numpy as np
import json

fgbg = cv2.BackgroundSubtractorMOG()

def process(data):
	nparr = np.frombuffer(data, np.uint8)
	img_np = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

	fgmask = fgbg.apply(img_np)

	param = [int(cv2.IMWRITE_PXM_BINARY),1]
	ret, buf = cv2.imencode(".ppm", fgmask, param)
	resimg = buf.tobytes()
	return bytearray(resimg)