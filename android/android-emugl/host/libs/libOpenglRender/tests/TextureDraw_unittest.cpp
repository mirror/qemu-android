// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "GLTestUtils.h"
#include "OpenGLTestContext.h"
#include "TextureDraw.h"

namespace emugl {

namespace {

void TestTextureDrawBasic(const GLESv2Dispatch* gl, GLenum internalformat,
                          GLenum format) {
    GLint viewport[4] = {};
    gl->glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(0, viewport[0]);
    EXPECT_EQ(0, viewport[1]);
    const int width = viewport[2];
    const int height = viewport[3];
    const GLenum type = GL_UNSIGNED_BYTE;
    const int bpp = 4;
    const int bytes = width * height * bpp;

    GLuint textureToDraw;

    gl->glGenTextures(1, &textureToDraw);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, textureToDraw);

    gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<unsigned char> pixels(bytes);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixels[i * width * bpp + j * bpp + 0] = (0xaa + i) % 0x100;
            pixels[i * width * bpp + j * bpp + 1] = (0x00 + j) % 0x100;
            pixels[i * width * bpp + j * bpp + 2] = (0x11 + i) % 0x100;
            pixels[i * width * bpp + j * bpp + 3] = (0xff + j) % 0x100;
        }
    }

    gl->glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0,
                     format, type, pixels.data());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLint fbStatus = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    EXPECT_EQ((GLint)GL_FRAMEBUFFER_COMPLETE, fbStatus);

    TextureDraw textureDraw;

    textureDraw.draw(textureToDraw, 0, 0, 0);

    std::vector<unsigned char> pixelsOut(bytes, 0xff);

    gl->glReadPixels(0, 0, width, height, format, type, pixelsOut.data());

    // Check that the texture is drawn upside down (because that's what SurfaceFlinger wants)
    for (int i = 0; i < height; i++) {
        size_t rowBytes = width * bpp;
        EXPECT_TRUE(RowMatches(i, width * bpp,
                               pixels.data() + i * rowBytes,
                               pixelsOut.data() + (height - i - 1) * rowBytes));
    }
}

}  // namespace

#define GL_BGRA_EXT 0x80E1

TEST_F(GLTest, TextureDrawBasic) {
     const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
     TestTextureDrawBasic(gl, GL_RGBA, GL_RGBA);
     const char* ext = (const char *)gl->glGetString(GL_EXTENSIONS);
     if (strstr(ext, "GL_EXT_texture_format_BGRA8888")) {
         fprintf(stderr, "Testing BGRA8888 format texture\n");
         TestTextureDrawBasic(gl, GL_BGRA_EXT, GL_BGRA_EXT);
     }
}

}  // namespace emugl
