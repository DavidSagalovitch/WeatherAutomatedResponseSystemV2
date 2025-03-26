import serial
import os

# Configuration
SERIAL_PORT = "COM12"  # Change to your actual port
BAUD_RATE = 1000000
# Get the current script directory
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_FILE = os.path.join(SCRIPT_DIR, "captured_image.jpg")

def receive_image():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5)
    print("Waiting for image data...")

    image_data = bytearray()
    receiving = False

    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()

        if "IMAGE_START" in line:
            print("Image transmission started.")
            receiving = True
            image_data.clear()
            continue

        if "IMAGE_END" in line:
            print("Image transmission ended.")
            break

        if receiving:
            # Convert hex string to bytes
            hex_values = line.split()
            for hex_val in hex_values:
                try:
                    image_data.append(int(hex_val, 16))
                except ValueError:
                    pass  # Ignore any invalid hex values

    # Save the image data to a file in the same directory as the script
    with open(OUTPUT_FILE, "wb") as f:
        f.write(image_data)

    print(f"Image saved as {OUTPUT_FILE}")
    ser.close()

receive_image()
