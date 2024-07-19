#include "cvimgproc.h"
#include <cmath>

namespace cv
{
    enum { XY_SHIFT = 16, XY_ONE = 1 << XY_SHIFT, DRAWING_STORAGE_BLOCK = (1<<12) - 256 };

    static const int MAX_THICKNESS = 32767;

    struct PolyEdge
    {
        PolyEdge() : y0(0), y1(0), x(0), dx(0), next(0) {}
        int y0, y1;
        int x, dx;
        PolyEdge *next;
    };

    class LineIterator
    {
    public:
        LineIterator(const Mat& img, Point pt1, Point pt2,
                    int connectivity = 8, bool leftToRight = false)
        {
            init(&img, Rect(0, 0, img.cols, img.rows), pt1, pt2, connectivity, leftToRight);
            ptmode = false;
        }
        LineIterator( Point pt1, Point pt2,
                    int connectivity = 8, bool leftToRight = false )
        {
            init(0, Rect(std::min(pt1.x, pt2.x),
                        std::min(pt1.y, pt2.y),
                        std::max(pt1.x, pt2.x) - std::min(pt1.x, pt2.x) + 1,
                        std::max(pt1.y, pt2.y) - std::min(pt1.y, pt2.y) + 1),
                pt1, pt2, connectivity, leftToRight);
            ptmode = true;
        }
        LineIterator( Size boundingAreaSize, Point pt1, Point pt2,
                    int connectivity = 8, bool leftToRight = false )
        {
            init(0, Rect(0, 0, boundingAreaSize.width, boundingAreaSize.height),
                pt1, pt2, connectivity, leftToRight);
            ptmode = true;
        }
        LineIterator( Rect boundingAreaRect, Point pt1, Point pt2,
                    int connectivity = 8, bool leftToRight = false )
        {
            init(0, boundingAreaRect, pt1, pt2, connectivity, leftToRight);
            ptmode = true;
        }
        void init(const Mat* img, Rect boundingAreaRect, Point pt1, Point pt2, int connectivity, bool leftToRight);

        /** @brief Returns pointer to the current pixel.
        */
        uint8_t* operator *();

        /** @brief Moves iterator to the next pixel on the line.

        This is the prefix version (++it).
        */
        LineIterator& operator ++();

        /** @brief Moves iterator to the next pixel on the line.

        This is the postfix version (it++).
        */
        LineIterator operator ++(int);

        /** @brief Returns coordinates of the current pixel.
        */
        Point pos() const;

        uint8_t* ptr;
        const uint8_t* ptr0;
        int step, elemSize;
        int err, count;
        int minusDelta, plusDelta;
        int minusStep, plusStep;
        int minusShift, plusShift;
        Point p;
        bool ptmode;
    };

    //! @cond IGNORED

    // === LineIterator implementation ===

    inline uint8_t* LineIterator::operator *()
    {
        return ptmode ? 0 : ptr;
    }

    inline LineIterator& LineIterator::operator ++()
    {
        int mask = err < 0 ? -1 : 0;
        err += minusDelta + (plusDelta & mask);
        if(!ptmode)
        {
            ptr += minusStep + (plusStep & mask);
        }
        else
        {
            p.x += minusShift + (plusShift & mask);
            p.y += minusStep + (plusStep & mask);
        }
        return *this;
    }

    inline LineIterator LineIterator::operator ++(int)
    {
        LineIterator it = *this;
        ++(*this);
        return it;
    }

    inline Point LineIterator::pos() const
    {
        if(!ptmode)
        {
            size_t offset = (size_t)(ptr - ptr0);
            int y = (int)(offset/step);
            int x = (int)((offset - (size_t)y*step)/elemSize);
            return Point(x, y);
        }
        return p;
    }

    static void CollectPolyEdges(Mat& img, const Point* v, int npts, std::vector<PolyEdge>& edges, const uint16_t color, int shift = 0);

    static void FillEdgeCollection(Mat& img, std::vector<PolyEdge>& edges, uint16_t color);

    static void PolyLine(Mat& img, const Point* v, int npts, bool closed, uint16_t color, int thickness, int shift = 0);

    static void FillConvexPoly(Mat& img, const Point* v, int npts, uint16_t color, int shift = 0);

    static inline void ICV_HLINE(uint8_t* ptr, int xl, int xr, uint16_t color, int pix_size)
    {
        uint8_t* hline_min_ptr = (uint8_t*)(ptr) + (xl)*(pix_size);
        uint8_t* hline_end_ptr = (uint8_t*)(ptr) + (xr+1)*(pix_size);
        uint8_t* hline_ptr = hline_min_ptr;
        if (pix_size == 1)
            memset(hline_min_ptr, color, hline_end_ptr-hline_min_ptr);
        else//if (pix_size != 1)
        {
            if (hline_min_ptr < hline_end_ptr)
            {
                memcpy(hline_ptr, &color, pix_size);
                hline_ptr += pix_size;
            }//end if (hline_min_ptr < hline_end_ptr)
            size_t sizeToCopy = pix_size;
            while(hline_ptr < hline_end_ptr)
            {
                memcpy(hline_ptr, hline_min_ptr, sizeToCopy);
                hline_ptr += sizeToCopy;
                sizeToCopy = std::min(2*sizeToCopy, static_cast<size_t>(hline_end_ptr-hline_ptr));
            }//end while(hline_ptr < hline_end_ptr)
        }//end if (pix_size != 1)
    }

    static void Line(Mat& img, Point pt1, Point pt2, uint16_t _color)
    {
        constexpr int connectivity = 8;
        LineIterator iterator(img, pt1, pt2, connectivity, true);
        int i, count = iterator.count;
        int pix_size = (int)img.elemSize();
        const uint8_t* color = (const uint8_t*)&_color;
        for( i = 0; i < count; i++, ++iterator )
        {
            uint8_t* ptr = *iterator;
            if(pix_size == 1)
                ptr[0] = color[0];
            else
                memcpy(*iterator, color, pix_size);
        }
    }

    static void CollectPolyEdges(Mat& img, const Point* v, int count, std::vector<PolyEdge>& edges,
                    uint16_t color, int shift)
    {
        int i, delta = (1 << shift) >> 1;
        Point pt0 = v[count-1], pt1;
        pt0.x = pt0.x << (XY_SHIFT - shift);
        pt0.y = (pt0.y + delta) >> shift;

        edges.reserve( edges.size() + count );

        for( i = 0; i < count; i++, pt0 = pt1 )
        {
            Point t0, t1;
            PolyEdge edge;

            pt1 = v[i];
            pt1.x = pt1.x << (XY_SHIFT - shift);
            pt1.y = (pt1.y + delta) >> shift;

            t0.y = pt0.y; t1.y = pt1.y;
            t0.x = (pt0.x + (XY_ONE >> 1)) >> XY_SHIFT;
            t1.x = (pt1.x + (XY_ONE >> 1)) >> XY_SHIFT;
            Line( img, t0, t1, color);

            if( pt0.y == pt1.y )
                continue;

            if( pt0.y < pt1.y )
            {
                edge.y0 = (int)(pt0.y);
                edge.y1 = (int)(pt1.y);
                edge.x = pt0.x;
            }
            else
            {
                edge.y0 = (int)(pt1.y);
                edge.y1 = (int)(pt0.y);
                edge.x = pt1.x;
            }
            edge.dx = (pt1.x - pt0.x) / (pt1.y - pt0.y);
            edges.push_back(edge);
        }
    }

    struct CmpEdges
    {
        bool operator ()(const PolyEdge& e1, const PolyEdge& e2)
        {
            return e1.y0 - e2.y0 ? e1.y0 < e2.y0 :
                e1.x - e2.x ? e1.x < e2.x : e1.dx < e2.dx;
        }
    };

    /**************** helper macros and functions for sequence/contour processing ***********/

