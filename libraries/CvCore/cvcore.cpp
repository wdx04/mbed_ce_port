#include "cvcore.h"
#include <climits>
#include <utility>
#include <algorithm>
#include <cmath>

namespace cv
{
    template<typename _Tp>
    Point_<_Tp>::Point_(_Tp _x, _Tp _y)
        : x(_x), y(_y)
    {
    }

    template<typename _Tp>
    Point_<_Tp>::Point_(const Size_<_Tp>& sz)
        : x(sz.width), y(sz.height)
    {
    }

    template<typename _Tp>
    bool Point_<_Tp>::inside(const Rect_<_Tp>& r) const
    {
        return r.contains(*this);
    }

    template<typename _Tp>
    Size_<_Tp>::Size_(_Tp _width, _Tp _height)
        : width(_width), height(_height)
    {
    }

    template<typename _Tp>
    Size_<_Tp>::Size_(const Point_<_Tp>& pt)
        : width(pt.x), height(pt.y)
    {
    }

    template<typename _Tp>
    typename area_type<_Tp>::type Size_<_Tp>::area() const
    {
        return typename area_type<_Tp>::type(width) * height;
    }

    template<typename _Tp>
    float Size_<_Tp>::aspectRatio() const
    {
        return static_cast<float>(width) / static_cast<float>(height);
    }

    template<typename _Tp>
    bool Size_<_Tp>::empty() const
    {
        return width <= 0 || height <= 0;
    }

