from flask import Flask, request
import numpy as np
import cv2
import datetime
import torch
from PIL import Image
from ultralytics import YOLO

app = Flask(__name__)

# Load YOLOv5 or YOLOv8 model trained for raindrop detection
model = YOLO("runs/detect/raindrop_detector_best2/weights/best.pt")  # Replace with your actual model file

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

# RAM-only storage
latest_image_bytes = None
latest_overlay_bytes = None
latest_intensity = 0

@app.route('/upload', methods=['POST'])
def upload():
    global latest_image_bytes, latest_overlay_bytes, latest_intensity
    try:
        image_bytes = request.data
        print("First 8 bytes of data:", image_bytes[:8])

        latest_image_bytes = image_bytes  # store raw
        np_arr = np.frombuffer(image_bytes, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        if img is None:
            print("Failed to decode image.")
            return "Image decoding failed", 400

        # Run rain detection using YOLO
        print("Running YOLO raindrop detection...")
        result_img, latest_intensity = detect_raindrops_yolo(img)

        # Encode overlay to JPEG and store in memory
        _, buffer = cv2.imencode('.jpg', result_img)
        latest_overlay_bytes = buffer.tobytes()

        print(f"Rain detection done. Intensity: {latest_intensity}")
        return str(latest_intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

@app.route('/get_overlay', methods=['GET'])
def get_overlay():
    if latest_overlay_bytes is None:
        return "No overlay available", 404

    return latest_overlay_bytes, 200, {'Content-Type': 'image/jpeg'}

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
