from flask import Flask, request
import numpy as np
import cv2
import datetime

app = Flask(__name__)

def detect_rain(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Apply stronger Gaussian Blur to reduce background texture
    blurred = cv2.GaussianBlur(gray, (7, 7), 0)

    # Edge detection with higher thresholds to avoid small noise
    edges = cv2.Canny(blurred, 50, 150)

    # Morphological closing to fill gaps in raindrop outlines
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

    # Find contours
    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rain_blobs = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        perimeter = cv2.arcLength(cnt, True)
        circularity = (4 * np.pi * area / (perimeter * perimeter)) if perimeter > 0 else 0

        # Filter for typical raindrop size and shape (somewhat circular)
        if 80 < area < 800 and circularity > 0.5:
            # Optional: brightness mask filter
            mask = np.zeros(gray.shape, dtype=np.uint8)
            cv2.drawContours(mask, [cnt], -1, 255, -1)
            mean_brightness = cv2.mean(gray, mask=mask)[0]
            if mean_brightness > 120:  # raindrops are often bright reflections
                rain_blobs.append(cnt)

    # Overlay detected raindrops
    overlay = image.copy()
    for cnt in rain_blobs:
        x, y, w, h = cv2.boundingRect(cnt)
        cv2.rectangle(overlay, (x, y), (x + w, y + h), (0, 0, 255), 1)

    blob_count = len(rain_blobs)
    total_blob_area = sum([cv2.contourArea(c) for c in rain_blobs])
    image_area = image.shape[0] * image.shape[1]
    coverage_ratio = total_blob_area / image_area

    # Make intensity rely more on coverage
    intensity = int(min(100, coverage_ratio * 10000))  # scale factor tuned

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

        # Run rain detection
        print("Running OpenCV-based rain detection...")
        result_img, latest_intensity = detect_rain(img)

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
