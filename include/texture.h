#pragma once

#include "common.h"
#include "image.h"

struct texture_t {
    GLuint textureId;
    std::string path;

    texture_t(std::string path) :path(path),textureId(gluninitialized) { }

    texture_t() :path(),textureId(gluninitialized) { }

    bool isLoaded() {
        return textureId != gluninitialized;
    }

    void use(int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    void use() {
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    bool generate(glm::vec<4, float> rgba) {
        int width = 4, height = 4, channels = 4;
        int pixels = width * height;
        int size = pixels * channels;
        unsigned char* image = new unsigned char[size];

        glm::vec<4, unsigned char> vu = rgba * glm::vec4(255.0f);

        if (!image) {
            printf("Error generating image <%i,%i,%i,(%i),(%i),<%i,%i,%i,%i>>\n", width, height, channels, pixels, size, vu.r,vu.b,vu.g,vu.a);
            return glfail;
        }

        for (int i = 0; i < pixels; i++) {
            image[i * channels + 0] = vu.r;
            image[i * channels + 1] = vu.g;
            image[i * channels + 2] = vu.b;
            image[i * channels + 3] = vu.a;
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return glsuccess;
    }

    bool load() {
        int width, height, channels;
        unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!image) {
            std::cout << "Error opening image file: " << path << std::endl;
            return glfail;
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(image);

        return glsuccess;
    }
};
