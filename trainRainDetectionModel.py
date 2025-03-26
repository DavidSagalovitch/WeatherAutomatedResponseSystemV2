from ultralytics import YOLO

if __name__ == "__main__":
    model = YOLO("yolov8m.pt")  # Start with 'n' if it crashes again

    results = model.train(
        data="C:/Users/dsaga/Documents/CAPSTONE/WeatherAutomatedResponseSystemV2/raindrop.v4i.yolov8/data.yaml",
        epochs=100,
        imgsz=640,
        batch=8,        # 🧠 Safer batch size for laptops, reduce to 16
        device=0,
        workers=2,       # ✅ Great call here
        cache=False,     # 🔒 Prevent memory spike on Windows
        name="raindrop_detector"
    )
