# IndividualPR — Emotion Music

A webcam watches your face, a Google Teachable Machine model decides whether you look happy or sad, and an Arduino plays a different song on a buzzer for each one.

- **Happy** → Twinkle, Twinkle, Little Star
- **Sad** → Jingle Bells
- **Background / neutral face** → silence

Python does the vision work and sends a one-word command (`happy`, `sad`, or `off`) over the serial port. The Arduino listens for those words and plays the matching melody on a piezo buzzer.

---

## Demo

<!-- VIDEO: drop your demo clip here.
     Easiest way: open this README on GitHub, click the pencil icon to edit,
     then drag the video file into the editor. GitHub uploads it and pastes
     a link in automatically — replace this comment with that link.
     Works with .mp4, .mov, and .webm up to 10 MB.
     Longer clip? Upload to YouTube and use the thumbnail-link form below. -->

> _Demo video goes here._

<!-- YouTube version (uncomment and swap in your video ID):
[![Watch the demo](https://img.youtube.com/vi/VIDEO_ID/maxresdefault.jpg)](https://youtu.be/VIDEO_ID)
-->

### Examples

<!-- IMAGES: put your screenshots in a folder called `media/` at the root of
     the repo, then uncomment the lines below and fix the filenames.
     Or drag images straight into the GitHub editor like the video above. -->

| Happy | Sad | Idle |
|---|---|---|
| _screenshot_ | _screenshot_ | _screenshot_ |

<!--
| Happy | Sad | Idle |
|---|---|---|
| ![Happy face detected, playing Twinkle Twinkle](media/happy.png) | ![Sad face detected, playing Jingle Bells](media/sad.png) | ![No face detected, buzzer silent](media/idle.png) |
-->

<!-- Wiring photo — same deal, drop the file in `media/` and uncomment:
![Buzzer wired to pin 12 and ground](media/wiring.jpg)
-->

---

## What's in here

| File | What it does |
|---|---|
| `GoogleTeachableCode-KerasOpenCV-Serial.py` | Runs the webcam, classifies each frame, sends commands to the Arduino |
| `model.ino` | Arduino sketch — reads commands and plays the songs on a buzzer |
| `keras_model.h5` | The trained Teachable Machine model |
| `labels.txt` | Class names the model was trained on |

---

## Hardware

- An Arduino (Uno or anything similar)
- A piezo buzzer — positive leg to **pin 12**, other leg to **GND**
- A webcam
- A USB cable between the Arduino and the computer

## Software setup

You need Python 3 and the Arduino IDE.

```bash
pip install opencv-python numpy pyserial tf-keras
```

The script sets `TF_USE_LEGACY_KERAS=1` before importing anything, which is why it uses `tf-keras` instead of plain Keras — Teachable Machine exports models in the older format.

## Running it

1. Open `model.ino` in the Arduino IDE and upload it to the board.
2. **Close the Serial Monitor.** Only one program can hold the port at a time, and if the Arduino IDE has it, Python can't connect.
3. Check the port name near the top of the Python file:

   ```python
   SERIAL_PORT = "COM10"
   ```

   On Windows that's whatever COM number the Arduino IDE shows. On macOS or Linux it looks more like `/dev/ttyACM0` or `/dev/cu.usbmodem14101`.
4. Run it:

   ```bash
   python GoogleTeachableCode-KerasOpenCV-Serial.py
   ```
5. A window opens showing the webcam feed with the predicted label, the confidence, and the song currently playing. Click the window and press **Q** or **Esc** to quit.

## Settings you can change

All near the top of the Python file:

| Setting | Default | What it does |
|---|---|---|
| `SERIAL_PORT` | `"COM10"` | Which port the Arduino is on |
| `BAUD_RATE` | `9600` | Has to match `Serial.begin()` in the sketch |
| `CONFIDENCE_THRESHOLD` | `0.80` | How sure the model has to be before a song starts. Lower it if it feels unresponsive, raise it if it triggers on nothing |
| `SEND_INTERVAL` | `0.25` | How often a repeat command gets sent, in seconds |

In `model.ino`, `beatMs` sets the tempo for each song (350 for Twinkle, 250 for Jingle Bells) and `TIMEOUT_MS` is the safety timeout.

## How the two sides talk

Python sends a word plus a newline, nothing else. The Arduino reads characters until it hits `\n`, compares the result against `happy`, `sad`, and `off`, and switches songs.

Two details worth knowing:

- **Repeat commands don't restart the song.** `selectSong()` returns early if you ask for the song that's already playing, so the melody keeps going instead of stuttering back to the first note.
- **The buzzer shuts itself off.** If the Arduino doesn't hear anything for 2 seconds it goes silent on its own. So if you close the Python script, kill the terminal, or unplug the webcam, the buzzer stops instead of playing forever.

## Troubleshooting

**"Arduino connection error" / can't open the port**
The Serial Monitor is probably still open, or `SERIAL_PORT` is wrong. Close the monitor and double-check the port.

**"Cannot open webcam"**
Another app has the camera, or it isn't camera index 0. Change `cv2.VideoCapture(0)` to `cv2.VideoCapture(1)`.

**It predicts the wrong thing constantly**
Lighting is usually the culprit — Teachable Machine models are picky about matching the conditions they were trained in. Retrain with samples from where you'll actually be using it, or lower `CONFIDENCE_THRESHOLD` to see what it's guessing.

**Nothing comes out of the buzzer**
Check the buzzer legs are on pin 12 and GND, and that the terminal is actually printing a label change. If Python says "happy" and the buzzer is quiet, it's the wiring.
