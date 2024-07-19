#pragma once

#include "../core/api.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SURFACE_CNT_MAX	6//root + pages

typedef enum
{
	Z_ORDER_LEVEL_0,//lowest graphic level
	Z_ORDER_LEVEL_1,//middle graphic level, call activate_layer before use it, draw everything inside the active rect.
	Z_ORDER_LEVEL_2,//highest graphic level, call activate_layer before use it, draw everything inside the active rect.
	Z_ORDER_LEVEL_MAX
}Z_ORDER_LEVEL;

struct DISPLAY_DRIVER
{
	void(*draw_pixel)(int x, int y, unsigned int rgb);
	void(*fill_rect)(int x0, int y0, int x1, int y1, unsigned int rgb);
};

class c_surface;
class c_display {
	friend class c_surface;
public:
	inline c_display(void* phy_fb, int display_width, int display_height, c_surface* surface, DISPLAY_DRIVER* driver = 0);//single custom surface
	inline c_display(void* phy_fb, int display_width, int display_height, int surface_width, int surface_height, unsigned int color_bytes, int surface_cnt, DISPLAY_DRIVER* driver = 0);//multiple surface
	inline c_surface* alloc_surface(Z_ORDER_LEVEL max_zorder, c_rect layer_rect = c_rect());//for slide group
	inline int swipe_surface(c_surface* s0, c_surface* s1, int x0, int x1, int y0, int y1, int offset);
	int get_width() { return m_width; }
	int get_height() { return m_height; }
	void* get_phy_fb() { return m_phy_fb; }

	void* get_updated_fb(int* width, int* height, bool force_update = false)
	{
		if (width && height)
		{
			*width = m_width;
			*height = m_height;
		}
		if (force_update)
		{
			return m_phy_fb;
		}
		if (m_phy_read_index == m_phy_write_index)
		{//No update
			return 0;
		}
		m_phy_read_index = m_phy_write_index;
		return m_phy_fb;
	}

	int snap_shot(const char* file_name)
	{
		if (!m_phy_fb || (m_color_bytes !=2 && m_color_bytes != 4))
		{
			return -1;
		}

		//16 bits framebuffer
		if (m_color_bytes == 2)
		{
			return build_bmp(file_name, m_width, m_height, (unsigned char*)m_phy_fb);
		}

		//32 bits framebuffer
		unsigned short* p_bmp565_data = new unsigned short[m_width * m_height];
		unsigned int* p_raw_data = (unsigned int*)m_phy_fb;

		for (int i = 0; i < m_width * m_height; i++)
		{
			unsigned int rgb = *p_raw_data++;
			p_bmp565_data[i] = GL_RGB_32_to_16(rgb);
		}

		int ret = build_bmp(file_name, m_width, m_height, (unsigned char*)p_bmp565_data);
		delete[]p_bmp565_data;
		return ret;
	}

protected:
	virtual void draw_pixel(int x, int y, unsigned int rgb)
	{
		if ((x >= m_width) || (y >= m_height)) { return; }

		if (m_driver && m_driver->draw_pixel)
		{
			return m_driver->draw_pixel(x, y, rgb);
		}

		if (m_color_bytes == 2)
		{
			((unsigned short*)m_phy_fb)[y * m_width + x] = GL_RGB_32_to_16(rgb);
		}
		else
		{
			((unsigned int*)m_phy_fb)[y * m_width + x] = rgb;
		}
	}

