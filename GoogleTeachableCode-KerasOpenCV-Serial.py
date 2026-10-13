import os
import sys
import time

os.environ["TF_USE_LEGACY_KERAS"] = "1"
sys.modules["pyarrow"] = None

import cv2
import numpy as np
import serial
from tf_keras.models import load_model

HERE = os.path.dirname(os.path.abspath(__file__))

SERIAL_PORT = "COM10"
BAUD_RATE = 9600
CONFIDENCE_THRESHOLD = 0.80
SEND_INTERVAL = 0.25

# 0 happy, 1 sad, 2 Background, 3 base face
COMMAND_FOR_CLASS = {
    0: "happy",
    1: "sad",
    2: "off",
    3: "off",
}

SONG_NAMES = {
    "happy": "Twinkle Twinkle",
    "sad": "Jingle Bells",
    "off": "Silent",
}

board = None
camera = None

try:
    model = load_model(
        os.path.join(HERE, "keras_model.h5"),
        compile=False
    )

    with open(
        os.path.join(HERE, "labels.txt"),
        encoding="utf-8"
    ) as f:
        class_names = [
            line.strip().split(" ", 1)[1]
            for line in f
            if line.strip()
        ]

    board = serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=1,
        write_timeout=1
    )
    time.sleep(2)
    board.write(b"off\n")

    print(f"Connected to Arduino on {SERIAL_PORT}")

    camera = cv2.VideoCapture(0)

    if not camera.isOpened():
        raise RuntimeError(
            "Cannot open webcam. Try VideoCapture(1)."
        )

    last_message = None
    last_sent = 0.0

    print("Click the webcam window. Press Q or Esc to quit.")

    while True:
        success, frame = camera.read()

        if not success:
            print("Could not read the webcam.")
            break

        # Crop the center square.
        height, width = frame.shape[:2]
        size = min(height, width)
        top = (height - size) // 2
        left = (width - size) // 2
        square = frame[top:top + size, left:left + size]

        # Prepare the model input.
        resized = cv2.resize(
            square,
            (224, 224),
            interpolation=cv2.INTER_AREA
        )
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
        image = rgb.astype(np.float32)
        image = (image / 127.5) - 1.0
        image = np.expand_dims(image, axis=0)

        prediction = model.predict(image, verbose=0)[0]

        index = int(np.argmax(prediction))
        confidence = float(prediction[index])
        class_name = class_names[index]

        message = "off"
        if confidence >= CONFIDENCE_THRESHOLD:
            message = COMMAND_FOR_CLASS.get(index, "off")

        # Send changes and refresh Arduino's timeout.
        now = time.monotonic()

        if (
            message != last_message
            or now - last_sent >= SEND_INTERVAL
        ):
            board.write((message + "\n").encode("utf-8"))

            if message != last_message:
                print(
                    f"{class_name}: {confidence:.0%} | "
                    f"{SONG_NAMES[message]}"
                )

            last_message = message
            last_sent = now

        display = cv2.resize(square, (480, 480))
        color = (
            (0, 255, 0)
            if message != "off"
            else (0, 0, 255)
        )

        cv2.putText(
            display,
            f"{class_name}: {confidence:.0%}",
            (10, 35),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            color,
            2
        )

        cv2.putText(
            display,
            SONG_NAMES[message],
            (10, 75),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            color,
            2
        )

        cv2.imshow("Emotion Music", display)

        key = cv2.waitKey(1) & 0xFF
        if key in (27, ord("q")):
            break

except serial.SerialException as error:
    print(f"Arduino connection error: {error}")
    print("Check COM10 and close Arduino Serial Monitor.")

except KeyboardInterrupt:
    print("Stopping.")

finally:
    if board is not None:
        try:
            if board.is_open:
                board.write(b"off\n")
                board.flush()
        except serial.SerialException:
            pass
        finally:
            board.close()

    if camera is not None:
        camera.release()

    cv2.destroyAllWindows()