    static void FillEdgeCollection(Mat& img, std::vector<PolyEdge>& edges, uint16_t color)
    {
        PolyEdge tmp;
        int i, y, total = (int)edges.size();
        Size size = img.size();
        PolyEdge* e;
        int y_max = INT_MIN, y_min = INT_MAX;
        int x_max = INT_MAX, x_min = INT_MIN;
        int pix_size = (int)img.elemSize();

        if( total < 2 )
            return;

        for( i = 0; i < total; i++ )
        {
            PolyEdge& e1 = edges[i];
            // Determine x-coordinate of the end of the edge.
            // (This is not necessary x-coordinate of any vertex in the array.)
            int x1 = e1.x + (e1.y1 - e1.y0) * e1.dx;
            y_min = std::min( y_min, e1.y0 );
            y_max = std::max( y_max, e1.y1 );
            x_min = std::min( x_min, e1.x );
            x_max = std::max( x_max, e1.x );
            x_min = std::min( x_min, x1 );
            x_max = std::max( x_max, x1 );
        }

        if( y_max < 0 || y_min >= size.height || x_max < 0 || x_min >= ((int)size.width<<XY_SHIFT) )
            return;

        std::sort( edges.begin(), edges.end(), CmpEdges() );

        // start drawing
        tmp.y0 = INT_MAX;
        edges.push_back(tmp); // after this point we do not add
                            // any elements to edges, thus we can use pointers
        i = 0;
        tmp.next = 0;
        e = &edges[i];
        y_max = std::min( y_max, size.height);

        for( y = e->y0; y < y_max; y++ )
        {
            PolyEdge *last, *prelast, *keep_prelast;
            int draw = 0;
            int clipline = y < 0;

            prelast = &tmp;
            last = tmp.next;
            while( last || e->y0 == y )
            {
                if( last && last->y1 == y )
                {
                    // exclude edge if y reaches its lower point
                    prelast->next = last->next;
                    last = last->next;
                    continue;
                }
                keep_prelast = prelast;
                if( last && (e->y0 > y || last->x < e->x) )
                {
                    // go to the next edge in active list
                    prelast = last;
                    last = last->next;
                }
                else if( i < total )
                {
                    // insert new edge into active list if y reaches its upper point
                    prelast->next = e;
                    e->next = last;
                    prelast = e;
                    e = &edges[++i];
                }
                else
                    break;

                if( draw )
                {
                    if( !clipline )
                    {
                        // convert x's from fixed-point to image coordinates
                        uint8_t *timg = img.ptr<uint8_t>(y);
                        int x1, x2;

                        if (keep_prelast->x > prelast->x)
                        {
                            x1 = (int)((prelast->x + XY_ONE - 1) >> XY_SHIFT);
                            x2 = (int)(keep_prelast->x >> XY_SHIFT);
                        }
                        else
                        {
                            x1 = (int)((keep_prelast->x + XY_ONE - 1) >> XY_SHIFT);
                            x2 = (int)(prelast->x >> XY_SHIFT);
                        }

                        // clip and draw the line
                        if( x1 < size.width && x2 >= 0 )
                        {
                            if( x1 < 0 )
                                x1 = 0;
                            if( x2 >= size.width )
                                x2 = size.width - 1;
                            ICV_HLINE( timg, x1, x2, color, pix_size );
                        }
                    }
                    keep_prelast->x += keep_prelast->dx;
                    prelast->x += prelast->dx;
                }
                draw ^= 1;
            }

            // sort edges (using bubble sort)
            keep_prelast = 0;

            do
            {
                prelast = &tmp;
                last = tmp.next;
                PolyEdge *last_exchange = 0;

                while( last != keep_prelast && last->next != 0 )
                {
                    PolyEdge *te = last->next;

                    // swap edges
                    if( last->x > te->x )
                    {
                        prelast->next = te;
                        last->next = te->next;
                        te->next = last;
                        prelast = te;
                        last_exchange = prelast;
                    }
                    else
                    {
                        prelast = last;
                        last = te;
                    }
                }
                if (last_exchange == NULL)
                    break;
                keep_prelast = last_exchange;
            } while( keep_prelast != tmp.next && keep_prelast != &tmp );
        }
    }

    static bool clipLine(Size img_size, Point& pt1, Point& pt2)
    {
        int c1, c2;
        int right = img_size.width-1, bottom = img_size.height-1;

        if( img_size.width <= 0 || img_size.height <= 0 )
            return false;

        int &x1 = pt1.x, &y1 = pt1.y, &x2 = pt2.x, &y2 = pt2.y;
        c1 = (x1 < 0) + (x1 > right) * 2 + (y1 < 0) * 4 + (y1 > bottom) * 8;
        c2 = (x2 < 0) + (x2 > right) * 2 + (y2 < 0) * 4 + (y2 > bottom) * 8;

        if( (c1 & c2) == 0 && (c1 | c2) != 0 )
        {
            int a;
            if( c1 & 12 )
            {
                a = c1 < 8 ? 0 : bottom;
                x1 += (int)((float)(a - y1) * (x2 - x1) / (y2 - y1));
                y1 = a;
                c1 = (x1 < 0) + (x1 > right) * 2;
            }
            if( c2 & 12 )
            {
                a = c2 < 8 ? 0 : bottom;
                x2 += (int)((float)(a - y2) * (x2 - x1) / (y2 - y1));
                y2 = a;
                c2 = (x2 < 0) + (x2 > right) * 2;
            }
            if( (c1 & c2) == 0 && (c1 | c2) != 0 )
            {
                if( c1 )
                {
                    a = c1 == 1 ? 0 : right;
                    y1 += (int)((float)(a - x1) * (y2 - y1) / (x2 - x1));
                    x1 = a;
                    c1 = 0;
                }
                if( c2 )
                {
                    a = c2 == 1 ? 0 : right;
                    y2 += (int)((float)(a - x2) * (y2 - y1) / (x2 - x1));
                    x2 = a;
                    c2 = 0;
                }
            }
        }
        return (c1 | c2) == 0;
    }

    void LineIterator::init( const Mat* img, Rect rect, Point pt1_, Point pt2_, int connectivity, bool leftToRight )
    {
        count = -1;
        p = Point(0, 0);
        ptr0 = ptr = 0;
        step = elemSize = 0;
        ptmode = !img;

        Point pt1 = pt1_ - rect.tl();
        Point pt2 = pt2_ - rect.tl();

        if( (unsigned)pt1.x >= (unsigned)(rect.width) ||
            (unsigned)pt2.x >= (unsigned)(rect.width) ||
            (unsigned)pt1.y >= (unsigned)(rect.height) ||
            (unsigned)pt2.y >= (unsigned)(rect.height) )
        {
            if( !clipLine(Size(rect.width, rect.height), pt1, pt2) )
            {
                err = plusDelta = minusDelta = plusStep = minusStep = plusShift = minusShift = count = 0;
                return;
            }
        }

        pt1 += rect.tl();
        pt2 += rect.tl();

        int delta_x = 1, delta_y = 1;
        int dx = pt2.x - pt1.x;
        int dy = pt2.y - pt1.y;

        if( dx < 0 )
        {
            if( leftToRight )
            {
                dx = -dx;
                dy = -dy;
                pt1 = pt2;
            }
            else
            {
                dx = -dx;
                delta_x = -1;
            }
        }

        if( dy < 0 )
        {
            dy = -dy;
            delta_y = -1;
        }

        bool vert = dy > dx;
        if( vert )
        {
            std::swap(dx, dy);
            std::swap(delta_x, delta_y);
        }

        if( connectivity == 8 )
        {
            err = dx - (dy + dy);
            plusDelta = dx + dx;
            minusDelta = -(dy + dy);
            minusShift = delta_x;
            plusShift = 0;
            minusStep = 0;
            plusStep = delta_y;
            count = dx + 1;
        }
        else /* connectivity == 4 */
        {
            err = 0;
            plusDelta = (dx + dx) + (dy + dy);
            minusDelta = -(dy + dy);
            minusShift = delta_x;
            plusShift = -delta_x;
            minusStep = 0;
            plusStep = delta_y;
            count = dx + dy + 1;
        }

        if( vert )
        {
            std::swap(plusStep, plusShift);
            std::swap(minusStep, minusShift);
        }

        p = pt1;
        if( !ptmode )
        {
            ptr0 = img->ptr<uint8_t>();
            step = (int)img->step[0];
            elemSize = (int)img->elemSize();
            ptr = (uint8_t*)ptr0 + (size_t)p.y*step + (size_t)p.x*elemSize;
            plusStep = plusStep*step + plusShift*elemSize;
            minusStep = minusStep*step + minusShift*elemSize;
        }
    }

