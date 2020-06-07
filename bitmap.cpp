#include "bitmap.h"
/**
     * Read in an image.
     * reads a bitmap in from the stream
     *
     * @param in the stream to read from.
     * @param b the bitmap that we are creating.
     *
     * @return the stream after we've read in the image.
     *
     * @throws BitmapException if it's an invalid bitmap.
     * @throws bad_alloc exception if we failed to allocate memory.
     */
std::istream &operator>>(std::istream &in, Bitmap &b)
{
    try
    {
        if (in)
        {
            in.read((char *)&b.file_header, sizeof(b.file_header));
            if (b.file_header.file_type != 0x4D42)
            {
                throw std::runtime_error("Error! Not a correct file format");
            }
            in.read((char *)&b.bmp_info_header, sizeof(b.bmp_info_header));
            b.imageType = b.bmp_info_header.bit_count / 8;

            // The BMPColorHeader is used only for transparent images
            if (b.bmp_info_header.bit_count == 32)
            {
                // Check if the file has bit mask color information
                if (b.bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader)))
                {
                    in.read((char *)&b.bmp_color_header, sizeof(b.bmp_color_header));
                }
                else
                {
                    throw(BitmapException("Error! The file does not contain bit mask information\n", 74));
                }
            }

            // Jump to the pixel data location
            in.seekg(b.file_header.offset_data, in.beg);

            // Adjust the header fields for output.
            if (b.bmp_info_header.bit_count == 32)
            {
                b.bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                b.file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            }
            else
            {
                b.bmp_info_header.size = sizeof(BMPInfoHeader);
                b.file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            b.file_header.file_size = b.file_header.offset_data;
            b.data.resize(b.bmp_info_header.width * 3 * 3 * b.bmp_info_header.height * b.bmp_info_header.bit_count / 8);

            // Here we check if we need to take into account row padding
            if (b.bmp_info_header.width % 4 == 0)
            {
                in.read((char *)b.data.data(), b.data.size());
                b.file_header.file_size += static_cast<uint32_t>(b.data.size());
            }
            else
            {
                b.row_stride = b.bmp_info_header.width * b.bmp_info_header.bit_count / 8;
                uint32_t new_stride = b.make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - b.row_stride);

                for (int y = 0; y < b.bmp_info_header.height; ++y)
                {
                    in.read((char *)(b.data.data() + b.row_stride * y), b.row_stride);
                    in.read((char *)padding_row.data(), padding_row.size());
                }
                b.file_header.file_size += static_cast<uint32_t>(b.data.size()) + b.bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
            }
        }
        else
        {
            throw std::runtime_error("Unable to open the input image file.");
        }

        return in;
    }
    catch (BitmapException ex)
    {
        ex.print_exception();
    }
}

/**
     * Write the binary representation of the image to the stream.
     *
     * @param out the stream to write to.
     * @param b the bitmap that we are writing.
     *
     * @return the stream after we've finished writting.
     *
     * @throws failure if we failed to write.
     */
std::ostream &operator<<(std::ostream &out, Bitmap &b)
{
    if (out)
    {
        if (b.bmp_info_header.bit_count == 32)
        {
            b.write_headers_and_data(out);
        }
        else if (b.bmp_info_header.bit_count == 24)
        {
            if (b.bmp_info_header.width % 4 == 0)
            {
                b.write_headers_and_data(out);
            }
            else
            {
                uint32_t new_stride = b.make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - b.row_stride);

                b.write_headers(out);

                for (int y = 0; y < b.bmp_info_header.height; ++y)
                {
                    uint8_t *row = (&b.data[0] + b.row_stride * y);
                    out.write((const char *)(row), b.row_stride);
                    out.write((const char *)padding_row.data(), padding_row.size());
                }
            }
        }
        else
        {
            throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
        }
    }
    else
    {
        throw std::runtime_error("Unable to open the output image file.");
    }
}

BitmapException::BitmapException(const std::string &message, uint32_t position) : _message(message), _position(position)
{
}

BitmapException::BitmapException(std::string &&message, uint32_t position) : _message(message), _position(position)
{
}

/**
     * prints out the exception in the form:
     *
     * "Error in bitmap at position 0xposition :
     * message"
     */
void BitmapException::print_exception()
{
    std::cerr << "BitmapException :" << _message << " at memory position " << _position;
}

/** 
     *  To align strid, add 1 to the row_stride until it is divisible with align_stride
     * @param align_stride value to identify when the stride is divisable by 4
    */
