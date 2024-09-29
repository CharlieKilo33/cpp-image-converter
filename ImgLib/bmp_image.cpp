#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
  uint16_t file_type{0x4D42};
  uint32_t file_size{0};
  uint16_t reserved1{0};
  uint16_t reserved2{0};
  uint32_t offset_data{0};
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
  uint32_t size{40};
  int32_t width{0};
  int32_t height{0};
  uint16_t planes{1};
  uint16_t bit_count{24};
  uint32_t compression{0};
  uint32_t size_image{0};
  int32_t x_pixels_per_meter{11811};
  int32_t y_pixels_per_meter{11811};
  uint32_t colors_used{0};
  uint32_t colors_important{0x1000000};
}
PACKED_STRUCT_END

const int BYTES_PER_PIXEL = 3;
const int ALIGNMENT = 4;

static int GetBMPStride(int w) {
  return ALIGNMENT * ((w * BYTES_PER_PIXEL + ALIGNMENT - 1) / ALIGNMENT);
}

// напишите эту функцию
bool SaveBMP(const Path &file, const Image &image) {
  std::ofstream output(file, std::ios::binary);
  if (!output) {
    return false;
  }

  BitmapFileHeader file_header;
  BitmapInfoHeader info_header;

  int padding = GetBMPStride(image.GetWidth()) - image.GetWidth() * 3;

  file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) +
                          (image.GetWidth() * 3 + padding) * image.GetHeight();
  file_header.offset_data = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

  info_header.width = image.GetWidth();
  info_header.height = image.GetHeight();
  info_header.size_image = (image.GetWidth() * 3 + padding) * image.GetHeight();

  if (!output.write(reinterpret_cast<const char *>(&file_header),
                    sizeof(file_header))) {
    return false;
  }

  if (!output.write(reinterpret_cast<const char *>(&info_header),
                    sizeof(info_header))) {
    return false;
  }

  std::vector<uint8_t> buffer(image.GetWidth() * 3);

  for (int y = image.GetHeight() - 1; y >= 0; --y) {
    const Color *line = image.GetLine(y);
    for (int x = 0; x < image.GetWidth(); ++x) {
      buffer[x * 3] = static_cast<uint8_t>(line[x].b);
      buffer[x * 3 + 1] = static_cast<uint8_t>(line[x].g);
      buffer[x * 3 + 2] = static_cast<uint8_t>(line[x].r);
    }
    output.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());

    if (padding > 0) {
      std::vector<uint8_t> padding_buf(padding, 0);
      if (!output.write(reinterpret_cast<const char *>(padding_buf.data()),
                        padding_buf.size())) {
        return false;
      }
    }
  }
  return true;
}

// напишите эту функцию
Image LoadBMP(const Path &file) {
  std::ifstream input(file.string(), std::ios::binary);
  if (!input.is_open()) {
    return {};
  }

  BitmapFileHeader file_header;
  BitmapInfoHeader info_header;

  input.read(reinterpret_cast<char *>(&file_header), sizeof(file_header));
  if (!input) {
    return {};
  }

  input.read(reinterpret_cast<char *>(&info_header), sizeof(info_header));
  if (!input) {
    return {};
  }

  if (file_header.file_type != 0x4D42 || info_header.bit_count != 24 ||
      info_header.compression != 0) {
    return {};
  }

  Image image(info_header.width, info_header.height, Color::Black());

  int padding = (ALIGNMENT - (info_header.width * 3) % ALIGNMENT) % ALIGNMENT;

  std::vector<uint8_t> buffer((info_header.width * 3) + padding);
  for (int y = info_header.height - 1; y >= 0; --y) {
    if (!input.read(reinterpret_cast<char *>(buffer.data()), buffer.size())) {
      return {};
    }
    Color *line = image.GetLine(y);
    for (int x = 0; x < info_header.width; ++x) {
      uint8_t blue = buffer[x * 3];
      uint8_t green = buffer[x * 3 + 1];
      uint8_t red = buffer[x * 3 + 2];

      line[x] = img_lib::Color{byte{red}, byte{green}, byte{blue}, byte{255}};
    }
  }

  return image;
}

} // namespace img_lib