/*
    FrameRenderer.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <stdint.h>

namespace ST
{
    struct DepthFrame;
    struct ColorFrame;
    struct InfraredFrame;
}

namespace SampleProcessing
{
    class FrameRenderer
    {
    private:
        uint32_t _framebuffer = 0;
        uint32_t _depthProg = 0;
        uint32_t _genDepthProg = 0;
        uint32_t _visibleProg = 0;
        uint32_t _infraredProg = 0;
        uint32_t _renderVAO = 0;
        uint32_t _vertexVBO = 0;
        uint32_t _texCoordVBO = 0;

        // Textures
        uint32_t _depthInput = 0;
        uint32_t _genDepthInput = 0;
        uint32_t _visibleInput = 0;
        uint32_t _infraredInput = 0;
        uint32_t _depthOutput = 0;
        uint32_t _genDepthOutput = 0;
        uint32_t _visibleOutput = 0;
        uint32_t _infraredOutput = 0;

        // parameters for shaders
        float _minDepthBasedOnRangeInMm = 0.f;
        float _maxDepthBasedOnRangeInMm = 10000.f;

        void render(uint32_t inputTexture, uint32_t outputTexture, unsigned width, unsigned height, uint32_t prog);

    public:
        // GL context required
        FrameRenderer();
        ~FrameRenderer();

        void setMinMaxDepthInMm(float min, float max);

        void renderDepthFrame(const ST::DepthFrame& frame);
        void renderGenDepthFrame(const ST::DepthFrame& frame);
        void renderVisibleFrame(const ST::ColorFrame& frame);
        void renderInfraredFrame(const ST::InfraredFrame& frame);

        // Return GL texture ID for rendered frame or 0 if not yet rendered
        uint32_t depthTexture()    { return _depthOutput;    }
        uint32_t genDepthTexture() { return _genDepthOutput; }
        uint32_t visibleTexture()  { return _visibleOutput;  }
        uint32_t infraredTexture() { return _infraredOutput; }

        FrameRenderer(const FrameRenderer&) = delete;
        FrameRenderer(FrameRenderer&&) = delete;
        FrameRenderer& operator=(const FrameRenderer&) = delete;
        FrameRenderer& operator=(FrameRenderer&&) = delete;
    };
}
