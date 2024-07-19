#pragma once

#include <mbed.h>

// basic types for image processing

namespace cv
{
    template<class T> bool
    is_aligned(const void * ptr) noexcept
    {
        auto iptr = reinterpret_cast<std::uintptr_t>(ptr);
        return !(iptr % alignof(T));
    }

    template<typename _Pt> struct area_type
    {
        typedef _Pt type;
    };

    template<> struct area_type<short>
    {
        typedef int type;
    };

    template<typename _Tp> class Size_;
    template<typename _Tp> class Rect_;

    template<typename _Tp> class Point_
    {
    public:
        Point_() = default;
        Point_(const Point_<_Tp>& r) = default;
        Point_(_Tp _x, _Tp _y);
        Point_(const Size_<_Tp>& sz);
        Point_<_Tp>& operator = (const Point_<_Tp>& r) = default;

        bool inside(const Rect_<_Tp>& r) const;

        _Tp x = 0; //!< x coordinate of the point
        _Tp y = 0; //!< y coordinate of the point
    };

    typedef Point_<int> Point2i;
    typedef Point_<short> Point2s;
    typedef Point_<float> Point2f;
    typedef Point2i Point;

    template<typename _Tp> class Size_
    {
    public:
        Size_() = default;
        Size_(const Size_<_Tp>& r) = default;
        Size_(_Tp _width, _Tp _height);
        Size_(const Point_<_Tp>& pt);
        Size_<_Tp>& operator = (const Size_<_Tp>& r) = default;

        typename area_type<_Tp>::type area() const;
        float aspectRatio() const;
        bool empty() const;

        _Tp width = 0; //!< the width
        _Tp height = 0; //!< the height
    };

    typedef Size_<int> Size2i;
    typedef Size_<short> Size2s;
    typedef Size_<float> Size2f;
    typedef Size2i Size;

