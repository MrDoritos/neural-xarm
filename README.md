# Advanced xArm Control Project

![](/assets/xarm.gif)

Project for controlling an xArm robotic arm. Eventually will also include other robotic arms.

Using a neural network for control was an initial idea, however when you consider you need a working movement model to generate the data to train a neural network I put the cart before the horse. I still think it would be a fun idea to pursue.

You can compile the C++ version or use the python version. I had to port the algorithm to C++/OpenGL to better support a more embedded device.

Both programs intend on solving the same idea which is display a virtual model of the robotic arm.

The OpenGL version made it possible to use proper 4x4 identity matrices which introduced some differences between how the C++ and python versions perform calculations. I prefer the identity matrices. This makes glm a better library for this application as opposed to numpy.

## Using

### Step 1. Download

```
git clone https://github.com/MrDoritos/neural-xarm
cd neural-xarm
```

### Step 2. Compile & Run

#### Python

```
# Install requirements
sudo python3 -m pip install -r ./requirements.txt
# Run program (uses #!/bin/python3)
./main.py
```

#### C++

```
# Install dependencies
sudo apt install libglfw3-dev libopengl-dev libgles2-mesa-dev libegl1-mesa-dev libglm-dev libgl1-mesa-dev mesa-common-dev g++
# Compile
g++ main.cpp -lGL -lglfw -g -o main.out
# Run program
./main.out
```

### Optional for USB

```
sudo apt install libhidapi-hidraw0 libhidapi-libusb0
```

Note, to use the robot over USB, you have to add a udev rule for the unprivileged user. Otherwise run the program with sudo or as root or admin. For more details see https://github.com/libusb/hidapi

## Issues

The kinematic solver is not a generic algorithm, adding different segments will require modifications.

If you have any issues, feel free to submit an issue or contact me.