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

app = Flask(__name__)

def detect_rain(image):
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

