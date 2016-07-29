import cv2
import numpy as np

fast = cv2.FastFeatureDetector()

def process(data):
	nparr = np.frombuffer(data, np.uint8)
	img_np = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

	fast.setBool('nonmaxSuppression',0)
	kp = fast.detect(img_np,None)
	res_img = cv2.drawKeypoints(img_np, kp, color=(255,0,0))

	param = [int(cv2.IMWRITE_PXM_BINARY),1]
	ret, buf = cv2.imencode(".ppm", res_img, param)
	resimg = buf.tobytes()
	return bytearray(resimg)