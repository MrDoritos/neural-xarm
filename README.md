# Advanced xArm Control Project

Project for controlling an xArm robotic arm. Eventually will also include other robotic arms.

Using a neural network for control was an initial idea, however when you consider you need a working movement model to generate the data to train a neural network I put the cart before the horse. I still think it would be a fun idea to pursue.

You can compile the C++ version or use the python version. I had to port the algorithm to C++/OpenGL to better support a more embedded device.

Both programs intend on solving the same idea which is display a virtual model of the robotic arm.

The OpenGL version made it possible to use proper 4x4 identity matrices which introduced some differences between how the C++ and python versions perform calculations. I prefer the identity matrices. This makes glm a better library for this application as opposed to numpy.

## Issues

The kinematic solver is not a generic algorithm, adding different segments will require modifications.