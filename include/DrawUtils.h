#pragma once
#include <TFT_eSPI.h>

#define P_HORIZONTAL 0
#define P_VERTICAL 1

// Fast alphaBlend
template <typename A, typename F, typename B> static inline uint16_t
fastBlend(A alpha, F fgc, B bgc)
{
  // Split out and blend 5-bit red and blue channels
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
  // Split out and blend 6-bit green channel
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
  // Recombine channels
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}

void DrawProgressBar(TFT_eSprite& targetSprite, uint8_t type, int x, int y, int w, int h, 
int blockSize, int blockSpacing, int cornerRadius, int percent, uint16_t colorStart, uint16_t colorEnd)
{
    if(type == P_HORIZONTAL)
    {
        int end = ((w * percent)/100) + x;
        for(int blockStart = x; blockStart < end; blockStart += blockSize + blockSpacing)
        {
        int width = min(end - blockStart, blockSize);
        uint16_t color = fastBlend(255 * (blockStart - x) / w, colorEnd, colorStart);
        targetSprite.fillSmoothRoundRect(blockStart, y, width, h, cornerRadius, color);
        }
    }
    else
    {
        int end = y - (h * percent)/100;
        for(int blockStart = y - blockSize; blockStart + blockSize >= end; blockStart -= blockSize + blockSpacing)
        {
            int height = min((blockStart + blockSize) - end, blockSize);
            uint16_t color = fastBlend(255 * (y - blockStart) / h, colorEnd, colorStart);
            targetSprite.fillSmoothRoundRect(x, blockStart + blockSize - height, w, height, cornerRadius, color);
        }
    }
}