	virtual void fill_rect(int x0, int y0, int x1, int y1, unsigned int rgb)
	{
		if (m_driver && m_driver->fill_rect)
		{
			return m_driver->fill_rect(x0, y0, x1, y1, rgb);
		}

		if (m_driver && m_driver->draw_pixel)
		{
			for (int y = y0; y <= y1; y++)
			{
				for (int x = x0; x <= x1; x++)
				{
					m_driver->draw_pixel(x, y, rgb);
				}
			}
			return;
		}

		int _width = m_width;
		int _height = m_height;
		int x, y;
		if (m_color_bytes == 2)
		{
			unsigned short* phy_fb;
			unsigned int rgb_16 = GL_RGB_32_to_16(rgb);
			for (y = y0; y <= y1; y++)
			{
				phy_fb = &((unsigned short*)m_phy_fb)[y * _width + x0];
				for (x = x0; x <= x1; x++)
				{
					if ((x < _width) && (y < _height))
					{
						*phy_fb++ = rgb_16;
					}
				}
			}
		}
		else
		{
			unsigned int* phy_fb;
			for (y = y0; y <= y1; y++)
			{
				phy_fb = &((unsigned int*)m_phy_fb)[y * _width + x0];
				for (x = x0; x <= x1; x++)
				{
					if ((x < _width) && (y < _height))
					{
						*phy_fb++ = rgb;
					}
				}
			}
		}
	}

	virtual int flush_screen(int left, int top, int right, int bottom, void* fb, int fb_width)
	{
		if ((0 == m_phy_fb) || (0 == fb))
		{
			return -1;
		}

		int _width = m_width;
		int _height = m_height;

		left = (left >= _width) ? (_width - 1) : left;
		right = (right >= _width) ? (_width - 1) : right;
		top = (top >= _height) ? (_height - 1) : top;
		bottom = (bottom >= _height) ? (_height - 1) : bottom;

		for (int y = top; y < bottom; y++)
		{
			void* s_addr = (char*)fb + ((y * fb_width + left) * m_color_bytes);
			void* d_addr = (char*)m_phy_fb + ((y * _width + left) * m_color_bytes);
			memcpy(d_addr, s_addr, (right - left) * m_color_bytes);
		}
		return 0;
	}

	int						m_width;		//in pixels
	int						m_height;		//in pixels
	int						m_color_bytes;	//16/32 bits for default
	void*					m_phy_fb;		//physical framebuffer for default
	struct DISPLAY_DRIVER*  m_driver;		//Rendering by external method without default physical framebuffer

	int				m_phy_read_index;
	int				m_phy_write_index;
	c_surface*		m_surface_group[SURFACE_CNT_MAX];
	int				m_surface_cnt;	//surface count
	int				m_surface_index;
	
};

class c_layer
{
public:
	c_layer() { fb = 0; }
	void* fb;		//framebuffer
	c_rect rect;	//framebuffer area
	c_rect active_rect;
};

class c_surface {
	friend class c_display; friend class c_bitmap_operator;
public:
	Z_ORDER_LEVEL get_max_z_order() { return m_max_zorder; }

	c_surface(unsigned int width, unsigned int height, unsigned int color_bytes, Z_ORDER_LEVEL max_zorder = Z_ORDER_LEVEL_0, c_rect overlpa_rect = c_rect()) : m_width(width), m_height(height), m_color_bytes(color_bytes), m_fb(0), m_is_active(false), m_top_zorder(Z_ORDER_LEVEL_0), m_phy_write_index(0), m_display(0)
	{
		(overlpa_rect == c_rect()) ? set_surface(max_zorder, c_rect(0, 0, width, height)) : set_surface(max_zorder, overlpa_rect);
	}

	unsigned int get_pixel(int x, int y, unsigned int z_order)
	{
		if (x >= m_width || y >= m_height || x < 0 || y < 0 || z_order >= Z_ORDER_LEVEL_MAX)
		{
			ASSERT(false);
			return 0;
		}
		if (m_layers[z_order].fb)
		{
			return (m_color_bytes == 2) ? GL_RGB_16_to_32(((unsigned short*)(m_layers[z_order].fb))[y * m_width + x]) : ((unsigned int*)(m_layers[z_order].fb))[y * m_width + x];
		}
		else if (m_fb)
		{
			return (m_color_bytes == 2) ? GL_RGB_16_to_32(((unsigned short*)m_fb)[y * m_width + x]) : ((unsigned int*)m_fb)[y * m_width + x];
		}
		else if (m_display->m_phy_fb)
		{
			return (m_color_bytes == 2) ? GL_RGB_16_to_32(((unsigned short*)m_display->m_phy_fb)[y * m_width + x]) : ((unsigned int*)m_display->m_phy_fb)[y * m_width + x];
		}
		return 0;
	}

