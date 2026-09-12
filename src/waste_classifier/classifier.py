import time
import sys

import cv2
import numpy as np
import serial
from tensorflow.keras.applications.mobilenet_v2 import (
    MobileNetV2,
    preprocess_input,
)
from tensorflow.keras.preprocessing.image import img_to_array

SERIAL_PORT = "COM5"
BAUD_RATE = 9600
CAMERA_INDEX = 0
CAPTURE_INTERVAL = 4.0
CONFIDENCE_THRESHOLD = 0.55
MODEL_PATH = "./model.h5"

CLASS_LABELS = ["PLASTIC", "METAL", "PAPER"]
INPUT_SIZE = (224, 224)

def load_model():
    try:
        from tensorflow.keras.models import load_model as keras_load_model
        model = keras_load_model(MODEL_PATH)
        print(f"[INFO] Loaded fine-tuned classifier from '{MODEL_PATH}'.")
        return model, True
    except (IOError, OSError):
        print(
            f"[WARN] Could not find '{MODEL_PATH}'. Falling back to base "
            "MobileNetV2 (ImageNet weights). Replace with your fine-tuned "
            "model for real plastic/metal/paper classification."
        )
        base_model = MobileNetV2(weights="imagenet", include_top=True)
        return base_model, False


def preprocess_frame(frame):
    resized = cv2.resize(frame, INPUT_SIZE)
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    array = img_to_array(rgb)
    array = np.expand_dims(array, axis=0)
    return preprocess_input(array)


def classify_frame(model, frame, is_finetuned):
    processed = preprocess_frame(frame)
    preds = model.predict(processed, verbose=0)

    if is_finetuned:
        idx = int(np.argmax(preds[0]))
        confidence = float(preds[0][idx])
        label = CLASS_LABELS[idx] if idx < len(CLASS_LABELS) else "UNKNOWN"
        return label, confidence

    from tensorflow.keras.applications.mobilenet_v2 import decode_predictions
    decoded = decode_predictions(preds, top=3)[0]
    top_label, confidence = decoded[0][1].lower(), float(decoded[0][2])

    keyword_map = {
        "PLASTIC": ["bottle", "plastic", "cup", "container"],
        "METAL": ["can", "tin", "metal", "aluminum"],
        "PAPER": ["paper", "carton", "cardboard", "envelope"],
    }
    for category, keywords in keyword_map.items():
        if any(k in top_label for k in keywords):
            return category, confidence

    return "UNKNOWN", confidence


def main():
    model, is_finetuned = load_model()

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        time.sleep(2)
        print(f"[INFO] Connected to Arduino Mega on {SERIAL_PORT}.")
    except serial.SerialException as e:
        print(f"[ERROR] Could not open serial port {SERIAL_PORT}: {e}")
        sys.exit(1)

    cap = cv2.VideoCapture(CAMERA_INDEX)
    if not cap.isOpened():
        print("[ERROR] Could not open camera.")
        sys.exit(1)

    print("[INFO] Waste classifier running. Press 'c' to classify, 'q' to quit.")
    last_auto_trigger = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[WARN] Failed to read frame from camera.")
            continue

        cv2.imshow("Waste Segregation - Live Feed", frame)
        key = cv2.waitKey(1) & 0xFF

        auto_trigger = (time.time() - last_auto_trigger) >= CAPTURE_INTERVAL
        manual_trigger = key == ord("c")

        if auto_trigger or manual_trigger:
            label, confidence = classify_frame(model, frame, is_finetuned)
            last_auto_trigger = time.time()

            if confidence < CONFIDENCE_THRESHOLD:
                label = "UNKNOWN"

            print(f"[CLASSIFY] {label} (confidence: {confidence:.2f})")
            command = f"{label}\n"
            ser.write(command.encode("utf-8"))

        if key == ord("q"):
            print("[INFO] Shutting down.")
            break

    cap.release()
    cv2.destroyAllWindows()
    ser.close()


if __name__ == "__main__":
    main()
  
