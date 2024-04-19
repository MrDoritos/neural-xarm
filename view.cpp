#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Function to load OBJ file
bool loadOBJ(const std::string& filename, std::vector<GLfloat>& vertices, std::vector<GLfloat>& normals, std::vector<GLfloat>& texcoords) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") {
            GLfloat x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        } else if (token == "vn") {
            GLfloat nx, ny, nz;
            iss >> nx >> ny >> nz;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        } else if (token == "vt") {
            GLfloat u, v;
            iss >> u >> v;
            texcoords.push_back(u);
            texcoords.push_back(v);
        }
        // TODO: Parse other OBJ data and populate the vertices, normals, and texcoords vectors accordingly
    }

    file.close();
    return true;
}
// Function to load MTL file
bool loadMTL(const std::string& filename, std::vector<GLfloat>& materials) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open MTL file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "newmtl") {
            GLfloat materialName;
            iss >> materialName;
            materials.push_back(materialName);
        }
        // TODO: Parse other MTL data and populate the materials vector accordingly
    }

    file.close();
    return true;
}

// Function to initialize GLFW and create a window
GLFWwindow* initGLFW() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    // Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL GLFW Program", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    return window;
}

// Function to render the loaded OBJ model
void renderModel(const std::vector<GLfloat>& vertices, const std::vector<GLfloat>& normals, const std::vector<GLfloat>& texcoords, const std::vector<GLfloat>& materials) {
    // TODO: Implement rendering logic here
    // Use the vertices, normals, texcoords, and materials to render the model using OpenGL

    // Example rendering logic:
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < vertices.size(); i += 3) {
        // Set material properties
        GLfloat material = materials[i / 3];
        glColor3f(material, material, material);

        // Render vertex
        glVertex3f(vertices[i], vertices[i + 1], vertices[i + 2]);
    }
    glEnd();
}

int main() {
    // Initialize GLFW and create a window
    GLFWwindow* window = initGLFW();
    if (!window) {
        return -1;
    }

    // Load OBJ file
    std::vector<GLfloat> vertices, normals, texcoords;
    if (!loadOBJ("model.obj", vertices, normals, texcoords)) {
        std::cerr << "Failed to load OBJ file" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Load MTL file
    std::vector<GLfloat> materials;
    if (!loadMTL("model.mtl", materials)) {
        std::cerr << "Failed to load MTL file" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the model
        renderModel(vertices, normals, texcoords);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    return 0;
}