/*
    FrameRenderer.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "FrameRenderer.h"

#include <ST/CameraFrames.h>

#include <algorithm>
#include <stdexcept>

#if __ANDROID__
#    include <android/log.h>
#    include <GLES3/gl3.h>
#    define SHADER_VERSION_HEADER "#version 300 es\nprecision mediump float;"
#else
#    include <GL/glew.h>
#    define SHADER_VERSION_HEADER "#version 330"
#endif

using SampleProcessing::FrameRenderer;

static const char *const commonVertexShader = SHADER_VERSION_HEADER "\n" R"(
in vec2 aVertex;
in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
   gl_Position = vec4(aVertex, 0.0, 1.0);
   vTexCoord = aTexCoord;
}
)";

static const char *const depthFragmentShader = SHADER_VERSION_HEADER "\n" R"(
in vec2 vTexCoord;
uniform sampler2D sampler;
uniform float minDepth;
uniform float maxDepth;
layout(location = 0) out vec4 fragColor;
void main() {
    float mm = texture(sampler, vTexCoord).x;
    if (isnan(mm)) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else {
        float scaled = max(0.0, 6.0 * (mm - minDepth) / (maxDepth - minDepth));
        int cat = int(floor(scaled));
        float lb = scaled - floor(scaled);
        if (cat == 0) {
            fragColor = vec4(1.0, 1.0 - lb, 1.0 - lb, 1.0);
        }
        else if (cat == 1) {
            fragColor = vec4(1.0, lb, 0.0, 1.0);
        }
        else if (cat == 2) {
            fragColor = vec4(1.0 - lb, 1.0, 0.0, 1.0);
        }
        else if (cat == 3) {
            fragColor = vec4(0.0, 1.0, lb, 1.0);
        }
        else if (cat == 4) {
            fragColor = vec4(0.0, 1.0 - lb, 1.0, 1.0);
        }
        else if (cat == 5) {
            fragColor = vec4(0.0, 0.0, 1.0 - lb, 1.0);
        }
        else {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}
)";

static const char *const visibleFragmentShader = SHADER_VERSION_HEADER "\n" R"(
in vec2 vTexCoord;
uniform sampler2D sampler;
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(texture(sampler, vTexCoord).rgb, 1.0);
}
)";

static const char *const infraredFragmentShader = SHADER_VERSION_HEADER "\n" R"(
in vec2 vTexCoord;
uniform usampler2D sampler;
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(vec3(texture(sampler, vTexCoord).xxx) / 1023.0, 1.0);
}
)";

static constexpr GLuint vertexAttributeID = 0;
static constexpr GLuint texCoordAttributeID = 1;

static void reportShaderErrorOnFailureToCompile(GLuint shader)
{
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    char* errorLog = new char[maxLength];
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

#if __ANDROID__ || ANDROID
    __android_log_print(ANDROID_LOG_INFO, "StructureSDK", "Shader error: %s", errorLog);
#else
    fputs(errorLog, stderr);
#endif

    delete[] errorLog;

    glDeleteShader(shader);
}

static GLuint loadShader(const char *fragmentShaderSrc) {
    GLint status = 0;

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &commonVertexShader, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        reportShaderErrorOnFailureToCompile(vertexShader);
        throw std::runtime_error("Failed to load vertex shader");
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        reportShaderErrorOnFailureToCompile(fragmentShader);
        throw std::runtime_error("Failed to load fragment shader");
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glBindAttribLocation(prog, vertexAttributeID, "aVertex");
    glBindAttribLocation(prog, texCoordAttributeID, "aTexCoord");
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        throw std::runtime_error("Failed to link shader program");
    }

    glDetachShader(prog, vertexShader);
    glDetachShader(prog, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return prog;
}

FrameRenderer::FrameRenderer() {
    glGenFramebuffers(1, &_framebuffer);
    _depthProg = loadShader(depthFragmentShader);
    _genDepthProg = loadShader(depthFragmentShader);
    _visibleProg = loadShader(visibleFragmentShader);
    _infraredProg = loadShader(infraredFragmentShader);
    glGenVertexArrays(1, &_renderVAO);
    glGenBuffers(1, &_vertexVBO);
    glGenBuffers(1, &_texCoordVBO);
    auto textures = {
        &_depthInput,
        &_genDepthInput,
        &_visibleInput,
        &_infraredInput,
        &_depthOutput,
        &_genDepthOutput,
        &_visibleOutput,
        &_infraredOutput,
    };
    for (uint32_t *tex : textures) {
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

FrameRenderer::~FrameRenderer() {
    glDeleteFramebuffers(1, &_framebuffer);
    glDeleteProgram(_depthProg);
    glDeleteProgram(_genDepthProg);
    glDeleteProgram(_visibleProg);
    glDeleteProgram(_infraredProg);
    glDeleteVertexArrays(1, &_renderVAO);
    glDeleteBuffers(1, &_vertexVBO);
    glDeleteBuffers(1, &_texCoordVBO);
    auto textures = {
        &_depthInput,
        &_genDepthInput,
        &_visibleInput,
        &_infraredInput,
        &_depthOutput,
        &_genDepthOutput,
        &_visibleOutput,
        &_infraredOutput,
    };
    for (uint32_t *tex : textures) {
        glDeleteTextures(1, tex);
    }
}

void FrameRenderer::render(uint32_t inputTexture, uint32_t outputTexture, unsigned width, unsigned height, uint32_t prog) {
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    static const GLfloat vertices[] = {
        -1.f, -1.f,
        -1.f, +1.f,
        +1.f, -1.f,
        +1.f, +1.f,
    };
    static const GLfloat texCoords[] = {
        0.f, 0.f,
        0.f, 1.f,
        1.f, 0.f,
        1.f, 1.f,
    };

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Framebuffer not valid!");
    }
    glUseProgram(prog);
    glBindVertexArray(_renderVAO);
    glEnableVertexAttribArray(vertexAttributeID);
    glEnableVertexAttribArray(texCoordAttributeID);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(vertexAttributeID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof *vertices, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, _texCoordVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof texCoords, texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(texCoordAttributeID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof *texCoords, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(prog, "sampler"), 0);

    if (prog == _depthProg || prog == _genDepthProg) {
        glUniform1f(glGetUniformLocation(prog, "minDepth"), _minDepthBasedOnRangeInMm);
        glUniform1f(glGetUniformLocation(prog, "maxDepth"), _maxDepthBasedOnRangeInMm);
    }

    static const GLenum drawbuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawbuf);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(vertexAttributeID);
    glDisableVertexAttribArray(texCoordAttributeID);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameRenderer::renderDepthFrame(const ST::DepthFrame& frame)
{
    glBindTexture(GL_TEXTURE_2D, _depthInput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, frame.width(), frame.height(), 0, GL_RED, GL_FLOAT, frame.depthInMillimeters());
    glBindTexture(GL_TEXTURE_2D, 0);
    render(_depthInput, _depthOutput, frame.width(), frame.height(), _depthProg);
}

void FrameRenderer::renderGenDepthFrame(const ST::DepthFrame& frame)
{
    glBindTexture(GL_TEXTURE_2D, _genDepthInput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, frame.width(), frame.height(), 0, GL_RED, GL_FLOAT, frame.depthInMillimeters());
    glBindTexture(GL_TEXTURE_2D, 0);
    render(_genDepthInput, _genDepthOutput, frame.width(), frame.height(), _genDepthProg);
}

void FrameRenderer::renderVisibleFrame(const ST::ColorFrame& frame)
{
    glBindTexture(GL_TEXTURE_2D, _visibleInput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, frame.width(), frame.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, frame.rgbData());
    glBindTexture(GL_TEXTURE_2D, 0);
    render(_visibleInput, _visibleOutput, frame.width(), frame.height(), _visibleProg);
}

void FrameRenderer::renderInfraredFrame(const ST::InfraredFrame& frame)
{
    glBindTexture(GL_TEXTURE_2D, _infraredInput);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, frame.width(), frame.height(), 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, frame.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    render(_infraredInput, _infraredOutput, frame.width(), frame.height(), _infraredProg);
}

void FrameRenderer::setMinMaxDepthInMm(float min, float max)
{
    _minDepthBasedOnRangeInMm = min;
    _maxDepthBasedOnRangeInMm = max;
}