    static void Line2(Mat& img, Point pt1, Point pt2, uint16_t color)
    {
        int64_t dx, dy;
        int ecount;
        int64_t ax, ay;
        int64_t i, j;
        int x, y;
        int x_step, y_step;
        int pix_size = (int)img.elemSize();
        uint8_t *ptr = img.ptr<uint8_t>(), *tptr;
        size_t step = img.step[0];
        Size size = img.size();

        Size sizeScaled(size.width << XY_SHIFT, size.height << XY_SHIFT);
        if( !clipLine( sizeScaled, pt1, pt2 ))
            return;

        dx = pt2.x - pt1.x;
        dy = pt2.y - pt1.y;

        j = dx < 0 ? -1 : 0;
        ax = (dx ^ j) - j;
        i = dy < 0 ? -1 : 0;
        ay = (dy ^ i) - i;

        if( ax > ay )
        {
            dy = (dy ^ j) - j;
            pt1.x ^= pt2.x & j;
            pt2.x ^= pt1.x & j;
            pt1.x ^= pt2.x & j;
            pt1.y ^= pt2.y & j;
            pt2.y ^= pt1.y & j;
            pt1.y ^= pt2.y & j;

            x_step = XY_ONE;
            y_step = dy * (1 << XY_SHIFT) / (ax | 1);
            ecount = (int)((pt2.x - pt1.x) >> XY_SHIFT);
        }
        else
        {
            dx = (dx ^ i) - i;
            pt1.x ^= pt2.x & i;
            pt2.x ^= pt1.x & i;
            pt1.x ^= pt2.x & i;
            pt1.y ^= pt2.y & i;
            pt2.y ^= pt1.y & i;
            pt1.y ^= pt2.y & i;

            x_step = dx * (1 << XY_SHIFT) / (ay | 1);
            y_step = XY_ONE;
            ecount = (int)((pt2.y - pt1.y) >> XY_SHIFT);
        }

        pt1.x += (XY_ONE >> 1);
        pt1.y += (XY_ONE >> 1);

        if( pix_size == 1 )
        {
            #define  ICV_PUT_POINT(_x,_y) \
            x = (_x); y = (_y);           \
            if( 0 <= x && x < size.width && \
                0 <= y && y < size.height ) \
            {                           \
                tptr = ptr + y*step + x;\
                tptr[0] = (uint8_t)color;    \
            }

            ICV_PUT_POINT((int)((pt2.x + (XY_ONE >> 1)) >> XY_SHIFT),
                        (int)((pt2.y + (XY_ONE >> 1)) >> XY_SHIFT));

            if( ax > ay )
            {
                pt1.x >>= XY_SHIFT;

                while( ecount >= 0 )
                {
                    ICV_PUT_POINT((int)(pt1.x), (int)(pt1.y >> XY_SHIFT));
                    pt1.x++;
                    pt1.y += y_step;
                    ecount--;
                }
            }
            else
            {
                pt1.y >>= XY_SHIFT;

                while( ecount >= 0 )
                {
                    ICV_PUT_POINT((int)(pt1.x >> XY_SHIFT), (int)(pt1.y));
                    pt1.x += x_step;
                    pt1.y++;
                    ecount--;
                }
            }

            #undef ICV_PUT_POINT
        }
        else
        {
            #define  ICV_PUT_POINT(_x,_y)   \
            x = (_x); y = (_y);             \
            if( 0 <= x && x < size.width && \
                0 <= y && y < size.height ) \
            {                               \
                tptr = ptr + y*step + x*pix_size;\
                for( j = 0; j < pix_size; j++ ) \
                    tptr[j] = ((uint8_t*)&color)[j]; \
            }

           ICV_PUT_POINT((int)((pt2.x + (XY_ONE >> 1)) >> XY_SHIFT), (int)((pt2.y + (XY_ONE >> 1)) >> XY_SHIFT));

            if( ax > ay )
            {
                pt1.x >>= XY_SHIFT;

                while( ecount >= 0 )
                {
                    ICV_PUT_POINT((int)(pt1.x), (int)(pt1.y >> XY_SHIFT));
                    pt1.x++;
                    pt1.y += y_step;
                    ecount--;
                }
            }
            else
            {
                pt1.y >>= XY_SHIFT;

                while( ecount >= 0 )
                {
                    ICV_PUT_POINT((int)(pt1.x >> XY_SHIFT), (int)(pt1.y));
                    pt1.x += x_step;
                    pt1.y++;
                    ecount--;
                }
            }

            #undef ICV_PUT_POINT
        }
    }

    /* draws simple or filled circle */
    static void Circle( Mat& img, Point center, int radius, uint16_t color, int fill )
    {
        Size size = img.size();
        size_t step = img.step[0];
        int pix_size = (int)img.elemSize();
        uint8_t* ptr = img.ptr<uint8_t>();
        int err = 0, dx = radius, dy = 0, plus = 1, minus = (radius << 1) - 1;
        int inside = center.x >= radius && center.x < size.width - radius &&
            center.y >= radius && center.y < size.height - radius;

        #define ICV_PUT_POINT( ptr, x )     \
            memcpy( ptr + (x)*pix_size, &color, pix_size );

        while( dx >= dy )
        {
            int mask;
            int y11 = center.y - dy, y12 = center.y + dy, y21 = center.y - dx, y22 = center.y + dx;
            int x11 = center.x - dx, x12 = center.x + dx, x21 = center.x - dy, x22 = center.x + dy;

            if( inside )
            {
                uint8_t *tptr0 = ptr + y11 * step;
                uint8_t *tptr1 = ptr + y12 * step;

                if( !fill )
                {
                    ICV_PUT_POINT( tptr0, x11 );
                    ICV_PUT_POINT( tptr1, x11 );
                    ICV_PUT_POINT( tptr0, x12 );
                    ICV_PUT_POINT( tptr1, x12 );
                }
                else
                {
                    ICV_HLINE( tptr0, x11, x12, color, pix_size );
                    ICV_HLINE( tptr1, x11, x12, color, pix_size );
                }

                tptr0 = ptr + y21 * step;
                tptr1 = ptr + y22 * step;

                if( !fill )
                {
                    ICV_PUT_POINT( tptr0, x21 );
                    ICV_PUT_POINT( tptr1, x21 );
                    ICV_PUT_POINT( tptr0, x22 );
                    ICV_PUT_POINT( tptr1, x22 );
                }
                else
                {
                    ICV_HLINE( tptr0, x21, x22, color, pix_size );
                    ICV_HLINE( tptr1, x21, x22, color, pix_size );
                }
            }
            else if( x11 < size.width && x12 >= 0 && y21 < size.height && y22 >= 0 )
            {
                if( fill )
                {
                    x11 = std::max( x11, (int)0 );
                    x12 = std::min( x12, size.width - 1 );
                }

                if( (unsigned)y11 < (unsigned)size.height )
                {
                    uint8_t *tptr = ptr + y11 * step;

                    if( !fill )
                    {
                        if( x11 >= 0 )
                            ICV_PUT_POINT( tptr, x11 );
                        if( x12 < size.width )
                            ICV_PUT_POINT( tptr, x12 );
                    }
                    else
                        ICV_HLINE( tptr, x11, x12, color, pix_size );
                }

                if( (unsigned)y12 < (unsigned)size.height )
                {
                    uint8_t *tptr = ptr + y12 * step;

                    if( !fill )
                    {
                        if( x11 >= 0 )
                            ICV_PUT_POINT( tptr, x11 );
                        if( x12 < size.width )
                            ICV_PUT_POINT( tptr, x12 );
                    }
                    else
                        ICV_HLINE( tptr, x11, x12, color, pix_size );
                }

                if( x21 < size.width && x22 >= 0 )
                {
                    if( fill )
                    {
                        x21 = std::max( x21, (int)0 );
                        x22 = std::min( x22, size.width - 1 );
                    }

                    if( (unsigned)y21 < (unsigned)size.height )
                    {
                        uint8_t *tptr = ptr + y21 * step;

                        if( !fill )
                        {
                            if( x21 >= 0 )
                                ICV_PUT_POINT( tptr, x21 );
                            if( x22 < size.width )
                                ICV_PUT_POINT( tptr, x22 );
                        }
                        else
                            ICV_HLINE( tptr, x21, x22, color, pix_size );
                    }

                    if( (unsigned)y22 < (unsigned)size.height )
                    {
                        uint8_t *tptr = ptr + y22 * step;

                        if( !fill )
                        {
                            if( x21 >= 0 )
                                ICV_PUT_POINT( tptr, x21 );
                            if( x22 < size.width )
                                ICV_PUT_POINT( tptr, x22 );
                        }
                        else
                            ICV_HLINE( tptr, x21, x22, color, pix_size );
                    }
                }
            }
            dy++;
            err += plus;
            plus += 2;

            mask = (err <= 0) - 1;

            err -= minus & mask;
            dx += mask;
            minus -= mask & 2;
        }

        #undef  ICV_PUT_POINT
    }

