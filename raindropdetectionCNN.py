from flask import Flask, request
import numpy as np
import cv2
import datetime
import tflearn
from tflearn.layers.core import input_data, dropout, fully_connected
from tflearn.layers.conv import conv_2d, max_pool_2d
from tflearn.layers.normalization import local_response_normalization
from tflearn.layers.estimator import regression

app = Flask(__name__)

# Define the AlexNet-30² architecture

def create_basic_alexnet():
    network = input_data(shape=[None, 30, 30, 3])
    network = conv_2d(network, 96, 11, strides=4, activation='relu')
    network = max_pool_2d(network, 3, strides=2)
    network = local_response_normalization(network)
    network = conv_2d(network, 256, 5, activation='relu')
    network = max_pool_2d(network, 3, strides=2)
    network = local_response_normalization(network)
    network = conv_2d(network, 384, 3, activation='relu')
    network = conv_2d(network, 384, 3, activation='relu')
    network = conv_2d(network, 256, 3, activation='relu')
    network = max_pool_2d(network, 3, strides=2)
    network = local_response_normalization(network)
    network = fully_connected(network, 4096, activation='tanh')
    network = dropout(network, 0.5)
    network = fully_connected(network, 4096, activation='tanh')
    network = dropout(network, 0.5)
    network = fully_connected(network, 2, activation='softmax')
    network = regression(network, optimizer='momentum', loss='categorical_crossentropy')
    return network

# Load the pretrained raindrop classifier
alexnet = create_basic_alexnet()
model = tflearn.DNN(alexnet)
model.load("models/alexnet_30_2_classification.tfl", weights_only=True)

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

        # Preprocess for AlexNet-30²
        img_resized = cv2.resize(img, (30, 30))
        img_rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB).astype('float32') / 255.0

        # Predict with model
        prediction = model.predict([img_rgb])
        predicted_label = np.argmax(prediction[0])
        print("Prediction:", prediction[0])

        if predicted_label == 1:
            print("🌧️ Raindrop detected!")
            result_text = "Raindrop detected"
            intensity = 100  # Set high intensity if classified as raindrop
        else:
            print("☀️ No raindrop.")
            result_text = "No raindrop detected"
            intensity = 0

        # Save the received image for review
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"rain_overlay.jpg"
        cv2.imwrite(output_filename, img)

        print(f"Result saved. Rain Intensity: {intensity}")
        return str(intensity), 200

    except Exception as e:
        print(f"Error: {e}")
        return "Internal Server Error", 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
