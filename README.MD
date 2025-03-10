# Installation and Usage Guide

## 1. Install Dependencies (GStreamer, OpenCV)

Before proceeding, install the necessary dependencies for GStreamer and OpenCV.

### **Linux (Ubuntu/Debian)**

```bash
sudo apt update && sudo apt install -y \ 
    gstreamer1.0-tools gstreamer1.0-plugins-bad gstreamer1.0-plugins-good \ 
    gstreamer1.0-plugins-ugly gstreamer1.0-libav \ 
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \ 
    libopencv-dev
```

For more details, refer to the official [GStreamer installation guide](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c).

### **MacOS**

```bash
brew install gstreamer gst-plugins-bad gst-plugins-good gst-plugins-ugly gst-libav opencv
```

### **Windows**

Download and install GStreamer from the [official website](https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c).

## 2. Install `libfacedetection`

To install `libfacedetection`, run the provided script:

```bash
./install_libfacedetection.sh
```

This script will:

- Clone the repository from [libfacedetection](https://github.com/ShiqiYu/libfacedetection)
- Build the project using CMake
- Install necessary components
- Set up the environment variable `facedetection_DIR` in `.env`

## 3. Build the Project

Once the installation is complete, build the project with:

```bash
cmake --build build --config Release --target all --
```

## 4. Environment Variables

Ensure that the `.env` file is updated with the correct `facedetection_DIR`:

```bash
facedetection_DIR=/path/to/libfacedetection/build
```

## 5. Run the Application

To run the application pipeline using `gst-launch-1.0`, use the following command:

```bash
gst-launch-1.0 v4l2src ! videoconvert !  face_sticker silent=TRUE eye_img_path="./emoji.png" eye_img_scale=0.3 ! videoconvert ! xvimagesink sync=false
```

### Parameters:

- `eye_img_path` → Path to the mask image (e.g., `./emoji.png`)
- `eye_img_scale` → Scale factor for the mask overlay


## 6. Troubleshooting

- If `facedetection` is not found, check if `facedetection_DIR` is correctly set in `.env`.
- Ensure that CMake is looking in the correct directory using:
  ```bash
  echo $CMAKE_PREFIX_PATH
  ```
- If issues persist, manually set `CMAKE_PREFIX_PATH` before running CMake:
  ```bash
  export CMAKE_PREFIX_PATH=/path/to/libfacedetection/build:$CMAKE_PREFIX_PATH
  ```