	virtual void draw_pixel(int x, int y, unsigned int rgb, unsigned int z_order)
	{
		if (x >= m_width || y >= m_height || x < 0 || y < 0)
		{
			return;
		}
		
		if (z_order > (unsigned int)m_max_zorder)
		{
			ASSERT(false);
			return;
		}

		if (z_order > (unsigned int)m_top_zorder)
		{
			m_top_zorder = (Z_ORDER_LEVEL)z_order;
		}

		if (z_order == m_max_zorder)
		{
			return draw_pixel_low_level(x, y, rgb);
		}

		if (m_layers[z_order].rect.pt_in_rect(x, y))
		{
			c_rect layer_rect = m_layers[z_order].rect;
			if (m_color_bytes == 2)
			{
				((unsigned short*)(m_layers[z_order].fb))[(x - layer_rect.m_left) + (y - layer_rect.m_top) * layer_rect.width()] = GL_RGB_32_to_16(rgb);
			}
			else
			{
				((unsigned int*)(m_layers[z_order].fb))[(x - layer_rect.m_left) + (y - layer_rect.m_top) * layer_rect.width()] = rgb;
			}
		}
		
		if (z_order == m_top_zorder)
		{
			return draw_pixel_low_level(x, y, rgb);
		}

		bool be_overlapped = false;
		for (unsigned int tmp_z_order = Z_ORDER_LEVEL_MAX - 1; tmp_z_order > z_order; tmp_z_order--)
		{
			if (m_layers[tmp_z_order].active_rect.pt_in_rect(x, y))
			{
				be_overlapped = true;
				break;
			}
		}

		if (!be_overlapped)
		{
			draw_pixel_low_level(x, y, rgb);
		}
	}