    static void ThickLine(Mat& img, Point p0, Point p1, uint16_t color,
            int thickness, int flags, int shift = 0)
    {
        static const float INV_XY_ONE = 1.f/XY_ONE;

        p0.x <<= XY_SHIFT - shift;
        p0.y <<= XY_SHIFT - shift;
        p1.x <<= XY_SHIFT - shift;
        p1.y <<= XY_SHIFT - shift;

        if( thickness <= 1 )
        {
            if(shift == 0 )
            {
                p0.x = (p0.x + (XY_ONE>>1)) >> XY_SHIFT;
                p0.y = (p0.y + (XY_ONE>>1)) >> XY_SHIFT;
                p1.x = (p1.x + (XY_ONE>>1)) >> XY_SHIFT;
                p1.y = (p1.y + (XY_ONE>>1)) >> XY_SHIFT;
                Line(img, p0, p1, color);
            }
            else
                Line2(img, p0, p1, color);
        }
        else
        {
            Point pt[4], dp = Point(0,0);
            float dx = (p0.x - p1.x)*INV_XY_ONE, dy = (p1.y - p0.y)*INV_XY_ONE;
            float r = dx * dx + dy * dy;
            int i, oddThickness = thickness & 1;
            thickness <<= XY_SHIFT - 1;

            if(fabs(r) > 1e-5)
            {
                r = (thickness + oddThickness*XY_ONE*0.5f)/std::sqrt(r);
                dp.x = cvRound( dy * r );
                dp.y = cvRound( dx * r );

                pt[0].x = p0.x + dp.x;
                pt[0].y = p0.y + dp.y;
                pt[1].x = p0.x - dp.x;
                pt[1].y = p0.y - dp.y;
                pt[2].x = p1.x - dp.x;
                pt[2].y = p1.y - dp.y;
                pt[3].x = p1.x + dp.x;
                pt[3].y = p1.y + dp.y;

                FillConvexPoly( img, pt, 4, color, XY_SHIFT );
            }

            for( i = 0; i < 2; i++ )
            {
                if( flags & (i+1) )
                {
                    Point center;
                    center.x = (int)((p0.x + (XY_ONE>>1)) >> XY_SHIFT);
                    center.y = (int)((p0.y + (XY_ONE>>1)) >> XY_SHIFT);
                    Circle( img, center, (thickness + (XY_ONE>>1)) >> XY_SHIFT, color, 1 );
                }
                p0 = p1;
            }
        }
    }

    static void FillConvexPoly(Mat& img, const Point* v, int npts, uint16_t color, int shift)
    {
        struct
        {
            int idx, di;
            int x, dx;
            int ye;
        }
        edge[2];

        int delta = 1 << shift >> 1;
        int i, y, imin = 0;
        int edges = npts;
        int xmin, xmax, ymin, ymax;
        uint8_t* ptr = img.ptr<uint8_t>();
        Size size = img.size();
        int pix_size = (int)img.elemSize();
        Point p0;
        int delta1, delta2;
        delta1 = delta2 = XY_ONE >> 1;

        p0 = v[npts - 1];
        p0.x <<= XY_SHIFT - shift;
        p0.y <<= XY_SHIFT - shift;

        xmin = xmax = v[0].x;
        ymin = ymax = v[0].y;

        for( i = 0; i < npts; i++ )
        {
            Point p = v[i];
            if( p.y < ymin )
            {
                ymin = p.y;
                imin = i;
            }

            ymax = std::max( ymax, p.y );
            xmax = std::max( xmax, p.x );
            xmin = std::min( xmin, p.x );

            p.x <<= XY_SHIFT - shift;
            p.y <<= XY_SHIFT - shift;

            if(shift == 0)
            {
                Point pt0, pt1;
                pt0.x = (int)(p0.x >> XY_SHIFT);
                pt0.y = (int)(p0.y >> XY_SHIFT);
                pt1.x = (int)(p.x >> XY_SHIFT);
                pt1.y = (int)(p.y >> XY_SHIFT);
                Line( img, pt0, pt1, color);
            }
            else
                Line2(img, p0, p, color);

            p0 = p;
        }

        xmin = (xmin + delta) >> shift;
        xmax = (xmax + delta) >> shift;
        ymin = (ymin + delta) >> shift;
        ymax = (ymax + delta) >> shift;

        if( npts < 3 || (int)xmax < 0 || (int)ymax < 0 || (int)xmin >= size.width || (int)ymin >= size.height )
            return;

        ymax = std::min( ymax, size.height - 1 );
        edge[0].idx = edge[1].idx = imin;

        edge[0].ye = edge[1].ye = y = (int)ymin;
        edge[0].di = 1;
        edge[1].di = npts - 1;

        edge[0].x = edge[1].x = -XY_ONE;
        edge[0].dx = edge[1].dx = 0;

        ptr += (int)img.step[0]*y;

        do
        {
            for( i = 0; i < 2; i++ )
            {
                if( y >= edge[i].ye )
                {
                    int idx0 = edge[i].idx, di = edge[i].di;
                    int idx = idx0 + di;
                    if (idx >= npts) idx -= npts;
                    int ty = 0;

                    for (; edges-- > 0; )
                    {
                        ty = (int)((v[idx].y + delta) >> shift);
                        if (ty > y)
                        {
                            int xs = v[idx0].x;
                            int xe = v[idx].x;
                            if (shift != XY_SHIFT)
                            {
                                xs <<= XY_SHIFT - shift;
                                xe <<= XY_SHIFT - shift;
                            }

                            edge[i].ye = ty;
                            edge[i].dx = ((xe - xs)*2 + (ty - y)) / (2 * (ty - y));
                            edge[i].x = xs;
                            edge[i].idx = idx;
                            break;
                        }
                        idx0 = idx;
                        idx += di;
                        if (idx >= npts) idx -= npts;
                    }
                }
            }

            if (edges < 0)
                break;

            if (y >= 0)
            {
                int left = 0, right = 1;
                if (edge[0].x > edge[1].x)
                {
                    left = 1, right = 0;
                }

                int xx1 = (int)((edge[left].x + delta1) >> XY_SHIFT);
                int xx2 = (int)((edge[right].x + delta2) >> XY_SHIFT);

                if( xx2 >= 0 && xx1 < size.width )
                {
                    if( xx1 < 0 )
                        xx1 = 0;
                    if( xx2 >= size.width )
                        xx2 = size.width - 1;
                    ICV_HLINE(ptr, xx1, xx2, color, pix_size);
                }
            }
            else
            {
                // TODO optimize scan for negative y
            }

            edge[0].x += edge[0].dx;
            edge[1].x += edge[1].dx;
            ptr += img.step[0];
        }
        while( ++y <= (int)ymax );
    }