uint32_t Bitmap::make_stride_aligned(uint32_t align_stride)
{
    uint32_t new_stride = row_stride;
    while (new_stride % align_stride != 0)
    {
        new_stride++;
    }
    return new_stride;
}

/**
     * Write the binary representation of image header to stream
     * @param out stream to write to.
     * 
    */
void Bitmap::write_headers(std::ostream &out)
{
    out.write((const char *)&file_header, sizeof(file_header));
    out.write((const char *)&bmp_info_header, sizeof(bmp_info_header));
    if (bmp_info_header.bit_count == 32)
    {
        out.write((const char *)&bmp_color_header, sizeof(bmp_color_header));
    }
}

/**
 * Write the binary representation of image header and data to stream
 * @param out stream to write to.
 * 
*/
void Bitmap::write_headers_and_data(std::ostream &out)
{
    write_headers(out);
    out.write((const char *)data.data(), data.size());
}

/**
 * returns the nearest number to the given value.
 * @param inValue pixel for which nearest number to be find.
 *  
*/
uint8_t nearesetNumber(uint8_t inValue)
{
    uint8_t val[3] = {0, 128, 255};
    uint8_t min = 255;
    uint8_t minIndex = 0;

    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t curMin = (inValue > val[i]) ? inValue - val[i] : val[i] - inValue;
        if (curMin < min)
        {
            min = curMin;
            minIndex = i;
        }
    }
    return val[minIndex];
}
/**
 * cell shade an image.
 * for each component of each pixel we round to 
 * the nearest number of 0, 180, 255
 *
 * This has the effect of making the image look like.
 * it was colored.
 */
void cellShade(Bitmap &b)
{
    try
    {
        for (int y = 0; y < b.bmp_info_header.height; y++)
        {
            uint8_t *row = (&b.data[0] + b.row_stride * (y));
            if (!row)
            {
                throw(BitmapException("Image Data is not aviable ", 75));
            }
            for (int x = 0; x < b.bmp_info_header.width; x++)
            {
                uint8_t *pixel = &row[((x)*b.imageType)];
                for (uint8_t ch = 0; ch < b.imageType; ch++)
                {
                    pixel[ch] = nearesetNumber(pixel[ch]);
                }
            }
        }
    }
    catch (BitmapException exception)
    {
        exception.print_exception();
        return;
    }
}

/**
 * Grayscales an image by averaging all of the components.
 */
void grayscale(Bitmap &b)
{
    uint16_t gray = 0;
    for (int y = 0; y < b.bmp_info_header.height; y++)
    {
        uint8_t *row = (&b.data[0] + b.row_stride * (y));
        for (int x = 0; x < b.bmp_info_header.width; x++)
        {
            uint8_t *pixel = &row[((x)*b.imageType)];
            if (b.imageType == 3)
            {
                // averaging an RGB component.
                gray = (pixel[0] + pixel[1] + pixel[2]) / b.imageType;
            }
            else
            {
                gray = (pixel[0] + pixel[1] + pixel[2] + pixel[3]) / b.imageType;
                pixel[3] = gray;
            }
            pixel[0] = pixel[1] = pixel[2] = gray;
            pixel = pixel + b.imageType;
        }
    }
}

/**
 * Pixelats an image by creating groups of 16*16 pixel blocks.
 */