	virtual void fill_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order)
	{
		x0 = (x0 < 0) ? 0 : x0;
		y0 = (y0 < 0) ? 0 : y0;
		x1 = (x1 > (m_width - 1)) ? (m_width - 1) : x1;
		y1 = (y1 > (m_height - 1)) ? (m_height - 1) : y1;

		if (z_order == m_max_zorder)
		{
			return fill_rect_low_level(x0, y0, x1, y1, rgb);
		}

		if (z_order == m_top_zorder)
		{
			int width = m_layers[z_order].rect.width();
			c_rect layer_rect = m_layers[z_order].rect;
			unsigned int rgb_16 = GL_RGB_32_to_16(rgb);
			for (int y = y0; y <= y1; y++)
			{
				for (int x = x0; x <= x1; x++)
				{
					if (layer_rect.pt_in_rect(x, y))
					{
						if (m_color_bytes == 2)
						{
							((unsigned short*)m_layers[z_order].fb)[(y - layer_rect.m_top) * width + (x - layer_rect.m_left)] = rgb_16;
						}
						else
						{
							((unsigned int*)m_layers[z_order].fb)[(y - layer_rect.m_top) * width + (x - layer_rect.m_left)] = rgb;	
						}
					}
				}
			}
			return fill_rect_low_level(x0, y0, x1, y1, rgb);
		}

		for (; y0 <= y1; y0++)
		{
			draw_hline(x0, x1, y0, rgb, z_order);
		}
	}

	void draw_hline(int x0, int x1, int y, unsigned int rgb, unsigned int z_order)
	{
		for (; x0 <= x1; x0++)
		{
			draw_pixel(x0, y, rgb, z_order);
		}
	}

	void draw_vline(int x, int y0, int y1, unsigned int rgb, unsigned int z_order)
	{
		for (; y0 <= y1; y0++)
		{
			draw_pixel(x, y0, rgb, z_order);
		}
	}

	void draw_line(int x1, int y1, int x2, int y2, unsigned int rgb, unsigned int z_order)
	{
		int dx, dy, x, y, e;

		(x1 > x2) ? (dx = x1 - x2) : (dx = x2 - x1);
		(y1 > y2) ? (dy = y1 - y2) : (dy = y2 - y1);

		if (((dx > dy) && (x1 > x2)) || ((dx <= dy) && (y1 > y2)))
		{
			x = x2; y = y2;
			x2 = x1; y2 = y1;
			x1 = x; y1 = y;
		}
		x = x1; y = y1;

		if (dx > dy)
		{
			e = dy - dx / 2;
			for (; x1 <= x2; ++x1, e += dy)
			{
				draw_pixel(x1, y1, rgb, z_order);
				if (e > 0) { e -= dx; (y > y2) ? --y1 : ++y1; }
			}
		}
		else
		{
			e = dx - dy / 2;
			for (; y1 <= y2; ++y1, e += dx)
			{
				draw_pixel(x1, y1, rgb, z_order);
				if (e > 0) { e -= dy; (x > x2) ? --x1 : ++x1; }
			}
		}
	}

	void draw_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order, unsigned int size = 1)
	{
		for (unsigned int offset = 0; offset < size; offset++)
		{
			draw_hline(x0 + offset, x1 - offset, y0 + offset, rgb, z_order);
			draw_hline(x0 + offset, x1 - offset, y1 - offset, rgb, z_order);
			draw_vline(x0 + offset, y0 + offset, y1 - offset, rgb, z_order);
			draw_vline(x1 - offset, y0 + offset, y1 - offset, rgb, z_order);
		}
	}

	void draw_rect(c_rect rect, unsigned int rgb, unsigned int size, unsigned int z_order)
	{
		draw_rect(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, rgb, z_order, size);
	}

	void fill_rect(c_rect rect, unsigned int rgb, unsigned int z_order)
	{
		fill_rect(rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, rgb, z_order);
	}

	int flush_screen(int left, int top, int right, int bottom)
	{
		if (!m_is_active)
		{
			return -1;
		}

		if (left < 0 || left >= m_width || right < 0 || right >= m_width ||
			top < 0 || top >= m_height || bottom < 0 || bottom >= m_height)
		{
			ASSERT(false);
		}

		m_display->flush_screen(left, top, right, bottom, m_fb, m_width);
		*m_phy_write_index = *m_phy_write_index + 1;
		return 0;
	}

	bool is_active() { return m_is_active; }
	c_display* get_display() { return m_display; }

	void activate_layer(c_rect active_rect, unsigned int active_z_order)//empty active rect means inactivating the layer
	{
		ASSERT(active_z_order > Z_ORDER_LEVEL_0 && active_z_order <= Z_ORDER_LEVEL_MAX);
		
		//Show the layers below the current active rect.
		c_rect current_active_rect = m_layers[active_z_order].active_rect;
		for(int low_z_order = Z_ORDER_LEVEL_0; low_z_order < active_z_order; low_z_order++)
		{
			c_rect low_layer_rect = m_layers[low_z_order].rect;
			c_rect low_active_rect = m_layers[low_z_order].active_rect;
			void* fb = m_layers[low_z_order].fb;
			int width = low_layer_rect.width();
			for (int y = current_active_rect.m_top; y <= current_active_rect.m_bottom; y++)
			{
				for (int x = current_active_rect.m_left; x <= current_active_rect.m_right; x++)
				{
					if (low_active_rect.pt_in_rect(x, y) && low_layer_rect.pt_in_rect(x, y))//active rect maybe is bigger than layer rect
					{
						unsigned int rgb = (m_color_bytes == 2) ? GL_RGB_16_to_32(((unsigned short*)fb)[(x - low_layer_rect.m_left) + (y - low_layer_rect.m_top) * width]) : ((unsigned int*)fb)[(x - low_layer_rect.m_left) + (y - low_layer_rect.m_top) * width];
						draw_pixel_low_level(x, y, rgb);
					}
				}
			}
		}
		m_layers[active_z_order].active_rect = active_rect;//set the new acitve rect.
	}

	void set_active(bool flag) { m_is_active = flag; }