        static const float SinTable[] =
        { 0.0000000f, 0.0174524f, 0.0348995f, 0.0523360f, 0.0697565f, 0.0871557f,
        0.1045285f, 0.1218693f, 0.1391731f, 0.1564345f, 0.1736482f, 0.1908090f,
        0.2079117f, 0.2249511f, 0.2419219f, 0.2588190f, 0.2756374f, 0.2923717f,
        0.3090170f, 0.3255682f, 0.3420201f, 0.3583679f, 0.3746066f, 0.3907311f,
        0.4067366f, 0.4226183f, 0.4383711f, 0.4539905f, 0.4694716f, 0.4848096f,
        0.5000000f, 0.5150381f, 0.5299193f, 0.5446390f, 0.5591929f, 0.5735764f,
        0.5877853f, 0.6018150f, 0.6156615f, 0.6293204f, 0.6427876f, 0.6560590f,
        0.6691306f, 0.6819984f, 0.6946584f, 0.7071068f, 0.7193398f, 0.7313537f,
        0.7431448f, 0.7547096f, 0.7660444f, 0.7771460f, 0.7880108f, 0.7986355f,
        0.8090170f, 0.8191520f, 0.8290376f, 0.8386706f, 0.8480481f, 0.8571673f,
        0.8660254f, 0.8746197f, 0.8829476f, 0.8910065f, 0.8987940f, 0.9063078f,
        0.9135455f, 0.9205049f, 0.9271839f, 0.9335804f, 0.9396926f, 0.9455186f,
        0.9510565f, 0.9563048f, 0.9612617f, 0.9659258f, 0.9702957f, 0.9743701f,
        0.9781476f, 0.9816272f, 0.9848078f, 0.9876883f, 0.9902681f, 0.9925462f,
        0.9945219f, 0.9961947f, 0.9975641f, 0.9986295f, 0.9993908f, 0.9998477f,
        1.0000000f, 0.9998477f, 0.9993908f, 0.9986295f, 0.9975641f, 0.9961947f,
        0.9945219f, 0.9925462f, 0.9902681f, 0.9876883f, 0.9848078f, 0.9816272f,
        0.9781476f, 0.9743701f, 0.9702957f, 0.9659258f, 0.9612617f, 0.9563048f,
        0.9510565f, 0.9455186f, 0.9396926f, 0.9335804f, 0.9271839f, 0.9205049f,
        0.9135455f, 0.9063078f, 0.8987940f, 0.8910065f, 0.8829476f, 0.8746197f,
        0.8660254f, 0.8571673f, 0.8480481f, 0.8386706f, 0.8290376f, 0.8191520f,
        0.8090170f, 0.7986355f, 0.7880108f, 0.7771460f, 0.7660444f, 0.7547096f,
        0.7431448f, 0.7313537f, 0.7193398f, 0.7071068f, 0.6946584f, 0.6819984f,
        0.6691306f, 0.6560590f, 0.6427876f, 0.6293204f, 0.6156615f, 0.6018150f,
        0.5877853f, 0.5735764f, 0.5591929f, 0.5446390f, 0.5299193f, 0.5150381f,
        0.5000000f, 0.4848096f, 0.4694716f, 0.4539905f, 0.4383711f, 0.4226183f,
        0.4067366f, 0.3907311f, 0.3746066f, 0.3583679f, 0.3420201f, 0.3255682f,
        0.3090170f, 0.2923717f, 0.2756374f, 0.2588190f, 0.2419219f, 0.2249511f,
        0.2079117f, 0.1908090f, 0.1736482f, 0.1564345f, 0.1391731f, 0.1218693f,
        0.1045285f, 0.0871557f, 0.0697565f, 0.0523360f, 0.0348995f, 0.0174524f,
        0.0000000f, -0.0174524f, -0.0348995f, -0.0523360f, -0.0697565f, -0.0871557f,
        -0.1045285f, -0.1218693f, -0.1391731f, -0.1564345f, -0.1736482f, -0.1908090f,
        -0.2079117f, -0.2249511f, -0.2419219f, -0.2588190f, -0.2756374f, -0.2923717f,
        -0.3090170f, -0.3255682f, -0.3420201f, -0.3583679f, -0.3746066f, -0.3907311f,
        -0.4067366f, -0.4226183f, -0.4383711f, -0.4539905f, -0.4694716f, -0.4848096f,
        -0.5000000f, -0.5150381f, -0.5299193f, -0.5446390f, -0.5591929f, -0.5735764f,
        -0.5877853f, -0.6018150f, -0.6156615f, -0.6293204f, -0.6427876f, -0.6560590f,
        -0.6691306f, -0.6819984f, -0.6946584f, -0.7071068f, -0.7193398f, -0.7313537f,
        -0.7431448f, -0.7547096f, -0.7660444f, -0.7771460f, -0.7880108f, -0.7986355f,
        -0.8090170f, -0.8191520f, -0.8290376f, -0.8386706f, -0.8480481f, -0.8571673f,
        -0.8660254f, -0.8746197f, -0.8829476f, -0.8910065f, -0.8987940f, -0.9063078f,
        -0.9135455f, -0.9205049f, -0.9271839f, -0.9335804f, -0.9396926f, -0.9455186f,
        -0.9510565f, -0.9563048f, -0.9612617f, -0.9659258f, -0.9702957f, -0.9743701f,
        -0.9781476f, -0.9816272f, -0.9848078f, -0.9876883f, -0.9902681f, -0.9925462f,
        -0.9945219f, -0.9961947f, -0.9975641f, -0.9986295f, -0.9993908f, -0.9998477f,
        -1.0000000f, -0.9998477f, -0.9993908f, -0.9986295f, -0.9975641f, -0.9961947f,
        -0.9945219f, -0.9925462f, -0.9902681f, -0.9876883f, -0.9848078f, -0.9816272f,
        -0.9781476f, -0.9743701f, -0.9702957f, -0.9659258f, -0.9612617f, -0.9563048f,
        -0.9510565f, -0.9455186f, -0.9396926f, -0.9335804f, -0.9271839f, -0.9205049f,
        -0.9135455f, -0.9063078f, -0.8987940f, -0.8910065f, -0.8829476f, -0.8746197f,
        -0.8660254f, -0.8571673f, -0.8480481f, -0.8386706f, -0.8290376f, -0.8191520f,
        -0.8090170f, -0.7986355f, -0.7880108f, -0.7771460f, -0.7660444f, -0.7547096f,
        -0.7431448f, -0.7313537f, -0.7193398f, -0.7071068f, -0.6946584f, -0.6819984f,
        -0.6691306f, -0.6560590f, -0.6427876f, -0.6293204f, -0.6156615f, -0.6018150f,
        -0.5877853f, -0.5735764f, -0.5591929f, -0.5446390f, -0.5299193f, -0.5150381f,
        -0.5000000f, -0.4848096f, -0.4694716f, -0.4539905f, -0.4383711f, -0.4226183f,
        -0.4067366f, -0.3907311f, -0.3746066f, -0.3583679f, -0.3420201f, -0.3255682f,
        -0.3090170f, -0.2923717f, -0.2756374f, -0.2588190f, -0.2419219f, -0.2249511f,
        -0.2079117f, -0.1908090f, -0.1736482f, -0.1564345f, -0.1391731f, -0.1218693f,
        -0.1045285f, -0.0871557f, -0.0697565f, -0.0523360f, -0.0348995f, -0.0174524f,
        -0.0000000f, 0.0174524f, 0.0348995f, 0.0523360f, 0.0697565f, 0.0871557f,
        0.1045285f, 0.1218693f, 0.1391731f, 0.1564345f, 0.1736482f, 0.1908090f,
        0.2079117f, 0.2249511f, 0.2419219f, 0.2588190f, 0.2756374f, 0.2923717f,
        0.3090170f, 0.3255682f, 0.3420201f, 0.3583679f, 0.3746066f, 0.3907311f,
        0.4067366f, 0.4226183f, 0.4383711f, 0.4539905f, 0.4694716f, 0.4848096f,
        0.5000000f, 0.5150381f, 0.5299193f, 0.5446390f, 0.5591929f, 0.5735764f,
        0.5877853f, 0.6018150f, 0.6156615f, 0.6293204f, 0.6427876f, 0.6560590f,
        0.6691306f, 0.6819984f, 0.6946584f, 0.7071068f, 0.7193398f, 0.7313537f,
        0.7431448f, 0.7547096f, 0.7660444f, 0.7771460f, 0.7880108f, 0.7986355f,
        0.8090170f, 0.8191520f, 0.8290376f, 0.8386706f, 0.8480481f, 0.8571673f,
        0.8660254f, 0.8746197f, 0.8829476f, 0.8910065f, 0.8987940f, 0.9063078f,
        0.9135455f, 0.9205049f, 0.9271839f, 0.9335804f, 0.9396926f, 0.9455186f,
        0.9510565f, 0.9563048f, 0.9612617f, 0.9659258f, 0.9702957f, 0.9743701f,
        0.9781476f, 0.9816272f, 0.9848078f, 0.9876883f, 0.9902681f, 0.9925462f,
        0.9945219f, 0.9961947f, 0.9975641f, 0.9986295f, 0.9993908f, 0.9998477f,
        1.0000000f
    };

    static void sincos(int angle, float& cosval, float& sinval)
    {
        angle += (angle < 0 ? 360 : 0);
        sinval = SinTable[angle];
        cosval = SinTable[450 - angle];
    }

