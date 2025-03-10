# Installation and Usage Guide

## 1. Install `libfacedetection`
To install `libfacedetection`, run the provided script:

```bash
./install_libfacedetection.sh
```

This script will:
- Clone the repository from [libfacedetection](https://github.com/ShiqiYu/libfacedetection)
- Build the project using CMake
- Install necessary components
- Set up the environment variable `facedetection_DIR` in `.env`

## 2. Build the Project
Once the installation is complete, build the project with:

```bash
cmake --build build --config Release --target all --
```

## 3. Environment Variables
Ensure that the `.env` file is updated with the correct `facedetection_DIR`:

```bash
facedetection_DIR=/path/to/libfacedetection/build
```

## 4. Run the Application
To run the application using `gst-launch-1.0`, use the following command:

```bash
gst-launch-1.0 v4l2src ! videoconvert !  face_sticker silent=TRUE eye_img_path="./emoji.png" eye_img_scale=0.3 ! videoconvert ! xvimagesink sync=false
```

### Parameters:
- `eye_img_path` → Path to the mask image (e.g., `./emoji.png`)
- `eye_img_scale` → Scale factor for the mask overlay


## 5. Troubleshooting
- If `facedetection` is not found, check if `facedetection_DIR` is correctly set in `.env`.
- Ensure that CMake is looking in the correct directory using:

  ```bash
  echo $CMAKE_PREFIX_PATH
  ```
- If issues persist, manually set `CMAKE_PREFIX_PATH` before running CMake:
  
  ```bash
  export CMAKE_PREFIX_PATH=/path/to/libfacedetection/build:$CMAKE_PREFIX_PATH
  ```

