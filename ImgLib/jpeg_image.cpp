#include "ppm_image.h"

#include <array>
#include <fstream>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// В эту функцию вставлен код примера из библиотеки libjpeg.
// Измените его, чтобы адаптировать к переменным file и image.
// Задание качества уберите - будет использовано качество по умолчанию
bool SaveJPEG(const Path& file, const Image& image) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    FILE* outfile;
    JSAMPROW row_pointer[1];  // Массив указателей на строки
    int row_stride;           // Ширина строки в байтах (для RGB)

    // Инициализация обработчика ошибок
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // Открываем файл для записи
#ifdef _MSC_VER
    if ((outfile = _wfopen(file.wstring().c_str(), L"wb")) == nullptr) {
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == nullptr) {
#endif
        return false;  // Ошибка открытия файла
    }

    // Устанавливаем файл как целевое назначение
    jpeg_stdio_dest(&cinfo, outfile);

    // Устанавливаем параметры изображения
    cinfo.image_width = image.GetWidth();     // Ширина изображения
    cinfo.image_height = image.GetHeight();   // Высота изображения
    cinfo.input_components = 3;               // 3 компонента на пиксель (R, G, B)
    cinfo.in_color_space = JCS_RGB;           // Цветовое пространство

    // Устанавливаем параметры сжатия по умолчанию
    jpeg_set_defaults(&cinfo);

    // Начинаем сжатие
    jpeg_start_compress(&cinfo, TRUE);

    // Определяем ширину строки в байтах (для RGB)
    row_stride = image.GetWidth() * 3;

    // Временный буфер для строки изображения
    std::vector<JSAMPLE> buffer(row_stride);

    // Цикл по строкам изображения
    while (cinfo.next_scanline < cinfo.image_height) {
        // Получаем указатель на строку пикселей
        const Color* line = image.GetLine(cinfo.next_scanline);

        // Заполняем временный буфер, конвертируя пиксели из byte в JSAMPLE (RGB)
        for (int x = 0; x < image.GetWidth(); ++x) {
            const Color& pixel = line[x];
            buffer[x * 3]     = static_cast<JSAMPLE>(pixel.r);  // Красный
            buffer[x * 3 + 1] = static_cast<JSAMPLE>(pixel.g);  // Зеленый
            buffer[x * 3 + 2] = static_cast<JSAMPLE>(pixel.b);  // Синий
        }

        // Устанавливаем указатель на строку буфера
        row_pointer[0] = buffer.data();

        // Записываем строку в JPEG-файл
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // Завершаем сжатие
    jpeg_finish_compress(&cinfo);

    // Закрываем файл
    fclose(outfile);

    // Освобождаем ресурсы
    jpeg_destroy_compress(&cinfo);

    return true;  // Успешное завершение
}




// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveSсanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib