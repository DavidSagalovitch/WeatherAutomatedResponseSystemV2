from flask import Flask, request
import numpy as np
import cv2
import datetime

app = Flask(__name__)

def detect_rain(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Apply Gaussian Blur to reduce noise before edge detection
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Edge detection
    edges = cv2.Canny(blurred, 30, 100)

    # Morphological closing to fill gaps in raindrops
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

    # Find contours (potential raindrops)
    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rain_blobs = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if 20 < area < 1000:  # Adjust these thresholds based on expected drop size
            rain_blobs.append(cnt)

    # Overlay with filled blobs for better visibility
    overlay = image.copy()
    for cnt in rain_blobs:
        x, y, w, h = cv2.boundingRect(cnt)
        cv2.rectangle(overlay, (x, y), (x + w, y + h), (0, 0, 255), 1)

    # Estimate rain intensity
    blob_count = len(rain_blobs)
    print(blob_count)
    avg_area = np.mean([cv2.contourArea(c) for c in rain_blobs]) if rain_blobs else 0
    total_blob_area = sum([cv2.contourArea(c) for c in rain_blobs])

    # Scaled rain intensity based on total blob coverage and count
    image_area = image.shape[0] * image.shape[1]
    coverage_ratio = total_blob_area / image_area
    intensity = int(min(100, blob_count * coverage_ratio * 400))  # tweak scale factor

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

        # Run rain detection
        result_img, intensity = detect_rain(img)

        # Save output with overlay
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"rain_overlay.jpg"
        cv2.imwrite(output_filename, result_img)

        print(f"Rain detection done. Intensity: {intensity}")
        return str(intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
