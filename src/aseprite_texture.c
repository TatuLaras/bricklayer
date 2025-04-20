#define CUTE_ASEPRITE_IMPLEMENTATION

#include "aseprite_texture.h"

#include "cute_aseprite.h"
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Image *aseprite_load_as_image(const char *filepath) {
    // Load aseprite file
    ase_t *aseprite_file = cute_aseprite_load_from_file(filepath, 0);
    if (!aseprite_file || aseprite_file->frame_count == 0)
        return 0;

    Image *image = malloc(sizeof(Image));
    memset(image, 0, sizeof(Image));

    image->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    image->mipmaps = 1;
    image->height = aseprite_file->h;
    image->width = aseprite_file->w;

    size_t data_size = image->width * image->height * sizeof(ase_color_t);
    uint32_t *pixel_data = malloc(data_size);
    memcpy(pixel_data, aseprite_file->frames[0].pixels, data_size);
    image->data = pixel_data;

    cute_aseprite_free(aseprite_file);

    return image;
}
