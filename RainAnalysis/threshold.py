import cv2
import numpy as np
import os
import matplotlib.pyplot as plt

def compute_combined_variance(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    height, width = gray.shape

    horizontal_var = np.mean([np.var(gray[y, :]) for y in range(0, height, max(1, height // 50))])
    vertical_var = np.mean([np.var(gray[:, x]) for x in range(0, width, max(1, width // 50))])
    return (horizontal_var + vertical_var) / 2

def load_variances(folder):
    variances = []
    for filename in os.listdir(folder):
        path = os.path.join(folder, filename)
        image = cv2.imread(path)
        if image is None:
            continue
        var = compute_combined_variance(image)
        variances.append(var)
    return variances

# Set your paths here
foggy_path = "foggy"
clear_path = "clear"

foggy_vars = load_variances(foggy_path)
clear_vars = load_variances(clear_path)

# Plot histogram
plt.hist(foggy_vars, bins=20, alpha=0.7, label="Foggy", color='gray')
plt.hist(clear_vars, bins=20, alpha=0.7, label="Clear", color='green')
plt.axvline(np.mean(foggy_vars), color='black', linestyle='--', label='Avg Foggy')
plt.axvline(np.mean(clear_vars), color='lime', linestyle='--', label='Avg Clear')

# Suggest threshold
threshold = (np.mean(foggy_vars) + np.mean(clear_vars)) / 2
plt.axvline(threshold, color='red', linestyle='-', label=f"Suggested Threshold ≈ {threshold:.1f}")

plt.xlabel("Combined Intensity Variance")
plt.ylabel("Number of Images")
plt.title("Fog Detection Threshold Analysis")
plt.legend()
plt.grid(True)
plt.show()

print(f"\n📌 Suggested fog_threshold: {threshold:.2f}")