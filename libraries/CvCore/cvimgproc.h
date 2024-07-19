#pragma once

#include <mbed.h>
#include <vector>
#include "cvcore.h"
#include "cvfonts.h"
#include "cvspan.h"
#include "dmaops.h"

#ifndef USE_DIRTY_RECT
#define USE_DIRTY_RECT 1
#endif

namespace cv
{
    constexpr int FILLED = -1;

    constexpr uint16_t RGB565_BLACK = 0x0000;
    constexpr uint16_t RGB565_BLUE = 0x001F;
    constexpr uint16_t RGB565_RED = 0xF800;
    constexpr uint16_t RGB565_GREEN = 0x07E0;
    constexpr uint16_t RGB565_CYAN = 0x07FF;
    constexpr uint16_t RGB565_MAGENTA = 0xF81F;
    constexpr uint16_t RGB565_YELLOW = 0xFFE0;
    constexpr uint16_t RGB565_WHITE = 0xFFFF;
    constexpr uint16_t RGB565_GRAY = 0x8410;
    constexpr uint16_t RGB565_SILVER = 0xBDF7;

    constexpr uint8_t RGB332_BLACK = 0x00;
    constexpr uint8_t RGB332_BLUE = 0x03;
    constexpr uint8_t RGB332_RED = 0xE0;
    constexpr uint8_t RGB332_GREEN = 0x1C;
    constexpr uint8_t RGB332_CYAN = 0x1F;
    constexpr uint8_t RGB332_MAGENTA = 0xE3;
    constexpr uint8_t RGB332_YELLOW = 0xFC;
    constexpr uint8_t RGB332_WHITE = 0xFF;

    /** Possible set of marker types used for the cv::drawMarker function
    @ingroup imgproc_draw
    */
    enum MarkerTypes
    {
        MARKER_CROSS = 0,           //!< A crosshair marker shape
        MARKER_TILTED_CROSS = 1,    //!< A 45 degree tilted crosshair marker shape
        MARKER_STAR = 2,            //!< A star marker shape, combination of cross and tilted cross
        MARKER_DIAMOND = 3,         //!< A diamond marker shape
        MARKER_SQUARE = 4,          //!< A square marker shape
        MARKER_TRIANGLE_UP = 5,     //!< An upwards pointing triangle marker shape
        MARKER_TRIANGLE_DOWN = 6,   //!< A downwards pointing triangle marker shape
        MARKER_POINT = 7            //!< A single point
    };

    class Painter
    {
    public:
        Painter(const Mat& _mat);

        void fill(uint16_t color);

        void rectangle(Point pt1, Point pt2, uint16_t color, int thickness=1);

        void line(Point pt1, Point pt2, uint16_t color, int thickness=1);

        void circle(Point center, int radius, uint16_t color, int thickness=1);

        void ellipse(const RotatedRect& box, uint16_t color, int thickness=1);

        void ellipse(Point center, Size axes, float angle, float startAngle, float endAngle, uint16_t color, int thickness=1);

        void polyline(const std::vector<Point>& contour, uint16_t color, int thickness=1);

        void putText(std::string_view text, Point org, FontBase& font, uint16_t text_color, uint16_t bg_color, int wrap_width = 0, size_t *consumed_chars = nullptr);

        void putText(std::wstring_view text, Point org, UnicodeFont& font, uint16_t text_color, uint16_t bg_color, int wrap_width = 0, size_t *consumed_chars = nullptr);

        void drawBitmap(const Mat& bitmap, Point org);

        void drawBitmapWithAlpha(const Mat& bitmap, Point org);

        void drawMarker(Point position, uint16_t color, int markerType, int markerSize = 1, int thickness = 1);

        void set_mat(const cv::Mat& mat_);

        Mat get_mat() const;

        Size get_mat_size() const;

        span<const Rect> get_dirty_rects() const;

        void reset_dirty_rects();

#if USE_DIRTY_RECT
        void update_dirty_rect(Rect rc);
#endif

    private:
        Mat mat;
#if USE_DIRTY_RECT
        constexpr static size_t max_dirty_rect_count = 10;
        Rect dirty_rect_list[max_dirty_rect_count];
        int dirty_rect_count = 0;
        int dirty_rect_area = 0;
#endif
        cv::Rect default_dirty_rect;
    };

    extern const uint16_t RGB332to565LUT[256];

    Rect2f boundingRect(const std::vector<cv::Point2f>& pts);

    Rect boundingRect(const std::vector<cv::Point>& pts);

    inline uint16_t rgb332_to_rgb565(uint8_t src)
    {
        return RGB332to565LUT[src];
    }

    inline uint16_t swap_bytes(uint16_t src)
    {
        return (src >> 8) | (src << 8);
    }

    inline uint16_t rgb332_to_rgb565_swapped(uint8_t src)
    {
        return swap_bytes(RGB332to565LUT[src]);
    }

    enum ColorConversionCodes
    { 
      COLOR_RGB332_R, COLOR_RGB332_G, COLOR_RGB332_B, COLOR_RGB332_GRAY, GRAY_RGB332,
      COLOR_RGB565_R, COLOR_RGB565_G, COLOR_RGB565_B, COLOR_RGB565_GRAY, GRAY_RGB565,
      COLOR_RGB332_BGR332, COLOR_RGB565_BGR565, COLOR_RGB332_RGB565, COLOR_RGB565_RGB332
    };

    void cvtColor(const Mat& src, Mat& dest, int code);
}