    void ellipse2Poly( Point2f center, Size2f axes, int angle,
                   int arc_start, int arc_end,
                   int delta, std::vector<Point2f>& pts )
    {
        float alpha, beta;
        int i;

        while( angle < 0 )
            angle += 360;
        while( angle > 360 )
            angle -= 360;

        if( arc_start > arc_end )
        {
            i = arc_start;
            arc_start = arc_end;
            arc_end = i;
        }
        while( arc_start < 0 )
        {
            arc_start += 360;
            arc_end += 360;
        }
        while( arc_end > 360 )
        {
            arc_end -= 360;
            arc_start -= 360;
        }
        if( arc_end - arc_start > 360 )
        {
            arc_start = 0;
            arc_end = 360;
        }
        sincos(angle, alpha, beta);
        pts.resize(0);

        for( i = arc_start; i < arc_end + delta; i += delta )
        {
            float x, y;
            angle = i;
            if( angle > arc_end )
                angle = arc_end;
            if( angle < 0 )
                angle += 360;

            x = axes.width * SinTable[450-angle];
            y = axes.height * SinTable[angle];
            Point2f pt;
            pt.x = center.x + x * alpha - y * beta;
            pt.y = center.y + x * beta + y * alpha;
            pts.push_back(pt);
        }

        // If there are no points, it's a zero-size polygon
        if( pts.size() == 1) {
            pts.assign(2,center);
        }
    }

    static void EllipseEx(Mat& img, Point center, Size axes,
            int angle, int arc_start, int arc_end, uint16_t color, int thickness)
    {
        axes.width = std::abs(axes.width), axes.height = std::abs(axes.height);
        int delta = (int)((std::max(axes.width,axes.height)+(XY_ONE>>1))>>XY_SHIFT);
        delta = delta < 3 ? 90 : delta < 10 ? 30 : delta < 15 ? 18 : 5;

        std::vector<Point2f> _v;
        ellipse2Poly(Point2f((float)center.x, (float)center.y), Size2f((float)axes.width, (float)axes.height), angle, arc_start, arc_end, delta, _v );

        std::vector<Point> v;
        Point prevPt(INT_MAX, INT_MAX);
        v.resize(0);
        for (unsigned int i = 0; i < _v.size(); ++i)
        {
            Point pt;
            pt.x = cvRound(_v[i].x / XY_ONE) << XY_SHIFT;
            pt.y = cvRound(_v[i].y / XY_ONE) << XY_SHIFT;
            pt.x += cvRound(_v[i].x - pt.x);
            pt.y += cvRound(_v[i].y - pt.y);
            if (pt != prevPt) {
                v.push_back(pt);
                prevPt = pt;
            }
        }

        // If there are no points, it's a zero-size polygon
        if (v.size() == 1) {
            v.assign(2, center);
        }

        if( thickness >= 0 )
            PolyLine( img, &v[0], (int)v.size(), false, color, thickness, XY_SHIFT);
        else if( arc_end - arc_start >= 360 )
            FillConvexPoly( img, &v[0], (int)v.size(), color, XY_SHIFT);
        else
        {
            v.push_back(center);
            std::vector<PolyEdge> edges;
            CollectPolyEdges(img,  &v[0], (int)v.size(), edges, color, XY_SHIFT);
            FillEdgeCollection(img, edges, color);
        }
    }

    static void ellipse(Mat& img, Point center, Size axes, float angle, float startAngle, float endAngle, uint16_t color, int thickness)
    {
        int _angle = cvRound(angle);
        int _start_angle = cvRound(startAngle);
        int _end_angle = cvRound(endAngle);
        center.x <<= XY_SHIFT;
        center.y <<= XY_SHIFT;
        axes.width <<= XY_SHIFT;
        axes.height <<= XY_SHIFT;
        EllipseEx(img, center, axes, _angle, _start_angle, _end_angle, color, thickness);
    }

    static void ellipse(Mat& img, const RotatedRect& box, uint16_t color, int thickness)
    {
        int _angle = cvRound(box.angle);
        Point center(cvRound(box.center.x), cvRound(box.center.y));
        center.x = (center.x << XY_SHIFT) + cvRound((box.center.x - center.x)*XY_ONE);
        center.y = (center.y << XY_SHIFT) + cvRound((box.center.y - center.y)*XY_ONE);
        Size axes(cvRound(box.size.width), cvRound(box.size.height));
        axes.width  = (axes.width  << (XY_SHIFT - 1)) + cvRound((box.size.width - axes.width)*(XY_ONE>>1));
        axes.height = (axes.height << (XY_SHIFT - 1)) + cvRound((box.size.height - axes.height)*(XY_ONE>>1));
        EllipseEx(img, center, axes, _angle, 0, 360, color, thickness);
    }

    static void PolyLine(Mat& img, const Point* v, int count, bool is_closed,
          uint16_t color, int thickness, int shift)
    {
        if( !v || count <= 0 )
            return;

        int i = is_closed ? count - 1 : 0;
        int flags = 2 + !is_closed;
        Point p0;
        p0 = v[i];
        for( i = !is_closed; i < count; i++ )
        {
            Point p = v[i];
            ThickLine( img, p0, p, color, thickness, flags, shift);
            p0 = p;
            flags = 2;
        }
    }

    static void polyline(Mat& img, const std::vector<Point>& contour, uint16_t color, int thickness)
    {
        PolyLine(img, &contour[0], int(contour.size()), false, color, thickness, 0);
    }

     Painter::Painter(const Mat& _mat)
        : mat(_mat), default_dirty_rect(0, 0, _mat.cols, _mat.rows)
    {
    }

    void Painter::fill(uint16_t color)
    {
#if USE_DMA2D && defined(DMA2D)
        dma2d_fill(mat, color);
#else
        mat = color;
#endif
#if USE_DIRTY_RECT
        update_dirty_rect(Rect(0, 0, mat.cols, mat.rows));
#endif
    }

