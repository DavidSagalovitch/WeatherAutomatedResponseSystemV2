from flask import Flask, request
import numpy as np
import cv2
import datetime
import torch
from ultralytics import YOLO

app = Flask(__name__)

# Load YOLOv5/v8 model for raindrop detection
model = YOLO("runs/detect/raindrop_detector_best2/weights/best.pt")  # Update path as needed

def detect_rain_opencv(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (7, 7), 0)
    edges = cv2.Canny(blurred, 50, 150)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rain_blobs = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        perimeter = cv2.arcLength(cnt, True)
        circularity = (4 * np.pi * area / (perimeter * perimeter)) if perimeter > 0 else 0

        if 80 < area < 800 and circularity > 0.5:
            mask = np.zeros(gray.shape, dtype=np.uint8)
            cv2.drawContours(mask, [cnt], -1, 255, -1)
            mean_brightness = cv2.mean(gray, mask=mask)[0]
            if mean_brightness > 120:
                rain_blobs.append(cnt)

    overlay = image.copy()
    for cnt in rain_blobs:
        x, y, w, h = cv2.boundingRect(cnt)
        cv2.rectangle(overlay, (x, y), (x + w, y + h), (0, 0, 255), 1)

    total_blob_area = sum([cv2.contourArea(c) for c in rain_blobs])
    image_area = image.shape[0] * image.shape[1]
    coverage_ratio = total_blob_area / image_area

    intensity = int(min(100, coverage_ratio * 10000))  # scale factor

    return overlay, intensity

def detect_raindrops_yolo(image, min_detections=2):
    img_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    results = model.predict(img_rgb, imgsz=640, conf=0.3)

    overlay = image.copy()
    box_count = 0

    for r in results:
        boxes = r.boxes
        if boxes is not None and len(boxes) > 0:
            box_count = len(boxes)
            for box in boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cv2.rectangle(overlay, (x1, y1), (x2, y2), (255, 0, 0), 2)

    raindrop_detected = box_count >= min_detections
    return overlay, raindrop_detected


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
        overlay_yolo, detected = detect_raindrops_yolo(img, min_detections=2)

        if detected:
            print("✔️ YOLO detected raindrops — estimating intensity with OpenCV...")
            final_overlay, intensity = detect_rain_opencv(img)
        else:
            print("❌ YOLO detected no raindrops — setting intensity to 0.")
            final_overlay = img.copy()
            intensity = 0

        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"rain_overlay.jpg"
        cv2.imwrite(output_filename, final_overlay)

        print(f"Result saved. Rain Intensity: {intensity}")
        return str(intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