void pixelate(Bitmap &b)
{
    Bitmap originalImage = b;
    int16_t offset[16] = {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
    for (int y = 8; y < b.bmp_info_header.height - 8; y += 8)
    {
        uint8_t *row = (&originalImage.data[0] + originalImage.row_stride * y);
        for (int x = 8; x < originalImage.bmp_info_header.width - 8; x += 8)
        {
            uint32_t value[4] = {0, 0, 0, 0};
            for (int oY = 0; oY < 16; oY++)
            {
                uint8_t *oRow = (&originalImage.data[0] + originalImage.row_stride * (y + offset[oY]));
                for (int oX = 0; oX < 16; oX++)
                {
                    uint8_t *otherPixel = &oRow[0] + ((x + offset[oX]) * b.imageType);
                    value[0] += otherPixel[0];
                    value[1] += otherPixel[1];
                    value[2] += otherPixel[2];
                    if (b.imageType == 4)
                    {
                        value[3] += otherPixel[3];
                    }
                }
            }
            value[0] = value[0] / (16 * 16);
            value[1] = value[1] / (16 * 16);
            value[2] = value[2] / (16 * 16);
            if (b.imageType == 4)
            {
                value[3] = value[3] / (16 * 16);
            }

            for (int oY = 0; oY < 16; oY++)
            {
                uint8_t *oRow = (&b.data[0] + b.row_stride * (y + offset[oY]));
                for (int oX = 0; oX < 17; oX++)
                {
                    uint8_t *otherPixel = &oRow[0] + ((x + offset[oX]) * b.imageType);
                    otherPixel[0] = value[0];
                    otherPixel[1] = value[1];
                    otherPixel[2] = value[2];
                    if (b.imageType == 4)
                    {
                        otherPixel[3] = value[3];
                    }
                }
            }
        }
    }
}

/**
 * Use gaussian bluring to blur an image.
 */
void blur(Bitmap &b)
{
    // 5X5 block Kernal for gaussian blur.
    uint16_t filter[5][5] = {{1, 4, 6, 4, 1},
                             {4, 16, 24, 16, 4},
                             {6, 24, 36, 24, 6},
                             {4, 16, 24, 16, 4},
                             {1, 4, 6, 4, 1}};

    int16_t offset[5] = {-2, -1, 0, 1, 2};

    for (int y = 2; y < b.bmp_info_header.height - 3; y++)
    {
        uint8_t *row = (&b.data[0] + b.row_stride * y);
        for (int x = 2; x < b.bmp_info_header.width - 3; x++)
        {
            uint8_t *pixel = &row[0] + x * b.imageType;
            uint16_t filterValue[4] = {0, 0, 0, 0};
            for (int xOff = 0; xOff < 5; xOff++)
            {
                for (int yOff = 0; yOff < 5; yOff++)
                {
                    uint32_t yOffset = y + offset[yOff];
                    uint32_t xOffset = x + offset[xOff];
                    uint8_t *row = (&b.data[0] + b.row_stride * (yOffset));
                    uint8_t *otherPixel = &row[((xOffset)*b.imageType)];

                    filterValue[0] += (otherPixel[0] * filter[yOff][xOff]);
                    filterValue[1] += (otherPixel[1] * filter[yOff][xOff]);
                    filterValue[2] += (otherPixel[2] * filter[yOff][xOff]);
                    if (b.imageType == 4)
                    {
                        filterValue[3] += (otherPixel[3] * filter[yOff][xOff]);
                    }
                }
            }
            for (uint8_t ch = 0; ch < b.imageType; ch++)
            {
                filterValue[ch] = filterValue[ch] / 255.0;
                pixel[ch] = filterValue[ch];
            }
        }
    }
}

/**
 * rotates image 90 degrees, swapping the height and width.
 */
void rot90(Bitmap &b)
{
}

/**
 * rotates an image by 180 degrees.
 */
void rot180(Bitmap &b)
{
}

/**
 * rotates image 270 degrees, swapping the height and width.
 */
void rot270(Bitmap &b)
{
}

/**
 * flips and image over the vertical axis.
 */
void flipv(Bitmap &b)
{
    Bitmap original = b;
    for (uint16_t y = 0; y < b.bmp_info_header.height; ++y)
    {
        // store the copy of original bitmap data details.
        uint8_t *originalRow = (&original.data[0] + original.row_stride * y);
        uint8_t *originalPixel = originalRow;

        uint8_t *newRow = (&b.data[0] + original.row_stride * ((b.bmp_info_header.height - 1) - y));
        uint8_t *pixel = newRow;
        for (uint16_t x = 0; x < b.bmp_info_header.width * b.imageType; x += b.imageType)
        {
            pixel[x + 0] = originalPixel[x + 0];
            pixel[x + 1] = originalPixel[x + 1];
            pixel[x + 2] = originalPixel[x + 2];
            if (b.imageType == 4)
            {
                pixel[x + 3] = originalPixel[x + 3];
            }
        }
    }
}

/**
 * flips and image over the horizontal axis.
 */
void fliph(Bitmap &b)
{
    Bitmap original = b;
    for (uint16_t y = 0; y < b.bmp_info_header.height; ++y)
    {
        uint8_t *row = (&b.data[0] + b.row_stride * y);
        uint8_t *pixel = row;

        // store the copy of original bitmap data details.
        uint8_t *originalRow = (&original.data[0] + original.row_stride * y);
        uint8_t *originalPixel = originalRow;

        for (uint16_t x = 0; x < b.bmp_info_header.width * b.imageType; x += b.imageType)
        {
            uint16_t offset = (b.bmp_info_header.width - 1) * b.imageType - x;
            pixel[x + 0] = originalPixel[offset + 0];
            pixel[x + 1] = originalPixel[offset + 1];
            pixel[x + 2] = originalPixel[offset + 2];
            if (b.imageType == 4)
            {
                pixel[x + 3] = originalPixel[offset + 3];
            }
        }
    }
}

/**
 * flips and image over the line y = -x, swapping the height and width.
 */
void flipd1(Bitmap &b)
{
}

/**
 * flips and image over the line y = xr, swapping the height and width.
 */
void flipd2(Bitmap &b)
{
}

/**
 * scales the image by a factor of 2.
 */
void scaleUp(Bitmap &b)
{
    Bitmap original = b;
    b.bmp_info_header.width = b.bmp_info_header.width * 2;
    b.bmp_info_header.width += b.bmp_info_header.width % 4;
    b.row_stride = b.bmp_info_header.width * b.bmp_info_header.bit_count / 8;
    b.bmp_info_header.height = b.bmp_info_header.height * 2;

    uint32_t r = 0;
    for (uint16_t y = 0; y < original.bmp_info_header.height; ++y)
    {
        uint8_t *originalRow = (&original.data[0] + original.row_stride * y);
        uint8_t *originalPixel = originalRow;
        for (uint16_t rCount = 0; rCount < 2; rCount++)
        {
            uint32_t c = 0;
            uint8_t *row = (&b.data[0] + b.row_stride * r);
            uint8_t *pixel = row;
            for (uint16_t x = 0; x < b.bmp_info_header.width; x++)
            {
                for (uint16_t cCount = 0; cCount < 2; cCount++)
                {
                    pixel[(c * b.imageType) + 0] = originalPixel[(x * b.imageType) + 0];
                    pixel[(c * b.imageType) + 1] = originalPixel[(x * b.imageType) + 1];
                    pixel[(c * b.imageType) + 2] = originalPixel[(x * b.imageType) + 2];
                    if (b.imageType == 4)
                    {
                        pixel[(c * b.imageType) + 3] = originalPixel[(x * b.imageType) + 3];
                    }
                    c++;
                }
            }
            r++;
            while (c < b.bmp_info_header.width)
            {
                pixel[(c * b.imageType) + 0] = 0;
                pixel[(c * b.imageType) + 1] = 0;
                pixel[(c * b.imageType) + 2] = 0;
                if (b.imageType == 4)
                {
                    pixel[(c * b.imageType) + 3] = 0;
                }
                c++;
            }
        }
    }
}

/**
 * scales the image by a factor of 1/2.
 */
void scaleDown(Bitmap &b)
{
    Bitmap original = b;
    b.bmp_info_header.width = b.bmp_info_header.width / 2;
    b.bmp_info_header.width += b.bmp_info_header.width % 4;
    b.row_stride = b.bmp_info_header.width * b.bmp_info_header.bit_count / 8;
    b.bmp_info_header.height = b.bmp_info_header.height / 2;

    uint16_t r = 0;
    uint16_t c = 0;
    for (uint16_t y = 0; y < original.bmp_info_header.height; ++y)
    {
        if (y % 2 == 0)
        {
            continue;
        }
        uint8_t *row = (&b.data[0] + b.row_stride * r);
        uint8_t *pixel = row;
        uint8_t *originalRow = (&original.data[0] + original.row_stride * (y));
        uint8_t *originalPixel = originalRow;
        c = 0;

        for (uint16_t x = 0; x < original.bmp_info_header.width; x++)
        {
            if (c >= b.row_stride)
            {
                break;
            }
            if (x % 2 == 0)
            {
                continue;
            }
            pixel[(c * b.imageType) + 0] = originalPixel[(x * b.imageType) + 0];
            pixel[(c * b.imageType) + 1] = originalPixel[(x * b.imageType) + 1];
            pixel[(c * b.imageType) + 2] = originalPixel[(x * b.imageType) + 2];
            if (b.imageType == 4)
            {
                pixel[(c * b.imageType) + 3] = originalPixel[(x * b.imageType) + 3];
            }
            c++;
        }
        while (c < b.bmp_info_header.width)
        {
            pixel[(c * b.imageType) + 0] = 0;
            pixel[(c * b.imageType) + 1] = 0;
            pixel[(c * b.imageType) + 2] = 0;
            if (b.imageType == 4)
            {
                pixel[(c * b.imageType) + 3] = 0;
            }
            c++;
        }
        r++;
    }
}
