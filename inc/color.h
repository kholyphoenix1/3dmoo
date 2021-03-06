#ifndef COLOR_H
#define COLOR_H

typedef enum{
    RGBA8 = 0,
    RGB8 = 1,
    RGBA5551 = 2,
    RGB565 = 3,
    RGBA4 = 4,
    IA8 = 5,
    HILO8 = 6,
    I8 = 7,
    A8 = 8,
    IA4 = 9,
    I4 = 10,
    A4 = 11,
    ETC1 = 12,
    ETC1A4 = 13,
    // TODO: Support for the other formats is not implemented, yet.
    // Seems like they are luminance formats and compressed textures.
} TextureFormat;

//Stored as RGBA8
struct Color{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

typedef struct Color Color;

//Decode one pixel into RGBA8 color format
void color_decode(unsigned char* input, const TextureFormat format, Color *output);

//Encode one pixel from RGBA8 color format
void color_encode(Color *input, const TextureFormat format, unsigned char* output);

#endif