protected:
	virtual void fill_rect_low_level(int x0, int y0, int x1, int y1, unsigned int rgb)
	{//fill rect on framebuffer of surface
		int x, y;
		if (m_color_bytes == 2)
		{
			unsigned short* fb;
			unsigned int rgb_16 = GL_RGB_32_to_16(rgb);
			for (y = y0; y <= y1; y++)
			{
				fb = m_fb ? &((unsigned short*)m_fb)[y * m_width + x0] : 0;
				if (!fb) { break; }
				for (x = x0; x <= x1; x++)
				{
					*fb++ = rgb_16;
				}
			}
		}
		else
		{
			unsigned int* fb;
			for (y = y0; y <= y1; y++)
			{
				fb = m_fb ? &((unsigned int*)m_fb)[y * m_width + x0] : 0;
				if (!fb) { break; }
				for (x = x0; x <= x1; x++)
				{
					*fb++ = rgb;
				}
			}
		}

		if (!m_is_active) { return; }
		m_display->fill_rect(x0, y0, x1, y1, rgb);
		*m_phy_write_index = *m_phy_write_index + 1;
	}

	virtual void draw_pixel_low_level(int x, int y, unsigned int rgb)
	{
		if (m_fb)
		{//draw pixel on framebuffer of surface
			(m_color_bytes == 2) ? ((unsigned short*)m_fb)[y * m_width + x] = GL_RGB_32_to_16(rgb): ((unsigned int*)m_fb)[y * m_width + x] = rgb;
		}
		if (!m_is_active) { return; }
		m_display->draw_pixel(x, y, rgb);
		*m_phy_write_index = *m_phy_write_index + 1;
	}

	void attach_display(c_display* display)
	{
		ASSERT(display);
		m_display = display;
		m_phy_write_index = &display->m_phy_write_index;
	}

	void set_surface(Z_ORDER_LEVEL max_z_order, c_rect layer_rect)
	{
		m_max_zorder = max_z_order;
		if (m_display && (m_display->m_surface_cnt > 1))
		{
			m_fb = calloc(m_width * m_height, m_color_bytes);
		}

		for (int i = Z_ORDER_LEVEL_0; i < m_max_zorder; i++)
		{//Top layber fb always be 0
			ASSERT(m_layers[i].fb = calloc(layer_rect.width() * layer_rect.height(), m_color_bytes));
			m_layers[i].rect = layer_rect;
		}

		m_layers[Z_ORDER_LEVEL_0].active_rect = layer_rect;
	}

	int				m_width;		//in pixels
	int				m_height;		//in pixels
	int				m_color_bytes;	//16 bits, 32 bits for default
	void*			m_fb;			//frame buffer you could see
	c_layer 		m_layers[Z_ORDER_LEVEL_MAX];//all graphic layers
	bool			m_is_active;	//active flag
	Z_ORDER_LEVEL	m_max_zorder;	//the highest graphic layer the surface will have
	Z_ORDER_LEVEL	m_top_zorder;	//the current highest graphic layer the surface have
	int*			m_phy_write_index;
	c_display*		m_display;
};

inline c_display::c_display(void* phy_fb, int display_width, int display_height, c_surface* surface, DISPLAY_DRIVER* driver) : m_phy_fb(phy_fb), m_width(display_width), m_height(display_height), m_driver(driver), m_phy_read_index(0), m_phy_write_index(0), m_surface_cnt(1), m_surface_index(0)
{
	m_color_bytes = surface->m_color_bytes;
	surface->m_is_active = true;
	(m_surface_group[0] = surface)->attach_display(this);
}

inline c_display::c_display(void* phy_fb, int display_width, int display_height, int surface_width, int surface_height, unsigned int color_bytes, int surface_cnt, DISPLAY_DRIVER* driver) : m_phy_fb(phy_fb), m_width(display_width), m_height(display_height), m_color_bytes(color_bytes), m_phy_read_index(0), m_phy_write_index(0), m_surface_cnt(surface_cnt), m_driver(driver), m_surface_index(0)
{
	ASSERT(color_bytes == 2 || color_bytes == 4);
	ASSERT(m_surface_cnt <= SURFACE_CNT_MAX);
	memset(m_surface_group, 0, sizeof(m_surface_group));
	
	for (int i = 0; i < m_surface_cnt; i++)
	{
		m_surface_group[i] = new c_surface(surface_width, surface_height, color_bytes);
		m_surface_group[i]->attach_display(this);
	}
}

