from flask import Flask, request
import numpy as np
import cv2
import datetime
import torch
from PIL import Image
from ultralytics import YOLO

app = Flask(__name__)

# Load YOLOv5 or YOLOv8 model trained for raindrop detection
model = YOLO("yolov5_raindrops.pt")  # Replace with your actual model file

def detect_raindrops_yolo(image):
    # Convert OpenCV image (BGR) to RGB and run inference
    img_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    results = model.predict(img_rgb, imgsz=640, conf=0.3)

    overlay = image.copy()
    intensity = 0

    for r in results:
        boxes = r.boxes
        if boxes is not None and len(boxes) > 0:
            for box in boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cv2.rectangle(overlay, (x1, y1), (x2, y2), (0, 0, 255), 2)
            intensity = int(min(100, len(boxes) * 2))  # Scaled intensity based on count

    return overlay, intensity

@app.route('/upload', methods=['POST'])
def upload():
    try:
        image_bytes = request.data
        print("First 8 bytes of data:", image_bytes[:8])
        np_arr = np.frombuffer(image_bytes, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        if img is None:
            print("Failed to decode image.")
            return "Image decoding failed", 400

        print("Running YOLO raindrop detection...")
        result_img, intensity = detect_raindrops_yolo(img)

        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"rain_overlay.jpg"
        cv2.imwrite(output_filename, result_img)

        print(f"Result saved. Rain Intensity: {intensity}")
        return str(intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