    template<typename _Tp> class Rect_
    {
    public:
        Rect_() = default;
        Rect_(const Rect_<_Tp>& r) = default;
        Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height);
        Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz);
        Rect_(const Point_<_Tp>& pt1, const Point_<_Tp>& pt2);
        Rect_<_Tp>& operator = (const Rect_<_Tp>& r) = default;

        Point_<_Tp> tl() const;
        Point_<_Tp> br() const;
        Size_<_Tp> size() const;
        typename area_type<_Tp>::type area() const;
        bool empty() const;
        bool contains(const Point_<_Tp>& pt) const;

        _Tp x = 0; //!< x coordinate of the top-left corner
        _Tp y = 0; //!< y coordinate of the top-left corner
        _Tp width = 0; //!< width of the rectangle
        _Tp height = 0; //!< height of the rectangle
    };

    typedef Rect_<int> Rect2i;
    typedef Rect_<short> Rect2s;
    typedef Rect_<float> Rect2f;
    typedef Rect2i Rect;

    class RotatedRect
    {
    public:
        RotatedRect() = default;
        RotatedRect(const Point2f& center, const Size2f& size, float angle);
        RotatedRect(const Point2f& point1, const Point2f& point2, const Point2f& point3);
        void points(Point2f pts[]) const;
        Rect boundingRect() const;
        Rect2f boundingRect2f() const;

        Point2f center;
        Size2f size;
        float angle = 0;
    };

    class Range
    {
    public:
        Range() = default;
        Range(const Range& r) = default;
        Range(int _start, int _end);
        Range& operator = (const Range& r) = default;
        int size() const;
        bool empty() const;
        static Range all();

        int start = 0, end = 0;
    };

    enum MatType { MONO8 = 0, RGB332 = 0, RGB565 = 1, ARGB1555 = 1 };
    constexpr size_t AUTO_STEP = 0;

    class Mat
    {
    public:
        Mat() = default;
        Mat(const Mat& m) = default;
        Mat(int _rows, int _cols, int _type, void* _data, size_t _step=AUTO_STEP);
        Mat(Size size, int _type, void* _data, size_t _step=AUTO_STEP);
        Mat(const Mat& m, const Rect& roi);
        Mat(const Mat& m, const Range& rowRange, const Range& colRange=Range::all());
        Mat& operator = (const Mat& m) = default;
        Mat row(int y) const;
        Mat col(int x) const;
        Mat rowRange(int startrow, int endrow) const;
        Mat colRange(int startcol, int endcol) const;
        Mat& operator = (const uint16_t& s);
        Mat operator()( const Rect& roi ) const;
        Size size() const;
        bool isContinuous() const;
        size_t elemSize() const;
        bool empty() const;
        size_t total() const;
        bool copyTo(Mat arr) const;
    
        template<typename _Tp> _Tp* ptr(int row = 0)
        {
                return reinterpret_cast<_Tp*>(data + step[0] * row);
        }

        template<typename _Tp> const _Tp* ptr(int row = 0) const
        {
                return reinterpret_cast<const _Tp*>(data + step[0] * row);
        }

        template<typename _Tp> _Tp* ptr(int row, int col)
        {
            return reinterpret_cast<_Tp*>(data + row * step[0] + col * step[1]);
        }

        template<typename _Tp> const _Tp* ptr(int row, int col) const
        {
            return reinterpret_cast<const _Tp*>(data + row * step[0] + col * step[1]);
        }

        template<typename _Tp> _Tp& at(int row, int col)
        {
            return *reinterpret_cast<_Tp*>(data + row * step[0] + col * step[1]);
        }

        template<typename _Tp> const _Tp& at(int row, int col) const
        {
            return *reinterpret_cast<const _Tp*>(data + row * step[0] + col * step[1]);
        }

        int rows = 0, cols = 0, type = 0;
        size_t step[2] = { 0, 0 };
        uint8_t* data = nullptr;
    };

    template<typename _Tp> static inline
    bool operator == (const Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        return a.x == b.x && a.y == b.y;
    }

    template<typename _Tp> static inline
    bool operator != (const Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        return !(a == b);
    }

    template<typename _Tp> static inline
    Point_<_Tp>& operator += (Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    template<typename _Tp> static inline
    Point_<_Tp>& operator -= (Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }

    template<typename _Tp> static inline
    Point_<_Tp> operator + (const Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        return Point_<_Tp>(a.x + b.x, a.y + b.y);
    }

    template<typename _Tp> static inline
    Point_<_Tp> operator - (const Point_<_Tp>& a, const Point_<_Tp>& b)
    {
        return Point_<_Tp>( a.x - b.x, a.y - b.y );
    }

    template<typename _Tp> static inline
    float norm(const Point_<_Tp>& pt)
    {
        return std::sqrt((float)pt.x*pt.x + (float)pt.y*pt.y);
    }

    template<typename _Tp> static inline
    bool operator == (const Size_<_Tp>& a, const Size_<_Tp>& b)
    {
        return a.width == b.width && a.height == b.height;
    }

    template<typename _Tp> static inline
    bool operator != (const Size_<_Tp>& a, const Size_<_Tp>& b)
    {
        return !(a == b);
    }

    template<typename _Tp> static inline
    bool operator == (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
    {
        return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    }

    template<typename _Tp> static inline
    bool operator != (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
    {
        return !(a == b);
    }

    template<typename _Tp> static inline
    Rect_<_Tp>& operator &= (Rect_<_Tp>& a, const Rect_<_Tp>& b)
    {
        if (a.empty() || b.empty()) {
            a = Rect();
            return a;
        }
        const Rect_<_Tp>& Rx_min = (a.x < b.x) ? a : b;
        const Rect_<_Tp>& Rx_max = (a.x < b.x) ? b : a;
        const Rect_<_Tp>& Ry_min = (a.y < b.y) ? a : b;
        const Rect_<_Tp>& Ry_max = (a.y < b.y) ? b : a;
        if ((Rx_min.x < 0 && Rx_min.x + Rx_min.width < Rx_max.x) ||
            (Ry_min.y < 0 && Ry_min.y + Ry_min.height < Ry_max.y)) {
            a = Rect();
            return a;
        }
        a.width = std::min(Rx_min.width - (Rx_max.x - Rx_min.x), Rx_max.width);
        a.height = std::min(Ry_min.height - (Ry_max.y - Ry_min.y), Ry_max.height);
        a.x = Rx_max.x;
        a.y = Ry_max.y;
        if (a.empty())
            a = Rect();
        return a;
    }

    template<typename _Tp> static inline
    Rect_<_Tp>& operator |= (Rect_<_Tp>& a, const Rect_<_Tp>& b )
    {
        if (a.empty()) {
            a = b;
        }
        else if (!b.empty()) {
            _Tp x1 = std::min(a.x, b.x);
            _Tp y1 = std::min(a.y, b.y);
            a.width = std::max(a.x + a.width, b.x + b.width) - x1;
            a.height = std::max(a.y + a.height, b.y + b.height) - y1;
            a.x = x1;
            a.y = y1;
        }
        return a;
    }

    template<typename _Tp> static inline
    Rect_<_Tp> operator & (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
    {
        Rect_<_Tp> c = a;
        return c &= b;
    }

    template<typename _Tp> static inline
    Rect_<_Tp> operator | (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
    {
        Rect_<_Tp> c = a;
        return c |= b;
    }

    inline bool operator == (const Range& a, const Range& b)
    {
        return a.start == b.start && a.end == b.end;
    }

    inline bool operator != (const Range& a, const Range& b)
    {
        return !(a == b);
    }

    void transpose(const Mat& src, Mat& dest);

    void rotate_left(const Mat& src, Mat& dest);

    void rotate_right(const Mat& src, Mat& dest);

    void flip_vert(const Mat& src, Mat& dest);

    void flip_horiz(const Mat& src, Mat& dest);

}

constexpr float CV_PI = 3.1415926f;

inline int cvRound(float value)
{
    return int(std::lround(value));
}

inline int cvCeil(float value)
{
    int i = (int)value;
    return i + (i < value);
}

inline int cvFloor(float value)
{
    int i = (int)value;
    return i - (i > value);
}
