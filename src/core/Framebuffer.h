#pragma once
// =============================================================================
//  Framebuffer.h — FBO OpenGL pour le rendu de la vue 3D dans un onglet ImGui
// =============================================================================

#include <GL/glew.h>
#include <iostream>
#include <utility>

class Framebuffer {
public:
    GLuint fbo     = 0;
    GLuint texture = 0;
    GLuint rbo     = 0;
    int    width   = 0;
    int    height  = 0;

    Framebuffer() = default;

    ~Framebuffer() {
        cleanup();
    }

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept
        : fbo(other.fbo), texture(other.texture), rbo(other.rbo),
          width(other.width), height(other.height) {
        other.fbo = 0;
        other.texture = 0;
        other.rbo = 0;
        other.width = 0;
        other.height = 0;
    }

    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            cleanup();
            fbo = other.fbo;
            texture = other.texture;
            rbo = other.rbo;
            width = other.width;
            height = other.height;

            other.fbo = 0;
            other.texture = 0;
            other.rbo = 0;
            other.width = 0;
            other.height = 0;
        }
        return *this;
    }

    void init(int w, int h) {
        if (w <= 0 || h <= 0) return;
        cleanup();

        width = w;
        height = h;

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Texture couleur RGBA8
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        // Renderbuffer Depth / Stencil
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[ERREUR] Le Framebuffer OpenGL n'est pas complet !\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void resize(int w, int h) {
        if (w <= 0 || h <= 0) return;
        if (w == width && h == height && fbo != 0) return;
        init(w, h);
    }

    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void cleanup() {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        if (rbo != 0) {
            glDeleteRenderbuffers(1, &rbo);
            rbo = 0;
        }
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
    }
};
