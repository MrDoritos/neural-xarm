# Advanced xArm Control Project

![](/assets/xarm.gif)

Project for controlling a robotic arm.

The python version - contained inside `main.py` - is highly out of date. All new work is going towards the C++ version.

## Using

### Step 1. Download

```
git clone https://github.com/MrDoritos/neural-xarm --recursive
cd neural-xarm
```

### Step 2. Compile & Run

#### Python (the original project file)

```
# Install requirements
sudo python3 -m pip install -r ./requirements.txt
# Run program (uses #!/bin/python3)
./main.py
```

#### C++

```
# Install dependencies
sudo apt install libglfw3-dev libopengl-dev libgles2-mesa-dev libegl1-mesa-dev libglm-dev libgl1-mesa-dev mesa-common-dev g++ cmake libhidapi-dev
# Compile
mkdir build && cd build
cmake ..
make
# Run program
./neural_xarm
```

### Connecting to the robot with USB

```
sudo apt install libhidapi-hidraw0 libhidapi-libusb0
```

Note, to use the robot over USB, you have to add a udev rule for the unprivileged user. Otherwise run the program with sudo or as root or admin. For more details see https://github.com/libusb/hidapi

### Connecting bluetooth controllers

In `/etc/bluetooth/input.conf`, uncomment or modify the `ClassicBondedOnly` variable to be `ClassicBondedOnly=false`. Setting this to false makes your device vulnerable to HID spoof attacks, but allows PS3 controllers to connect.

Then restart bluetooth daemon

```
sudo systemctl restart bluetooth
```

## Issues

The kinematic solver is not a generic algorithm, adding different segments will require modifications.

If you have any issues, feel free to submit an issue or contact me.

### To-Do

- [x] Robot descriptor format (simple version)
- [x] Multiple robots can now be rendered and transformed
- [ ] Expand descriptor
- [ ] Solve physics problem and start drafting the iterative solver
- [ ] Conceptualize new inverse kinematics solver
- [ ] Test control over bluetooth HID (need the robot again)

## Credits

- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
- [stl_reader](https://github.com/sreiter/stl_reader)
- [stb](https://github.com/nothings/stb)
- [glfw](https://github.com/glfw/glfw)
- [hidapi](https://github.com/libusb/hidapi)
- [glm](https://github.com/g-truc/glm)
- [Khronos Group](https://www.khronos.org/)