inline c_surface* c_display::alloc_surface(Z_ORDER_LEVEL max_zorder, c_rect layer_rect)
{
	ASSERT(max_zorder < Z_ORDER_LEVEL_MAX && m_surface_index < m_surface_cnt);
	(layer_rect == c_rect()) ? m_surface_group[m_surface_index]->set_surface(max_zorder, c_rect(0, 0, m_width, m_height)) : m_surface_group[m_surface_index]->set_surface(max_zorder, layer_rect);
	return m_surface_group[m_surface_index++];
}

inline int c_display::swipe_surface(c_surface* s0, c_surface* s1, int x0, int x1, int y0, int y1, int offset)
{
	int surface_width = s0->m_width;
	int surface_height = s0->m_height;

	if (offset < 0 || offset > surface_width || y0 < 0 || y0 >= surface_height ||
		y1 < 0 || y1 >= surface_height || x0 < 0 || x0 >= surface_width || x1 < 0 || x1 >= surface_width)
	{
		ASSERT(false);
		return -1;
	}

	int width = (x1 - x0 + 1);
	if (width < 0 || width > surface_width || width < offset)
	{
		ASSERT(false);
		return -1;
	}

	x0 = (x0 >= m_width) ? (m_width - 1) : x0;
	x1 = (x1 >= m_width) ? (m_width - 1) : x1;
	y0 = (y0 >= m_height) ? (m_height - 1) : y0;
	y1 = (y1 >= m_height) ? (m_height - 1) : y1;

	if (m_phy_fb)
	{
		for (int y = y0; y <= y1; y++)
		{
			//Left surface
			char* addr_s = ((char*)(s0->m_fb) + (y * surface_width + x0 + offset) * m_color_bytes);
			char* addr_d = ((char*)(m_phy_fb)+(y * m_width + x0) * m_color_bytes);
			memcpy(addr_d, addr_s, (width - offset) * m_color_bytes);
			//Right surface
			addr_s = ((char*)(s1->m_fb) + (y * surface_width + x0) * m_color_bytes);
			addr_d = ((char*)(m_phy_fb)+(y * m_width + x0 + (width - offset)) * m_color_bytes);
			memcpy(addr_d, addr_s, offset * m_color_bytes);
		}
	}
	else if (m_color_bytes == 2)
	{
		void(*draw_pixel)(int x, int y, unsigned int rgb) = m_driver->draw_pixel;
		for (int y = y0; y <= y1; y++)
		{
			//Left surface
			for (int x = x0; x <= (x1 - offset); x++)
			{
				draw_pixel(x, y, GL_RGB_16_to_32(((unsigned short*)s0->m_fb)[y * m_width + x + offset]));
			}
			//Right surface
			for (int x = x1 - offset; x <= x1; x++)
			{
				draw_pixel(x, y, GL_RGB_16_to_32(((unsigned short*)s1->m_fb)[y * m_width + x + offset - x1 + x0]));
			}
		}
	}
	else //m_color_bytes == 3/4...
	{
		void(*draw_pixel)(int x, int y, unsigned int rgb) = m_driver->draw_pixel;
		for (int y = y0; y <= y1; y++)
		{
			//Left surface
			for (int x = x0; x <= (x1 - offset); x++)
			{
				draw_pixel(x, y, ((unsigned int*)s0->m_fb)[y * m_width + x + offset]);
			}
			//Right surface
			for (int x = x1 - offset; x <= x1; x++)
			{
				draw_pixel(x, y, ((unsigned int*)s1->m_fb)[y * m_width + x + offset - x1 + x0]);
			}
		}
	}

	m_phy_write_index++;
	return 0;
}
