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
