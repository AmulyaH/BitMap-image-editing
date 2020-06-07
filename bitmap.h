#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <iostream>
#include <vector>
#include <exception>
#include <stdexcept>

using namespace std;

#pragma pack(push, 1)

/**
 * struct to hold the bit map file header information
*/
struct BMPFileHeader
{
    uint16_t file_type{0};   // File type always BM which is 0x4D42
    uint32_t file_size{0};   // Size of the file (in bytes)
    uint16_t reserved1{0};   // Reserved, always 0
    uint16_t reserved2{0};   // Reserved, always 0
    uint32_t offset_data{0}; // Start position of pixel data (bytes from the beginning of the file)
};

/**
 * struct to hold the bit map header information
*/
struct BMPInfoHeader
{
    uint32_t size{0};        // Size of this header (in bytes)
    int32_t width{0};        // width of bitmap in pixels
    int32_t height{0};       // width of bitmap in pixels
                             //       (if positive, bottom-up, with origin in lower left corner)
                             //       (if negative, top-down, with origin in upper left corner)
    uint16_t planes{0};      // No. of planes for the target device, this is always 1
    uint16_t bit_count{0};   // No. of bits per pixel
    uint32_t compression{0}; // 0 or 3 - uncompressed.
    uint32_t size_image{0};  // 0 - for uncompressed images
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};      // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{0}; // No. of colors used for displaying the bitmap. If 0 all colors are required
};

/**
 * struct to hold the bit map header colour information
*/
struct BMPColorHeader
{
    uint32_t red_mask{0x00ff0000};         // Bit mask for the red channel
    uint32_t green_mask{0x0000ff00};       // Bit mask for the green channel
    uint32_t blue_mask{0x000000ff};        // Bit mask for the blue channel
    uint32_t alpha_mask{0xff000000};       // Bit mask for the alpha channel
    uint32_t color_space_type{0x73524742}; // Default "sRGB" (0x73524742)
    uint32_t unused[16]{0};                // Unused data for sRGB color space
};
#pragma pack(pop)

class Bitmap
{
private:
    friend std::istream &operator>>(std::istream &in, Bitmap &b);
    friend std::ostream &operator<<(std::ostream &out, Bitmap &b);

    /** 
     *  To align strid, add 1 to the row_stride until it is divisible with align_stride
     * @param align_stride value to identify when the stride is divisable by 4
    */
    uint32_t make_stride_aligned(uint32_t align_stride);

    /**
     * Write the binary representation of image header to stream
     * @param out stream to write to.
     * 
    */
    void write_headers(std::ostream &out);

    /**
     * Write the binary representation of image header and data to stream
     * @param out stream to write to.
     * 
    */
    void write_headers_and_data(std::ostream &out);

public:
    Bitmap()
    {
    }
    uint32_t row_stride{0};
    uint16_t imageType = 0;
    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
    BMPColorHeader bmp_color_header;
    std::vector<uint8_t> data;
};

/**
 * returns the nearest number to the given value.
 * @param inValue pixel for which nearest number to be find.
 *  
*/
uint8_t nearesetNumber(uint8_t inValue);

/**
 * cell shade an image.
 * for each component of each pixel we round to 
 * the nearest number of 0, 180, 255
 *
 * This has the effect of making the image look like.
 * it was colored.
 */
void cellShade(Bitmap &b);

/**
 * Grayscales an image by averaging all of the components.
 */
void grayscale(Bitmap &b);

/**
 * Pixelats an image by creating groups of 16*16 pixel blocks.
 */
void pixelate(Bitmap &b);

/**
 * Use gaussian bluring to blur an image.
 */
void blur(Bitmap &b);

/**
 * rotates image 90 degrees, swapping the height and width.
 */
void rot90(Bitmap &b);

/**
 * rotates an image by 180 degrees.
 */
void rot180(Bitmap &b);

/**
 * rotates image 270 degrees, swapping the height and width.
 */
void rot270(Bitmap &b);

/**
 * flips and image over the vertical axis.
 */
void flipv(Bitmap &b);

/**
 * flips and image over the horizontal axis.
 */
void fliph(Bitmap &b);

/**
 * flips and image over the line y = -x, swapping the height and width.
 */
void flipd1(Bitmap &b);

/**
 * flips and image over the line y = xr, swapping the height and width.
 */
void flipd2(Bitmap &b);

/**
 * scales the image by a factor of 2.
 */
void scaleUp(Bitmap &b);

/**
 * scales the image by a factor of 1/2.
 */
void scaleDown(Bitmap &b);

/**
 * BitmapException denotes an exception from reading in a bitmap.
 */
class BitmapException : public std::exception
{
    // the message to print out
    std::string _message;

    // position in the bitmap file (in bytes) where the error occured.
    uint32_t _position;

public:
    BitmapException() = delete;

    BitmapException(const std::string &message, uint32_t position);
    BitmapException(std::string &&message, uint32_t position);
    /**
     * prints out the exception in the form:
     *
     * "Error in bitmap at position 0xposition :
     * message"
     */
    void print_exception();
};

#endif