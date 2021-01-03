#pragma once

#include <functional>

struct ID3D11Texture2D;

using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
using GPUFrameCb = std::function<void(ID3D11Texture2D* frame)>;