import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk
import cv2
import os
import time
import requests
import io

IMAGE_PATH = "latest.jpg"  # Image saved by Arduino upload

class RainApp:
    def __init__(self, master):
        self.master = master
        master.title("Rain Detection Viewer")
        self.last_overlay_image = None
        self.last_modified = None
        self.image = None
        self.img_display = None

        self.label = tk.Label(master, text="Waiting for image from Arduino...")
        self.label.pack()

        self.canvas = tk.Canvas(master, bg="black")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.master.update_idletasks()

        self.intensity_label = tk.Label(master, text="")
        self.intensity_label.pack()
        self.master.bind("<Configure>", lambda e: self.update_display())

        self.poll_for_image()  # Start monitoring

    def poll_for_image(self):
        try:
            response = requests.get("http://localhost:5000/get_overlay")
            if response.status_code == 200:
                overlay_bytes = response.content
                self.last_overlay_image = overlay_bytes
                self.update_display()
                self.intensity_label.config(text="Overlay updated.")
            else:
                print("No image available from Flask yet.")
        except Exception as e:
            print(f"❌ Error getting overlay: {e}")

        self.master.after(100, self.poll_for_image)


    def update_display(self):
        if self.last_overlay_image is None:
            return

        win_width = self.canvas.winfo_width()
        win_height = self.canvas.winfo_height()

        image_stream = io.BytesIO(self.last_overlay_image)
        pil_img = Image.open(image_stream)

        # Resize while keeping aspect ratio
        pil_img = pil_img.resize((win_width, win_height), Image.Resampling.LANCZOS)
        self.img_display = ImageTk.PhotoImage(pil_img)

        self.canvas.delete("all")  # Clear old image
        self.canvas.create_image(0, 0, anchor=tk.NW, image=self.img_display)

    def request_overlay_only(self):
        try:
            if self.image is None:
                return

            _, encoded_image = cv2.imencode('.jpg', self.image)
            img_bytes = encoded_image.tobytes()

            response = requests.post("http://localhost:5000/get_overlay", data=img_bytes)
            if response.status_code != 200:
                raise Exception(f"Overlay error: {response.text}")

            overlay_bytes = response.content
            self.last_overlay_image = overlay_bytes
            self.update_display()

            self.intensity_label.config(text="Overlay updated.")

        except Exception as e:
            print(f"❌ Error: {e}")
            messagebox.showerror("Error", str(e))



if __name__ == "__main__":
    root = tk.Tk()
    app = RainApp(root)
    root.mainloop()
