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

class RainApp:
    def __init__(self, master):
        self.master = master
        master.title("Rain Detection Viewer")
        self.last_overlay_image = None
        self.img_display = None
        self.last_seen_bytes = None  # Track changes

        self.label = tk.Label(master, text="Waiting for image from Arduino...")
        self.label.pack()

        self.canvas = tk.Canvas(master, bg="black")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.master.update_idletasks()

        self.intensity_label = tk.Label(master, text="")
        self.intensity_label.pack()
        self.master.bind("<Configure>", lambda e: self.update_display())

        self.poll_for_update()  # Start monitoring

    def poll_for_update(self):
        from __main__ import latest_overlay_bytes, latest_intensity  # access globals in shared script

        try:
            if latest_overlay_bytes and latest_overlay_bytes != self.last_seen_bytes:
                self.last_seen_bytes = latest_overlay_bytes
                self.last_overlay_image = latest_overlay_bytes
                self.update_display()
                self.intensity_label.config(text=f"Rain Intensity: {latest_intensity}")
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


def start_flask():
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)

if __name__ == "__main__":
    flask_thread = threading.Thread(target=start_flask, daemon=True)
    flask_thread.start()
    root = tk.Tk()
    app = RainApp(root)
    root.mainloop()

