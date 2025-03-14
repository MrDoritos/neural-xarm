# Advanced xArm Control Project

![](/assets/xarm.gif)

Project for controlling an xArm robotic arm. Eventually will also include other robotic arms.

Using a neural network for control was an initial idea, however when you consider you need a working movement model to generate the data to train a neural network I put the cart before the horse. I still think it would be a fun idea to pursue.

You can compile the C++ version or use the python version. I had to port the algorithm to C++/OpenGL to better support a slower device than my development computer. The C++ version will be my primary focus.

Both programs intend on solving the same idea which is display a virtual model of the robotic arm.

The OpenGL version made it possible to use proper 4x4 identity matrices which introduced some differences between how the C++ and python versions perform calculations. I prefer the identity matrices. This makes glm a better library for this application as opposed to numpy.

## Using

### Step 1. Download

```
git clone https://github.com/MrDoritos/neural-xarm --recursive
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
sudo apt install libglfw3-dev libopengl-dev libgles2-mesa-dev libegl1-mesa-dev libglm-dev libgl1-mesa-dev mesa-common-dev g++ cmake libhidapi-dev
# Compile
mkdir build && cd build
cmake ..
make
# Run program
./neural_xarm
```

Note, if `<format>` is not found, you may not have g++13. This was the case on my Debian 12/bookworm laptop. To fix this, the best way other than adding an external repo or 3rd party .deb file is to compile gcc/g++ from source see https://stackoverflow.com/questions/78306968/installing-gcc13-and-g13-in-debian-bookworm-rust-docker-image for the example commands to achieve this. You may need to compile glfw3 3.4 for joystick support.

Since I have to build with a different compiler I added a flag `COMPAT` to switch the lib and bin path for compilation. You may also set `COMPAT_PATH`, but currently only searches for `g++-13` at this time.

```
cmake .. -DCOMPAT=1 && make
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

- [x] Check origins of model, they are wrong again 
- [x] Add new dependencies as submodules
- [x] Integrate CMake into the build process
- [x] After CMake, separate the header files
- [ ] Maybe add tests to challenge myself
- [x] Add new screenshot/gif video of the colored segments and new shader
- [x] Finish movement rewrite
- [ ] Conceptualize new inverse kinematics solver
- [ ] Test control over bluetooth HID (need the robot again)
- [ ] Order thermostat for car? (I keep forgetting)

## Credits

- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
- [stl_reader](https://github.com/sreiter/stl_reader)
- [stb](https://github.com/nothings/stb)
- [glfw](https://github.com/glfw/glfw)
- [hidapi](https://github.com/libusb/hidapi)
- [glm](https://github.com/g-truc/glm)
- [Khronos Group](https://www.khronos.org/)


