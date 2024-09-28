#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <bmp_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;

class ImageFormatInterface {
public:
    virtual ~ImageFormatInterface() = default;
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

class JPEGFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

class PPMFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class BMPFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};

enum class Format {
    JPEG,
    PPM,
    BMP,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const std::string ext = input_file.extension().string();
    if (ext == ".jpg"s || ext == ".jpeg"s) {
        return Format::JPEG;
    }
    if (ext == ".ppm"s) {
        return Format::PPM;
    }
    if (ext == ".bmp"s) {
        return Format::BMP;
    }
    return Format::UNKNOWN;
}

ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    Format format = GetFormatByExtension(path);
    switch (format) {
        case Format::JPEG:
            return new JPEGFormat();
        case Format::PPM:
            return new PPMFormat();
        case Format::BMP:
            return new BMPFormat();
        default:
            return nullptr;
    }
}


int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "s << argv[0] << " <in_file> <out_file>"s << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];
    ImageFormatInterface* input_format = GetFormatInterface(in_path);
    if (!input_format) {
        std::cerr << "Unknown format of the input file." << std::endl;
        return 2;
    }

    img_lib::Image image = input_format->LoadImage(in_path);
    delete input_format;

    ImageFormatInterface* output_format = GetFormatInterface(out_path);
    if (!output_format) {
        std::cerr << "Unknown format of the output file." << std::endl;
        return 3;
    }

    bool save_success = output_format->SaveImage(out_path, image);
    delete output_format;

    if (!save_success) {
        cerr << "Saving failed"s << endl;
        return 5;
    }

    cout << "Successfully converted"s << endl;
}