    void Painter::rectangle(Point pt1, Point pt2, uint16_t color, int thickness)
    {
        if(thickness >= 0)
        {
            Point pt[4];
            pt[0] = pt1;
            pt[1].x = pt2.x;
            pt[1].y = pt1.y;
            pt[2] = pt2;
            pt[3].x = pt1.x;
            pt[3].y = pt2.y;
            PolyLine(mat, pt, 4, true, color, thickness);
        }
        else
        {
#if USE_DMA2D && defined(DMA2D)
        dma2d_fill(mat(Rect(pt1, pt2)), color);
#else
        Point pt[4];
        pt[0] = pt1;
        pt[1].x = pt2.x;
        pt[1].y = pt1.y;
        pt[2] = pt2;
        pt[3].x = pt1.x;
        pt[3].y = pt2.y;
        FillConvexPoly(mat, pt, 4, color);
#endif
        }
#if USE_DIRTY_RECT
        Rect current_dirty_rect(pt1, pt2);
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::line(Point pt1, Point pt2, uint16_t color, int thickness)
    {
        ThickLine(mat, pt1, pt2, color, thickness, 3);
#if USE_DIRTY_RECT
        Rect current_dirty_rect(pt1, pt2);
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::circle(Point center, int radius, uint16_t color, int thickness)
    {
        if(thickness > 1)
        {
            Point _center(center);
            int _radius(radius);
            _center.x <<= XY_SHIFT;
            _center.y <<= XY_SHIFT;
            _radius <<= XY_SHIFT;
            EllipseEx(mat, _center, Size(_radius, _radius), 0, 0, 360, color, thickness);
        }
        else
            Circle(mat, center, radius, color, thickness < 0);
#if USE_DIRTY_RECT
        Rect current_dirty_rect(center.x - radius, center.y - radius, radius * 2, radius * 2);
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::polyline(const std::vector<Point>& contour, uint16_t color, int thickness)
    {
        ::cv::polyline(mat, contour, color, thickness);
#if USE_DIRTY_RECT
        Rect current_dirty_rect = boundingRect(contour);
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::ellipse(Point center, Size axes, float angle, float startAngle, float endAngle, uint16_t color, int thickness)
    {
        ::cv::ellipse(mat, center, axes, angle, startAngle, endAngle, color, thickness);
#if USE_DIRTY_RECT
        Rect current_dirty_rect(center.x - axes.width, center.y - axes.height, axes.width * 2 + 1, axes.height * 2 + 1);
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::ellipse(const RotatedRect& box, uint16_t color, int thickness)
    {
        ::cv::ellipse(mat, box, color, thickness);
#if USE_DIRTY_RECT
        Rect current_dirty_rect = box.boundingRect();
        if(thickness > 0)
        {
            current_dirty_rect.x -= thickness / 2;
            current_dirty_rect.y -= thickness / 2;
            current_dirty_rect.width += thickness;
            current_dirty_rect.height += thickness;
        }
        update_dirty_rect(current_dirty_rect);
#endif
    }

    void Painter::putText(std::string_view text, Point org, FontBase& font, uint16_t text_color, uint16_t bg_color, int wrap_width, size_t *consumed_chars)
    {
        Rect text_rect(org.x, org.y, mat.cols - org.x, mat.rows - org.y);
        Mat subMat(mat, text_rect);
        get_text_bitmap_result_t rc = font.get_text_bitmap(text, subMat, text_color, bg_color, wrap_width);
        if(consumed_chars != nullptr)
        {
            *consumed_chars = rc.consumed_chars;
        }
#if USE_DIRTY_RECT
        text_rect.width = rc.text_size.width;
        text_rect.height = rc.text_size.height;
        update_dirty_rect(text_rect);
#endif
    }

    void Painter::putText(std::wstring_view text, Point org, UnicodeFont& font, uint16_t text_color, uint16_t bg_color, int wrap_width, size_t *consumed_chars)
    {
        putText(std::string_view(reinterpret_cast<const char*>(text.data()), text.length() * 2), org, font, text_color, bg_color, wrap_width, consumed_chars);
    }

    void Painter::drawBitmap(const Mat& bitmap, Point org)
    {
        if(bitmap.type != mat.type)
        {
            return;
        }
        Rect target_rect(org.x, org.y, bitmap.cols, bitmap.rows);
        target_rect &= Rect(0, 0, mat.cols, mat.rows);
        Rect src_rect(0, 0, target_rect.width, target_rect.height);
        if(org.x < 0) src_rect.x = -org.x;
        if(org.y < 0) src_rect.y = -org.y;
        if(!target_rect.empty())
        {
#if USE_DMA2D && defined(DMA2D)
            dma2d_copy(bitmap, src_rect, mat, target_rect.tl());
#else
            switch(bitmap.type)
            {
            case MONO8:
                for(int row = 0; row < src_rect.height; row++)
                {
                    const uint8_t *p_src = bitmap.ptr<uint8_t>(src_rect.y + row, src_rect.x);
                    uint8_t *p_target = mat.ptr<uint8_t>(target_rect.y + row, target_rect.x);
                    std::copy_n(p_src, src_rect.width, p_target);
                }
                break;
            case RGB565:
                for(int row = 0; row < src_rect.height; row++)
                {
                    const uint16_t *p_src = bitmap.ptr<uint16_t>(src_rect.y + row, src_rect.x);
                    uint16_t *p_target = mat.ptr<uint16_t>(target_rect.y + row, target_rect.x);
                    std::copy_n(p_src, src_rect.width, p_target);
                }
                break;
            }
#endif
#if USE_DIRTY_RECT
            update_dirty_rect(target_rect);
#endif
        }
    }

    void Painter::drawBitmapWithAlpha(const Mat& bitmap, Point org)
    {
        if(bitmap.type != mat.type || bitmap.type != cv::ARGB1555)
        {
            return;
        }
        Rect target_rect(org.x, org.y, bitmap.cols, bitmap.rows);
        target_rect &= Rect(0, 0, mat.cols, mat.rows);
        Rect src_rect(0, 0, target_rect.width, target_rect.height);
        if(org.x < 0) src_rect.x = -org.x;
        if(org.y < 0) src_rect.y = -org.y;
        if(!target_rect.empty())
        {
#if USE_DMA2D && defined(DMA2D)
            dma2d_blend_argb1555_to_rgb565(mat, Rect(org.x, org.y, bitmap.cols, bitmap.rows), bitmap, cv::Point(0, 0), mat, org);
#else
            for(int rel_row = 0; rel_row < bitmap.rows; rel_row++)
            {
                const uint16_t *p_bitmap = bitmap.ptr<uint16_t>(rel_row);
                uint16_t *p_target = mat.ptr<uint16_t>(rel_row + org.y) + org.x;
                for(int rel_col = 0; rel_col < bitmap.cols; rel_col++)
                {
                    uint16_t bitmap_val = p_bitmap[rel_col];
                    if(bitmap_val & 0x8000)
                    {
                        // ARGB1555 to RGB565;
                        p_target[rel_col] = ((bitmap_val & 0x7FE0) << 1) | (bitmap_val & 0x1F);
                    }
                }
            }
#endif
        }
#if USE_DIRTY_RECT
        update_dirty_rect(target_rect);
#endif
    }

    void Painter::drawMarker(Point position, uint16_t color, int markerType, int markerSize, int thickness)
    {
        switch(markerType)
        {
        // The cross marker case
        case MARKER_CROSS:
            line(Point(position.x-(markerSize/2), position.y), Point(position.x+(markerSize/2), position.y), color, thickness);
            line(Point(position.x, position.y-(markerSize/2)), Point(position.x, position.y+(markerSize/2)), color, thickness);
            break;

        // The tilted cross marker case
        case MARKER_TILTED_CROSS:
            line(Point(position.x-(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y+(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y-(markerSize/2)), Point(position.x-(markerSize/2), position.y+(markerSize/2)), color, thickness);
            break;

        // The star marker case
        case MARKER_STAR:
            line(Point(position.x-(markerSize/2), position.y), Point(position.x+(markerSize/2), position.y), color, thickness);
            line(Point(position.x, position.y-(markerSize/2)), Point(position.x, position.y+(markerSize/2)), color, thickness);
            line(Point(position.x-(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y+(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y-(markerSize/2)), Point(position.x-(markerSize/2), position.y+(markerSize/2)), color, thickness);
            break;

        // The diamond marker case
        case MARKER_DIAMOND:
            line(Point(position.x, position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y), color, thickness);
            line(Point(position.x+(markerSize/2), position.y), Point(position.x, position.y+(markerSize/2)), color, thickness);
            line(Point(position.x, position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y), color, thickness);
            line(Point(position.x-(markerSize/2), position.y), Point(position.x, position.y-(markerSize/2)), color, thickness);
            break;

        // The square marker case
        case MARKER_SQUARE:
            line(Point(position.x-(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y-(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y+(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y+(markerSize/2)), color, thickness);
            line(Point(position.x-(markerSize/2), position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y-(markerSize/2)), color, thickness);
            break;

        // The triangle up marker case
        case MARKER_TRIANGLE_UP:
            line(Point(position.x-(markerSize/2), position.y+(markerSize/2)), Point(position.x+(markerSize/2), position.y+(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y+(markerSize/2)), Point(position.x, position.y-(markerSize/2)), color, thickness);
            line(Point(position.x, position.y-(markerSize/2)), Point(position.x-(markerSize/2), position.y+(markerSize/2)), color, thickness);
            break;

        // The triangle down marker case
        case MARKER_TRIANGLE_DOWN:
            line(Point(position.x-(markerSize/2), position.y-(markerSize/2)), Point(position.x+(markerSize/2), position.y-(markerSize/2)), color, thickness);
            line(Point(position.x+(markerSize/2), position.y-(markerSize/2)), Point(position.x, position.y+(markerSize/2)), color, thickness);
            line(Point(position.x, position.y+(markerSize/2)), Point(position.x-(markerSize/2), position.y-(markerSize/2)), color, thickness);
            break;

        // The single point case
        default:
            int pix_size = (int)mat.elemSize();
            const uint8_t* color_ = (const uint8_t*)&color;
            uint8_t* p_row = mat.ptr<uint8_t>(position.y);
            if(pix_size == 1)
            {
                p_row[position.x] = color_[0];
            }
            else
            {
                memcpy(p_row + position.x * pix_size, color_, pix_size);
            }
#if USE_DIRTY_RECT
            update_dirty_rect(Rect(position.x, position.y, 1, 1));
#endif
            break;
        }
    }

    span<const Rect> Painter::get_dirty_rects() const
    {
#if USE_DIRTY_RECT
        if(dirty_rect_count >= 0)
        {
            return span<const Rect>(dirty_rect_list, dirty_rect_count);
        }
#endif
        return span<const Rect>(&default_dirty_rect, 1);
    }

    void Painter::reset_dirty_rects()
    {
#if USE_DIRTY_RECT
        dirty_rect_count = 0;
        dirty_rect_area = 0;
#endif
    }

#if USE_DIRTY_RECT
    void Painter::update_dirty_rect(Rect rc)
    {
        if(dirty_rect_count < 0 || rc.empty())
        {
            return;
        }
        if(dirty_rect_count == 0)
        {
            dirty_rect_list[0] = rc;
            dirty_rect_count++;
            dirty_rect_area += rc.width * rc.height;
            return;
        }
        constexpr float sum_union_rate_thresh = 0.5f;
        cv::Rect union_rects[max_dirty_rect_count];
        float sum_union_rates[max_dirty_rect_count];
        int area_increments[max_dirty_rect_count];
        int best_match_index = -1;
        int rc_area = rc.area();
        for(int i = 0; i < dirty_rect_count; i++)
        {
            union_rects[i] = rc | dirty_rect_list[i];
            int union_area = union_rects[i].area();
            int current_area = dirty_rect_list[i].area();
            int sum_area = rc_area + current_area;
            sum_union_rates[i] = float(sum_area) / float(union_area);
            area_increments[i] = union_area - current_area;
            if(best_match_index < 0 || (sum_union_rates[i] > sum_union_rate_thresh && area_increments[i] < area_increments[best_match_index]))
            {
                best_match_index = i;
            }
        }
        if(sum_union_rates[best_match_index] >= sum_union_rate_thresh || dirty_rect_count == max_dirty_rect_count)
        {
            dirty_rect_list[best_match_index] = union_rects[best_match_index];
            dirty_rect_area += area_increments[best_match_index];
        }
        else
        {
            dirty_rect_list[dirty_rect_count] = rc;
            dirty_rect_count++;
            dirty_rect_area += rc_area;
        }
        if(dirty_rect_area >= default_dirty_rect.area())
        {
            dirty_rect_count = -1;
        }
    }
#endif

    void Painter::set_mat(const cv::Mat& mat_)
    {
        mat = mat_;
        default_dirty_rect.width = mat_.cols;
        default_dirty_rect.height = mat_.rows;
    }

    Mat Painter::get_mat() const
    {
        return mat;
    }

    Size Painter::get_mat_size() const
    {
        return Size(mat.cols, mat.rows);
    }

    Rect2f boundingRect(const std::vector<cv::Point2f>& pts)
    {
        if(pts.empty()) return Rect2f();
        float min_x = pts[0].x, max_x = pts[0].x, min_y = pts[0].y, max_y = pts[0].y;
        for(const cv::Point2f& pt: pts)
        {
            if(pt.x < min_x) min_x = pt.x;
            if(pt.y < min_y) min_y = pt.y;
            if(pt.x > max_x) max_x = pt.x;
            if(pt.y > max_y) max_y = pt.y;
        }
        return Rect2f(min_x, min_y, max_x - min_x + 1.0f, max_y - min_y + 1.0f);
    }

    Rect boundingRect(const std::vector<cv::Point>& pts)
    {
        if(pts.empty()) return Rect();
        int min_x = pts[0].x, max_x = pts[0].x, min_y = pts[0].y, max_y = pts[0].y;
        for(const cv::Point& pt: pts)
        {
            if(pt.x < min_x) min_x = pt.x;
            if(pt.y < min_y) min_y = pt.y;
            if(pt.x > max_x) max_x = pt.x;
            if(pt.y > max_y) max_y = pt.y;
        }
        return Rect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    }

    const uint16_t RGB332to565LUT[256] = {
        0x0000, 0x000a, 0x0015, 0x001f, 0x0120, 0x012a, 0x0135, 0x013f, 
        0x0240, 0x024a, 0x0255, 0x025f, 0x0360, 0x036a, 0x0375, 0x037f, 
        0x0480, 0x048a, 0x0495, 0x049f, 0x05a0, 0x05aa, 0x05b5, 0x05bf, 
        0x06c0, 0x06ca, 0x06d5, 0x06df, 0x07e0, 0x07ea, 0x07f5, 0x07ff, 
        0x2000, 0x200a, 0x2015, 0x201f, 0x2120, 0x212a, 0x2135, 0x213f, 
        0x2240, 0x224a, 0x2255, 0x225f, 0x2360, 0x236a, 0x2375, 0x237f, 
        0x2480, 0x248a, 0x2495, 0x249f, 0x25a0, 0x25aa, 0x25b5, 0x25bf, 
        0x26c0, 0x26ca, 0x26d5, 0x26df, 0x27e0, 0x27ea, 0x27f5, 0x27ff, 
        0x4800, 0x480a, 0x4815, 0x481f, 0x4920, 0x492a, 0x4935, 0x493f, 
        0x4a40, 0x4a4a, 0x4a55, 0x4a5f, 0x4b60, 0x4b6a, 0x4b75, 0x4b7f, 
        0x4c80, 0x4c8a, 0x4c95, 0x4c9f, 0x4da0, 0x4daa, 0x4db5, 0x4dbf, 
        0x4ec0, 0x4eca, 0x4ed5, 0x4edf, 0x4fe0, 0x4fea, 0x4ff5, 0x4fff, 
        0x6800, 0x680a, 0x6815, 0x681f, 0x6920, 0x692a, 0x6935, 0x693f, 
        0x6a40, 0x6a4a, 0x6a55, 0x6a5f, 0x6b60, 0x6b6a, 0x6b75, 0x6b7f, 
        0x6c80, 0x6c8a, 0x6c95, 0x6c9f, 0x6da0, 0x6daa, 0x6db5, 0x6dbf, 
        0x6ec0, 0x6eca, 0x6ed5, 0x6edf, 0x6fe0, 0x6fea, 0x6ff5, 0x6fff, 
        0x9000, 0x900a, 0x9015, 0x901f, 0x9120, 0x912a, 0x9135, 0x913f, 
        0x9240, 0x924a, 0x9255, 0x925f, 0x9360, 0x936a, 0x9375, 0x937f, 
        0x9480, 0x948a, 0x9495, 0x949f, 0x95a0, 0x95aa, 0x95b5, 0x95bf, 
        0x96c0, 0x96ca, 0x96d5, 0x96df, 0x97e0, 0x97ea, 0x97f5, 0x97ff, 
        0xb000, 0xb00a, 0xb015, 0xb01f, 0xb120, 0xb12a, 0xb135, 0xb13f, 
        0xb240, 0xb24a, 0xb255, 0xb25f, 0xb360, 0xb36a, 0xb375, 0xb37f, 
        0xb480, 0xb48a, 0xb495, 0xb49f, 0xb5a0, 0xb5aa, 0xb5b5, 0xb5bf, 
        0xb6c0, 0xb6ca, 0xb6d5, 0xb6df, 0xb7e0, 0xb7ea, 0xb7f5, 0xb7ff, 
        0xd800, 0xd80a, 0xd815, 0xd81f, 0xd920, 0xd92a, 0xd935, 0xd93f, 
        0xda40, 0xda4a, 0xda55, 0xda5f, 0xdb60, 0xdb6a, 0xdb75, 0xdb7f, 
        0xdc80, 0xdc8a, 0xdc95, 0xdc9f, 0xdda0, 0xddaa, 0xddb5, 0xddbf, 
        0xdec0, 0xdeca, 0xded5, 0xdedf, 0xdfe0, 0xdfea, 0xdff5, 0xdfff, 
        0xf800, 0xf80a, 0xf815, 0xf81f, 0xf920, 0xf92a, 0xf935, 0xf93f, 
        0xfa40, 0xfa4a, 0xfa55, 0xfa5f, 0xfb60, 0xfb6a, 0xfb75, 0xfb7f, 
        0xfc80, 0xfc8a, 0xfc95, 0xfc9f, 0xfda0, 0xfdaa, 0xfdb5, 0xfdbf, 
        0xfec0, 0xfeca, 0xfed5, 0xfedf, 0xffe0, 0xffea, 0xfff5, 0xffff 
    };

    void cvtColor(const Mat& src, Mat& dest, int code)
    {
        // TODO
    }

}
