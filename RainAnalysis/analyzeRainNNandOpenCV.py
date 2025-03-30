from flask import Flask, request
import numpy as np
import cv2
from ultralytics import YOLO
import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk
import cv2
import io
import threading

IMAGE_PATH = "latest.jpg"  # Image saved by Arduino upload
app = Flask(__name__)

# Load YOLOv5/v8 model for raindrop detection
model = YOLO("runs/detect/raindrop_detector_best2/weights/best.pt")  # Update path as needed

def detect_rain_opencv(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (7, 7), 0)
    edges = cv2.Canny(blurred, 30, 100)
    #edges = cv2.Canny(blurred, 50, 150)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rain_blobs = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        perimeter = cv2.arcLength(cnt, True)
        circularity = (4 * np.pi * area / (perimeter * perimeter)) if perimeter > 0 else 0

        #if 80 < area < 800 and circularity > 0.5:
        if 30 < area < 1000 and circularity > 0.5:
            mask = np.zeros(gray.shape, dtype=np.uint8)
            cv2.drawContours(mask, [cnt], -1, 255, -1)
            mean_brightness = cv2.mean(gray, mask=mask)[0]
            if mean_brightness > 100: #120
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


# RAM-only storage
latest_image_bytes = None
latest_overlay_bytes = None
latest_intensity = 0

@app.route('/upload', methods=['POST'])
def upload():
    global latest_image_bytes, latest_overlay_bytes, latest_intensity

    try:
        image_bytes = request.data
        if not image_bytes:
            return "No image received", 400

        # Decode once
        np_arr = np.frombuffer(image_bytes, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        if img is None:
            return "Image decoding failed", 400

        # Store the original raw image bytes in RAM
        latest_image_bytes = image_bytes

        # Run YOLO to check if any raindrops are detected
        overlay, detected = detect_raindrops_yolo(img, min_detections=2)

        if detected:
            print("✔️ YOLO detected raindrops — estimating intensity with OpenCV...")
            overlay, latest_intensity = detect_rain_opencv(img)
        else:
            print("❌ YOLO did NOT detect raindrops — skipping OpenCV, intensity set to 0.")
            overlay = img
            latest_intensity = 0


        _, buffer = cv2.imencode('.jpg', overlay)
        latest_overlay_bytes = buffer.tobytes()

        print(f"✔️ Image processed. Intensity: {latest_intensity}")
        return str(latest_intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

class RainApp:
    def __init__(self, master):
        self.master = master
        master.title("Rain Detection Viewer")
        self.last_overlay_image = None
        self.img_display = None
        self.last_seen_bytes = None  # Track changes
        self.intensity_text = ""

        self.canvas = tk.Canvas(master, bg="black")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.master.update_idletasks()

        self.master.bind("<Configure>", lambda e: self.update_display())
        self.poll_for_update()  # Start monitoring

    def poll_for_update(self):
        from __main__ import latest_overlay_bytes, latest_intensity  # Access globals in shared script

        try:
            if latest_overlay_bytes and latest_overlay_bytes != self.last_seen_bytes:
                self.last_seen_bytes = latest_overlay_bytes
                self.last_overlay_image = latest_overlay_bytes
                self.intensity_text = f"Rain Intensity: {latest_intensity}"
                self.update_display()
        except Exception as e:
            print(f"❌ GUI Update Error: {e}")

        self.master.after(100, self.poll_for_update)

    def update_display(self):
        if self.last_overlay_image is None:
            return

        win_width = self.canvas.winfo_width()
        win_height = self.canvas.winfo_height()

        image_stream = io.BytesIO(self.last_overlay_image)
        pil_img = Image.open(image_stream)
        pil_img = pil_img.resize((win_width, win_height), Image.Resampling.LANCZOS)
        self.img_display = ImageTk.PhotoImage(pil_img)

        self.canvas.delete("all")
        self.canvas.create_image(0, 0, anchor=tk.NW, image=self.img_display)
        
        # Overlay rain intensity text in red, large font
        font_size = max(20, win_width // 20)  # Scale font size based on window width
        self.canvas.create_text(
            win_width // 2, win_height - 70,  # Position near the bottom center
            text=self.intensity_text, 
            fill="Violet", 
            font=("Impact", font_size, "bold")
        )


def start_flask():
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)

if __name__ == "__main__":
    flask_thread = threading.Thread(target=start_flask, daemon=True)
    flask_thread.start()
    root = tk.Tk()
    app = RainApp(root)
    root.mainloop()


