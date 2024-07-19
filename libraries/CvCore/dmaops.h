#pragma once

#include "mbed.h"
#include "cvcore.h"

#if defined(DMA2D)

// clean DCACHE for the mat
void clean_cache_for_matrix(const cv::Mat& mat, const cv::Rect& roi);

// fill the mat with the given color
void dma2d_fill(const cv::Mat& mat, uint16_t color);

// fill 1D or 2D buffer(max number of cols is 16383, max number of rows is 65535)
void dma2d_memset(uint8_t *dest, uint8_t val, uint16_t rows, uint16_t cols, uint16_t offset = 0);
void dma2d_memset(uint16_t *dest, uint16_t val, uint16_t rows, uint16_t cols, uint16_t offset = 0);
void dma2d_memset(uint32_t *dest, uint32_t val, uint16_t rows, uint16_t cols, uint16_t offset = 0);

// copy roi of source mat to the given position of dest mat
void dma2d_copy(const cv::Mat& src_mat, const cv::Rect& src_roi, const cv::Mat& dest_mat, const cv::Point& dest_pos);

// copy the mat to the target continuous buffer
void dma2d_flat_copy(const cv::Mat& mat, const cv::Rect& roi, volatile void *buffer);

// copy 1D or 2D buffer(max number of cols is 16383, max number of rows is 65535)
void dma2d_memcpy(uint8_t *dest, const uint8_t *src, uint16_t rows, uint16_t cols, uint16_t offset_dest = 0, uint16_t offset_src = 0);
void dma2d_memcpy(uint16_t *dest, const uint16_t *src, uint16_t rows, uint16_t cols, uint16_t offset_dest = 0, uint16_t offset_src = 0);
void dma2d_memcpy(uint32_t *dest, const uint32_t *src, uint16_t rows, uint16_t cols, uint16_t offset_dest = 0, uint16_t offset_src = 0);

// transform a RGB332 mat to RGB565 and output to the target continuous buffer
void dma2d_flat_rgb332_to_rgb565(const cv::Mat& mat, const cv::Rect& roi, volatile void *buffer);

// mix src_bg_mat(rgb565) and src_fg_mat(argb1555) and output to target mat
void dma2d_blend_argb1555_to_rgb565(const cv::Mat& src_bg_mat, const cv::Rect& src_bg_roi, const cv::Mat& src_fg_mat, const cv::Point& src_fg_pos, 
    const cv::Mat& dest_mat, const cv::Point& dest_pos);

#endif
