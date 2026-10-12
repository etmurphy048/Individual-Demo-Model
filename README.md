# Individual Demo Model — Teachable Machine → Python → Arduino

A webcam image classifier trained in [Google Teachable Machine](https://teachablemachine.withgoogle.com/)
and run locally in Python. The model watches the camera, decides which of three
classes it is looking at, and (in the serial version) sends that decision over USB
to an Arduino board.

Built for **IT254** as an individual demo.

![Python](https://img.shields.io/badge/python-3.9%E2%80%933.11-blue)
![TensorFlow](https://img.shields.io/badge/tensorflow-tf--keras%20(legacy)-orange)
![OpenCV](https://img.shields.io/badge/opencv-webcam-green)
![License](https://img.shields.io/badge/license-unlicensed-lightgrey)

---

## Table of contents

- [Overview](#overview)
- [Repository contents](#repository-contents)
- [How it works](#how-it-works)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Arduino side](#arduino-side)
- [Troubleshooting](#troubleshooting)
- [Known limitations](#known-limitations)
- [Credits](#credits)

---

## Overview

Teachable Machine makes it easy to train an image model in the browser, but the
exported model is only half of a project. This repo is the other half: the code
that loads that export, runs it on a live webcam feed, and turns its predictions
into something physical.

The model was trained on three classes:

| Index | Label     | Meaning                              |
|:-----:|-----------|--------------------------------------|
| 0     | `Blue`    | Show the blue object to the camera   |
| 1     | `Red`     | Show the red object to the camera    |
| 2     | `Nothing` | Empty frame / background             |

There are two scripts. The first is a classifier only — useful for checking that
the model actually works before any hardware is involved. The second adds the
serial link to an Arduino.

---

## Repository contents

| File | Purpose |
|------|---------|
| `GoogleTeachableCode-KerasOpenCV.py` | Classify webcam frames and print the result. No hardware needed. |
| `GoogleTeachableCode-KerasOpenCV-Serial.py` | Same classifier, plus sends a message over USB serial to an Arduino when the prediction changes. |
| `keras_model.h5` | The trained Teachable Machine model (~2.4 MB, Keras 2 format). |
| `labels.txt` | Class indices and names, one per line, as exported by Teachable Machine. |

> The Arduino sketch is **not** in this repo — see [Arduino side](#arduino-side)
> for what the board needs to do.

---

## How it works

Both scripts run the same per-frame pipeline, matched to how Teachable Machine
preprocesses images during training:

1. **Capture** a frame from the default webcam (`cv2.VideoCapture(0)`).
2. **Mirror** it horizontally — serial version only, so the preview behaves like a mirror.
3. **Center-crop** to a square, so resizing does not squash the image.
4. **Resize** to 224 × 224 pixels, the model's input size.
5. **Convert** BGR → RGB, since OpenCV and Keras disagree on channel order.
6. **Normalize** pixel values to the range −1 … 1 (`value / 127.5 - 1`).
7. **Predict**, take `argmax` of the output, and read off the label and confidence.

The serial version then adds two pieces of logic that matter a lot in practice:

- **A confidence gate.** A prediction below `CONFIDENCE_THRESHOLD` (default `0.90`)
  sends nothing. One blurry frame should not flip the hardware.
- **Change detection.** A message is only sent when it differs from the last one
  sent. Without this the script would spam the board dozens of times per second.

Two environment tweaks happen before TensorFlow is imported, and both are
deliberate:

```python
os.environ["TF_USE_LEGACY_KERAS"] = "1"  # Teachable Machine exports Keras 2; Keras 3 can't load it
sys.modules["pyarrow"] = None            # pyarrow can crash TF on import on macOS
```

---

## Requirements

- Python 3.9 – 3.11 (TensorFlow does not yet support the newest releases)
- A webcam
- For the serial script: an Arduino (tested on Uno and Mega) connected over USB

Python packages:

```
opencv-python
numpy
tensorflow
tf-keras
pyserial
```

---

## Installation

```bash
# Clone and enter the project
git clone <your-repo-url>
cd "Individual Demo Model"

# Create and activate a virtual environment
python -m venv .venv
# Windows (PowerShell)
.venv\Scripts\Activate.ps1
# macOS / Linux
source .venv/bin/activate

# Install dependencies
pip install opencv-python numpy tensorflow tf-keras pyserial
```

The scripts locate `keras_model.h5` and `labels.txt` relative to their own
location, so you can run them from any working directory.

---

## Usage

### 1. Classifier only — start here

```bash
python GoogleTeachableCode-KerasOpenCV.py
```

A webcam window opens and the terminal prints a line per frame:

```
Class: Blue   Confidence Score: 98 %
Class: Blue   Confidence Score: 97 %
Class: Nothing   Confidence Score: 91 %
```

Click the webcam window, then press **Esc** or **q** to quit. `Ctrl+C` in the
terminal also works.

Confirm the model recognizes your objects reliably before moving on.

### 2. With the Arduino

Set `SERIAL_PORT` to your board's port (see [Configuration](#configuration)), then:

```bash
python GoogleTeachableCode-KerasOpenCV-Serial.py
```

```
Connected to COM8 at 9600 baud.
Click the webcam window, then press Esc or q to quit (or Ctrl+C in the terminal).
Class: Blue   Confidence Score: 99 %
Sent: Blue
Class: Red   Confidence Score: 96 %
Sent: Red
```

If the port cannot be opened the script prints a warning and **keeps running**,
so you can test the camera and model without the hardware attached.

---

## Configuration

All settings live near the top of `GoogleTeachableCode-KerasOpenCV-Serial.py`.

| Setting | Default | Notes |
|---------|---------|-------|
| `SERIAL_PORT` | `"COM8"` | Windows: `COM3`, `COM7`, … · macOS: `/dev/cu.usbmodem14101`. Find it in the Arduino IDE under **Tools → Port**. |
| `BAUD_RATE` | `9600` | Must match `Serial.begin()` in the sketch. |
| `CONFIDENCE_THRESHOLD` | `0.90` | Raise it if the output is jittery; lower it if the model never fires. |
| `MESSAGE_FOR_LABEL` | `{0: "Blue", 1: "Red", 2: "Nothing"}` | Maps each label index to the string sent over serial. Omit an index to send nothing for that class. |
| Camera index | `0` | Change `cv2.VideoCapture(0)` to `1` if the wrong camera opens. |

Keeping `MESSAGE_FOR_LABEL` in sync with `labels.txt` is the one thing worth
double-checking — the script reads names from the file but sends the strings from
the dictionary.

---

## Arduino side

The board is expected to read newline-terminated strings. The Python side sends
exactly `Blue\n`, `Red\n`, or `Nothing\n`. A minimal matching sketch:

```cpp
void setup() {
  Serial.begin(9600);          // must match BAUD_RATE
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg == "Blue")       { /* ... */ }
    else if (msg == "Red")   { /* ... */ }
    else if (msg == "Nothing") { /* ... */ }
  }
}
```
## Arduino wiring
LCD PinPin NameConnect ToDescription1VSSGNDGround2VDD5VPower supply3V0Potentiometer (Wiper)Contrast adjustment (connect ends to 5V and GND, wiper to V0)4RSArduino Digital Pin 12Register Select5R/WGNDRead/Write (tied to GND for Write mode)6EArduino Digital Pin 11Enable7–10D0–D3Not ConnectedUnused in 4-bit mode11D4Arduino Digital Pin 5Data Bit 412D5Arduino Digital Pin 4Data Bit 513D6Arduino Digital Pin 3Data Bit 614D7Arduino Digital Pin 2Data Bit 715A (LED+)5V via 220Ω resistorBacklight Anode16K (LED-)GNDBacklight Cathode

Opening the serial port resets most Arduino boards, so the Python script waits
two seconds after connecting before sending anything.

---

## Troubleshooting

| Symptom | Cause and fix |
|---------|---------------|
| `Could not open the camera.` | Another app is using the webcam, or permissions are missing. Try `VideoCapture(1)`. On macOS: **System Settings → Privacy & Security → Camera** and allow your terminal or VS Code. |
| `Could not open COM8: ...` | Wrong port, or something else holds it. **Close the Arduino IDE Serial Monitor** — it and Python cannot share the port. Re-check **Tools → Port**. |
| Model fails to load under Keras 3 | `TF_USE_LEGACY_KERAS` must be set *before* TensorFlow is imported, and `tf-keras` must be installed. Don't move those lines. |
| `mutex lock failed: Invalid argument` on macOS | The `sys.modules["pyarrow"] = None` line handles this; make sure it stays above the imports. |
| Esc / q does not quit | The webcam window must be focused. Click it first, or use `Ctrl+C`. |
| Predictions flicker between classes | Raise `CONFIDENCE_THRESHOLD`, improve lighting, or retrain with more varied samples. |
| `IndexError` while reading labels | Every line in `labels.txt` must be `<index> <name>` — an index with no name breaks the split. |

---

## Known limitations

- **Single camera, hardcoded index.** No device selection at runtime.
- **Three fixed classes.** Retraining in Teachable Machine means replacing both
  `keras_model.h5` and `labels.txt`, and updating `MESSAGE_FOR_LABEL`.
- **One-way communication.** Python writes to the board; nothing reads back from it.
- **No dependency lockfile.** Versions are listed in this README, not pinned in a
  `requirements.txt`.
- **Frame rate is bound by inference.** Every frame runs a full forward pass, with
  no skipping or threading.

---

## Credits

- Model trained with [Google Teachable Machine](https://teachablemachine.withgoogle.com/)
- Image preprocessing follows Teachable Machine's exported Keras sample code
- Webcam capture via [OpenCV](https://opencv.org/) · serial I/O via [pySerial](https://pyserial.readthedocs.io/)

Coursework for IT254.