    template<typename _Tp>
    Rect_<_Tp>::Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height)
        : x(_x), y(_y), width(_width), height(_height)
    {
    }

    template<typename _Tp>
    Rect_<_Tp>::Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz)
        : x(org.x), y(org.y), width(sz.width), height(sz.height)
    {
    }

    template<typename _Tp>
    Rect_<_Tp>::Rect_(const Point_<_Tp>& pt1, const Point_<_Tp>& pt2)
    {
        x = std::min(pt1.x, pt2.x);
        y = std::min(pt1.y, pt2.y);
        width = std::max(pt1.x, pt2.x) - x;
        height = std::max(pt1.y, pt2.y) - y;
    }

    template<typename _Tp>
    Point_<_Tp> Rect_<_Tp>::tl() const
    {
        return Point_<_Tp>(x, y);
    }

    template<typename _Tp>
    Point_<_Tp> Rect_<_Tp>::br() const
    {
        return Point_<_Tp>(x + width, y + height);
    }

    template<typename _Tp>
    Size_<_Tp> Rect_<_Tp>::size() const
    {
        return Size_<_Tp>(width, height);
    }

    template<typename _Tp>
    typename area_type<_Tp>::type Rect_<_Tp>::area() const
    {
        return typename area_type<_Tp>::type(width) * height;
    }

    template<typename _Tp>
    bool Rect_<_Tp>::empty() const
    {
        return width <= 0 || height <= 0;
    }

    template<typename _Tp>
    bool Rect_<_Tp>::contains(const Point_<_Tp>& pt) const
    {
        return x <= pt.x && pt.x < x + width && y <= pt.y && pt.y < y + height;
    }

    RotatedRect::RotatedRect(const Point2f& _center, const Size2f& _size, float _angle)
        : center(_center), size(_size), angle(_angle)
    {
    }

    RotatedRect::RotatedRect(const Point2f& _point1, const Point2f& _point2, const Point2f& _point3)
    {
        Point2f _center = _point1 + _point3;
        _center.x *= 0.5f;
        _center.y *= 0.5f;
        Point2f vecs[2];
        vecs[0] = _point1 - _point2;
        vecs[1] = _point2 - _point3;

        // wd_i stores which vector (0,1) or (1,2) will make the width
        // One of them will definitely have slope within -1 to 1
        int wd_i = 0;
        if( std::fabs(vecs[1].y) < std::fabs(vecs[1].x) ) wd_i = 1;
        int ht_i = (wd_i + 1) % 2;

        float _angle = std::atan(vecs[wd_i].y / vecs[wd_i].x) * 180.0f / (float) CV_PI;
        float _width = (float) norm(vecs[wd_i]);
        float _height = (float) norm(vecs[ht_i]);

        center = _center;
        size = Size2f(_width, _height);
        angle = _angle;
    }

    void RotatedRect::points(Point2f pt[]) const
    {
        float _angle = angle * CV_PI / 180.0f;
        float b = (float)cos(_angle)*0.5f;
        float a = (float)sin(_angle)*0.5f;
        pt[0].x = center.x - a*size.height - b*size.width;
        pt[0].y = center.y + b*size.height - a*size.width;
        pt[1].x = center.x + a*size.height - b*size.width;
        pt[1].y = center.y - b*size.height - a*size.width;
        pt[2].x = 2*center.x - pt[0].x;
        pt[2].y = 2*center.y - pt[0].y;
        pt[3].x = 2*center.x - pt[1].x;
        pt[3].y = 2*center.y - pt[1].y;
    }

    Rect RotatedRect::boundingRect() const
    {
        Point2f pt[4];
        points(pt);
        Rect r(cvFloor(std::min(std::min(std::min(pt[0].x, pt[1].x), pt[2].x), pt[3].x)),
            cvFloor(std::min(std::min(std::min(pt[0].y, pt[1].y), pt[2].y), pt[3].y)),
            cvCeil(std::max(std::max(std::max(pt[0].x, pt[1].x), pt[2].x), pt[3].x)),
            cvCeil(std::max(std::max(std::max(pt[0].y, pt[1].y), pt[2].y), pt[3].y)));
        r.width -= r.x - 1;
        r.height -= r.y - 1;
        return r;
    }


    Rect2f RotatedRect::boundingRect2f() const
    {
        Point2f pt[4];
        points(pt);
        Rect2f r(Point2f(min(min(min(pt[0].x, pt[1].x), pt[2].x), pt[3].x), min(min(min(pt[0].y, pt[1].y), pt[2].y), pt[3].y)),
                    Point2f(max(max(max(pt[0].x, pt[1].x), pt[2].x), pt[3].x), max(max(max(pt[0].y, pt[1].y), pt[2].y), pt[3].y)));
        return r;
    }

    Range::Range(int _start, int _end)
        : start(_start), end(_end)
    {
    }

    int Range::size() const
    {
        return end - start;
    }

    bool Range::empty() const
    {
        return end == start;
    }

    Range Range::all()
    {
        return Range(INT_MIN, INT_MAX);
    }

    Mat::Mat(int _rows, int _cols, int _type, void* _data, size_t _step)
        : rows(_rows), cols(_cols), type(_type), data(reinterpret_cast<uint8_t*>(_data))
    {
        step[0] = _step;
        step[1] = elemSize();
        if(_step == 0)
        {
            step[0] = _cols * step[1];
        }
    }

    Mat::Mat(Size size, int _type, void* _data, size_t _step)
        : rows(size.height), cols(size.width), type(_type), data(reinterpret_cast<uint8_t*>(_data))
    {
        step[0] = _step;
        step[1] = elemSize();
        if(_step == 0)
        {
            step[0] = size.width * step[1];
        }
    }

    Mat::Mat(const Mat& m, const Rect& roi)
        : rows(roi.height), cols(roi.width), type(m.type), data(m.data + roi.y*m.step[0])
    {
        step[0] = m.step[0];
        step[1] = m.step[1];
        data += roi.x * m.step[1];
    }

    Mat::Mat(const Mat& m, const Range& _rowRange, const Range& _colRange)
    {
        *this = m;
        if( _rowRange != Range::all() && _rowRange != Range(0,rows) )
        {
            rows = _rowRange.size();
            data += step[0]*_rowRange.start;
        }
        if( _colRange != Range::all() && _colRange != Range(0,cols) )
        {
            cols = _colRange.size();
            data += step[1]*_colRange.start;
        }
    }

    Mat Mat::row(int y) const
    {
        return Mat(*this, Range(y, y + 1), Range::all());
    }

    Mat Mat::col(int x) const
    {
        return Mat(*this, Range::all(), Range(x, x + 1));
    }

    Mat Mat::rowRange(int startrow, int endrow) const
    {
        return Mat(*this, Range(startrow, endrow), Range::all());
    }

    Mat Mat::colRange(int startcol, int endcol) const
    {
        return Mat(*this, Range::all(), Range(startcol, endcol));
    }

    Mat& Mat::operator =(const uint16_t& s)
    {
        switch (type)
        {
        case MONO8:
        {
            uint8_t v = static_cast<uint8_t>(s);
            if (isContinuous())
            {
                memset(ptr<uint8_t>(), v, total());
            }
            else
            {
                for (int y = 0; y < rows; y++)
                {
                    memset(ptr<uint8_t>(y), static_cast<size_t>(cols), v);
                }
            }
            break;
        }
        case RGB565:
        {
            if (isContinuous())
            {
                size_t total_pixels = total();
                uint16_t *p_data = ptr<uint16_t>();
                if(total_pixels > 0)
                {
                    if((reinterpret_cast<uint32_t>(p_data) & 2) != 0)
                    {
                        *p_data++ = s;
                        total_pixels--;
                    }
                    uint32_t ss = (uint32_t(s) << 16) | uint32_t(s);
                    std::fill_n(reinterpret_cast<uint32_t*>(p_data), total_pixels >> 1, ss);
                    if((total_pixels & 1) != 0)
                    {
                        p_data[total_pixels - 1] = s;
                    }
                }
            }
            else
            {
                for (int y = 0; y < rows; y++)
                {
                    uint16_t *p_data = ptr<uint16_t>(y);
                    int total_pixels = cols;
                    if(total_pixels > 0)
                    {
                        if((reinterpret_cast<uint32_t>(p_data) & 2) != 0)
                        {
                            *p_data++ = s;
                            total_pixels--;
                        }
                        uint32_t ss = (uint32_t(s) << 16) | uint32_t(s);
                        std::fill_n(reinterpret_cast<uint32_t*>(p_data), total_pixels >> 1, ss);
                        if((total_pixels & 1) != 0)
                        {
                            p_data[total_pixels - 1] = s;
                        }
                    }
                }
            }
            break;
        }
        }
        return *this;
    }

    Mat Mat::operator()( const Rect& roi ) const
    {
        return Mat(*this, roi);
    }

    bool Mat:: isContinuous() const
    {
        return step[0] == step[1] * cols;
    }

    Size Mat:: size() const
    {
        return Size(cols, rows);
    }

    size_t Mat::elemSize() const
    {
        switch(type)
        {
            case MONO8:
                return 1;
            case RGB565:
                return 2;
        }
        return 0;
    }

    bool Mat::empty() const
    {
        return cols <= 0 || rows <= 0;
    }

    size_t Mat::total() const
    {
        return cols * rows;
    }

    bool Mat::copyTo(Mat arr) const
    {
        if (type != arr.type)
        {
            return false;
        }
        int rows_to_copy = std::min(rows, arr.rows);
        int cols_to_copy = std::min(cols, arr.cols);
        if (rows_to_copy > 0 && cols_to_copy > 0)
        {
            switch (type)
            {
            case MONO8:
            {
                for (int y = 0; y < rows_to_copy; y++)
                {
                    std::copy_n(ptr<uint8_t>(y), cols_to_copy, arr.ptr<uint8_t>(y));
                }
                break;
            }
            case RGB565:
            {
                for (int y = 0; y < rows_to_copy; y++)
                {
                    std::copy_n(ptr<uint16_t>(y), cols_to_copy, arr.ptr<uint16_t>(y));
                }
                break;
            }
            }
        }
        return true;
    }

    template<typename T> static void
    transpose_(const uint8_t* src, size_t sstep, uint8_t* dst, size_t dstep, cv::Size sz)
    {
        int i = 0, j, m = sz.width, n = sz.height;

        for (; i <= m - 4; i += 4)
        {
            T* d0 = (T*)(dst + dstep * i);
            T* d1 = (T*)(dst + dstep * (i + 1));
            T* d2 = (T*)(dst + dstep * (i + 2));
            T* d3 = (T*)(dst + dstep * (i + 3));

            for (j = 0; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * j);
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (j + 1));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (j + 2));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (j + 3));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
                d1[j] = s0[1]; d1[j + 1] = s1[1]; d1[j + 2] = s2[1]; d1[j + 3] = s3[1];
                d2[j] = s0[2]; d2[j + 1] = s1[2]; d2[j + 2] = s2[2]; d2[j + 3] = s3[2];
                d3[j] = s0[3]; d3[j + 1] = s1[3]; d3[j + 2] = s2[3]; d3[j + 3] = s3[3];
            }

            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + j * sstep);
                d0[j] = s0[0]; d1[j] = s0[1]; d2[j] = s0[2]; d3[j] = s0[3];
            }
        }
        for (; i < m; i++)
        {
            T* d0 = (T*)(dst + dstep * i);
            j = 0;
            for (; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * j);
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (j + 1));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (j + 2));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (j + 3));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
            }
            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + j * sstep);
                d0[j] = s0[0];
            }
        }
    }

    template<typename T> static void
    rotate_left_(const uint8_t* src, size_t sstep, uint8_t* dst, size_t dstep, cv::Size sz)
    {
        int i = 0, j, m = sz.width, n = sz.height;

        for (; i <= m - 4; i += 4)
        {
            T* d0 = (T*)(dst + dstep * (m - 1 - i));
            T* d1 = (T*)(dst + dstep * (m - 2 - i));
            T* d2 = (T*)(dst + dstep * (m - 3 - i));
            T* d3 = (T*)(dst + dstep * (m - 4 - i));

            for (j = 0; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * j);
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (j + 1));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (j + 2));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (j + 3));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
                d1[j] = s0[1]; d1[j + 1] = s1[1]; d1[j + 2] = s2[1]; d1[j + 3] = s3[1];
                d2[j] = s0[2]; d2[j + 1] = s1[2]; d2[j + 2] = s2[2]; d2[j + 3] = s3[2];
                d3[j] = s0[3]; d3[j + 1] = s1[3]; d3[j + 2] = s2[3]; d3[j + 3] = s3[3];
            }

            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + j * sstep);
                d0[j] = s0[0]; d1[j] = s0[1]; d2[j] = s0[2]; d3[j] = s0[3];
            }
        }
        for (; i < m; i++)
        {
            T* d0 = (T*)(dst + dstep * (m - 1 - i));
            j = 0;
            for (; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * j);
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (j + 1));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (j + 2));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (j + 3));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
            }
            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + j * sstep);
                d0[j] = s0[0];
            }
        }
    }

    template<typename T> static void
    rotate_right_(const uint8_t* src, size_t sstep, uint8_t* dst, size_t dstep, cv::Size sz)
    {
        int i = 0, j, m = sz.width, n = sz.height;

        for (; i <= m - 4; i += 4)
        {
            T* d0 = (T*)(dst + dstep * i);
            T* d1 = (T*)(dst + dstep * (i + 1));
            T* d2 = (T*)(dst + dstep * (i + 2));
            T* d3 = (T*)(dst + dstep * (i + 3));

            for (j = 0; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 1));
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 2));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 3));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 4));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
                d1[j] = s0[1]; d1[j + 1] = s1[1]; d1[j + 2] = s2[1]; d1[j + 3] = s3[1];
                d2[j] = s0[2]; d2[j + 1] = s1[2]; d2[j + 2] = s2[2]; d2[j + 3] = s3[2];
                d3[j] = s0[3]; d3[j + 1] = s1[3]; d3[j + 2] = s2[3]; d3[j + 3] = s3[3];
            }

            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + (n - j - 1) * sstep);
                d0[j] = s0[0]; d1[j] = s0[1]; d2[j] = s0[2]; d3[j] = s0[3];
            }
        }
        for (; i < m; i++)
        {
            T* d0 = (T*)(dst + dstep * i);
            j = 0;
            for (; j <= n - 4; j += 4)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 1));
                const T* s1 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 2));
                const T* s2 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 3));
                const T* s3 = (const T*)(src + i * sizeof(T) + sstep * (n - j - 4));

                d0[j] = s0[0]; d0[j + 1] = s1[0]; d0[j + 2] = s2[0]; d0[j + 3] = s3[0];
            }
            for (; j < n; j++)
            {
                const T* s0 = (const T*)(src + i * sizeof(T) + (n - j - 1) * sstep);
                d0[j] = s0[0];
            }
        }
    }

    void transpose(const Mat& src, Mat& dest)
    {
        if(src.elemSize() == 1)
        {
            transpose_<uint8_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
        else
        {
            transpose_<uint16_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
    }

    void rotate_left(const Mat& src, Mat& dest)
    {
        if(src.elemSize() == 1)
        {
            rotate_left_<uint8_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
        else
        {
            rotate_left_<uint16_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
    }

    void rotate_right(const Mat& src, Mat& dest)
    {
        if(src.elemSize() == 1)
        {
            rotate_right_<uint8_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
        else
        {
            rotate_right_<uint16_t>(src.ptr<uint8_t>(), src.step[0], dest.ptr<uint8_t>(), dest.step[0], src.size());
        }
    }

    void flip_vert(const Mat& src, Mat& dest)
    {
        size_t esz = src.elemSize();
        cv::Size size = src.size();
        int sstep = (int) src.step[0];
        int dstep = (int) dest.step[0];
        const uint8_t *src0 = src.ptr<uint8_t>();
        uint8_t *dst0 = dest.ptr<uint8_t>();
        const uint8_t* src1 = src0 + (size.height - 1)*sstep;
        uint8_t* dst1 = dst0 + (size.height - 1)*dstep;
        size.width *= (int) esz;

        for( int y = 0; y < (size.height + 1)/2; y++, src0 += sstep, src1 -= sstep,
                                                    dst0 += dstep, dst1 -= dstep )
        {
            int i = 0;
            if (is_aligned<int>(src0) && is_aligned<int>(src1) && is_aligned<int>(dst0) && is_aligned<int>(dst1))
            {
                for( ; i <= size.width - 16; i += 16 )
                {
                    int t0 = ((int*)(src0 + i))[0];
                    int t1 = ((int*)(src1 + i))[0];

                    ((int*)(dst0 + i))[0] = t1;
                    ((int*)(dst1 + i))[0] = t0;

                    t0 = ((int*)(src0 + i))[1];
                    t1 = ((int*)(src1 + i))[1];

                    ((int*)(dst0 + i))[1] = t1;
                    ((int*)(dst1 + i))[1] = t0;

                    t0 = ((int*)(src0 + i))[2];
                    t1 = ((int*)(src1 + i))[2];

                    ((int*)(dst0 + i))[2] = t1;
                    ((int*)(dst1 + i))[2] = t0;

                    t0 = ((int*)(src0 + i))[3];
                    t1 = ((int*)(src1 + i))[3];

                    ((int*)(dst0 + i))[3] = t1;
                    ((int*)(dst1 + i))[3] = t0;
                }

                for( ; i <= size.width - 4; i += 4 )
                {
                    int t0 = ((int*)(src0 + i))[0];
                    int t1 = ((int*)(src1 + i))[0];

                    ((int*)(dst0 + i))[0] = t1;
                    ((int*)(dst1 + i))[0] = t0;
                }
            }

            for( ; i < size.width; i++ )
            {
                uint8_t t0 = src0[i];
                uint8_t t1 = src1[i];

                dst0[i] = t1;
                dst1[i] = t0;
            }
        }
    }

    void flip_horiz(const Mat& src, Mat& dest)
    {
        size_t esz = src.elemSize();
        cv::Size size = src.size();
        int sstep = (int) src.step[0];
        int dstep = (int) dest.step[0];
        const uint8_t *src0 = src.ptr<uint8_t>();
        uint8_t *dst0 = dest.ptr<uint8_t>();
        int i, j, limit = (int)(((size.width + 1)/2)*esz);
        std::vector<int> _tab(size.width*esz);
        int* tab = _tab.data();

        for( i = 0; i < size.width; i++ )
            for( size_t k = 0; k < esz; k++ )
                tab[i*esz + k] = (int)((size.width - i - 1)*esz + k);

        for( ; size.height--; src0 += sstep, dst0 += dstep )
        {
            for( i = 0; i < limit; i++ )
            {
                j = tab[i];
                uint8_t t0 = src0[i], t1 = src0[j];
                dst0[i] = t1; dst0[j] = t0;
            }
        }
    }

    template class Point_<int>;
    template class Point_<short>;
    template class Point_<float>;
    template class Size_<int>;
    template class Size_<short>;
    template class Size_<float>;
    template class Rect_<int>;
    template class Rect_<short>;
    template class Rect_<float>;
}
