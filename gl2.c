#include "gl2.h"
#include "font.h"
#include "printf.h"
#include "timer.h"

void gl_init(unsigned int width, unsigned int height)
{
    fb_init(width, height, 4);    // use 32-bit depth always for graphics library
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

unsigned int gl_get_width(void)
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return 0xFF000000 + (r << 16) + (g << 8) + b;
}

void gl_clear(color_t c)
{
    unsigned int* buf = fb_get_draw_buffer();
    int npixels = fb_get_pitch()*fb_get_height() / 4;
    for (int i = 0; i < npixels; i++)
        buf[i] = c;
}

// Code used from lab6
void gl_draw_pixel(int x, int y, color_t c)
{
    // Do not draw outside of bounds
    if (x < 0 || x >= fb_get_width() || y < 0 || y >= fb_get_height())
        return;
    
    unsigned int (*im)[fb_get_pitch() / 4] = fb_get_draw_buffer();
    im[y][x] = c;
}

// Draws an opaque pixel, combining the drawn color with the original color of the pixel
void gl_draw_pixel_opaque (int x, int y, color_t c, double opacity) {
    color_t new_c = 0xFF000000;
    color_t org_c = gl_read_pixel(x, y);
    new_c += ((unsigned int)((double)(c & 0x00FF0000) * opacity) & 0x00FF0000) 
            + ((unsigned int)((double)(org_c & 0x00FF0000) * (1-opacity)) & 0x00FF0000); // R
    new_c += ((unsigned int)((double)(c & 0x0000FF00) * opacity) & 0x0000FF00) 
            + ((unsigned int)((double)(org_c & 0x0000FF00) * (1-opacity)) & 0x0000FF00); // G
    new_c += ((unsigned int)((double)(c & 0x000000FF) * opacity) & 0x000000FF) 
            + ((unsigned int)((double)(org_c & 0x000000FF) * (1-opacity)) & 0x000000FF); // B

    gl_draw_pixel(x, y, new_c);
}

color_t gl_read_pixel(int x, int y)
{
    // Do not read outside of bounds
    if (x >= fb_get_width() || x < 0 || y >= fb_get_height() || y < 0)
        return 0;
    
    unsigned int (*im)[fb_get_pitch() / 4] = fb_get_draw_buffer();
    return im[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int pix_x = x > 0 ? x : 0; pix_x < x + w && pix_x < fb_get_width(); pix_x++)
        for (int pix_y = y > 0 ? y : 0; pix_y < y + h && pix_y < fb_get_height(); pix_y++)
            gl_draw_pixel(pix_x, pix_y, c);
}

void gl_draw_char(int x, int y, char ch, color_t c)
{
    int bufsize = font_get_glyph_size();
    unsigned char char_buf[bufsize];
    if (!font_get_glyph(ch, char_buf, bufsize)) // No glyph in current font
        return;

    for (int i = 0; i < bufsize; i++) {
        if (char_buf[i])
            gl_draw_pixel(x + i % (font_get_glyph_width()), y + i / font_get_glyph_width(), c);
    }
}

void gl_draw_string(int x, int y, const char* str, color_t c)
{
    while (*str) {
        gl_draw_char(x, y, *str, c);
        x += font_get_glyph_width();
        str++;
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_glyph_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_glyph_width();
}

// Macros used in draw _line
#define ipart(n) ((int)(n))         // Floor
#define round(n) (ipart((n) + 0.5)) // Round to nearest int
#define fpart(n) ((n) - ipart(n))   // Take fractional part
#define rfpart(n) (1 - fpart((n)))  // Take 1 - fractional part
#define abs(n) ((n >= 0) ? (n) : (n * -1))
void swap(int *a, int *b) { // Swap two integers
    int temp = *a;
    *a = *b;
    *b = temp;
}

void gl_draw_line(int x1, int y1, int x2, int y2, color_t c) {


    unsigned int steep = (abs((y2 - y1)) > abs((x2 - x1))) ? 1 : 0;
    if (steep) {
        swap(&x1, &y1);
        swap(&x2, &y2);
    }
    if (x2 < x1) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    double slope = x1 == x2 ? 1 : (double)(y2 - y1) / (double)(x2 - x1);

    double y = y1;
    if (steep) {
        for (int x = x1; x <= x2; x++) {
            gl_draw_pixel_opaque(ipart(y), x, c, rfpart(y));
            gl_draw_pixel_opaque(ipart(y) + 1, x, c, fpart(y));
            y += slope;
        }
    } else {
        for (int x = x1; x <= x2; x++) {
            gl_draw_pixel_opaque(x, ipart(y), c, rfpart(y));
            gl_draw_pixel_opaque(x, ipart(y) + 1, c, fpart(y));
            y += slope;
        }
    }
}

void gl_draw_triangle_top(int xt, int yt, int xl, int xr, int yb, color_t c) {
    if (xl > xr)
        swap(&xl, &xr);

    double slope_l = (double)(xl - xt) / (double)(yb - yt);
    double slope_r = (double)(xr - xt) / (double)(yb - yt);

    double x_start = xt;
    double x_end = xt;

    for (int y = yt; y < yb; y++) {
        for (int x = round(x_start + 0.5); x <= round(x_end - 0.5); x++) {
            gl_draw_pixel(x, y, c);
        }
        x_start += slope_l;
        x_end += slope_r;
    }
}

void gl_draw_triangle_bottom(int xb, int yb, int xl, int xr, int yt, color_t c) {
    if (xl > xr)
        swap(&xl, &xr);

    double slope_l = (double)(xb - xl) / (double)(yb - yt);
    double slope_r = (double)(xb - xr) / (double)(yb - yt);

    double x_start = xl;
    double x_end = xr;

    for (int y = yt; y < yb; y++) {
        for (int x = round(x_start + 0.5); x <= round(x_end - 0.5); x++) {
            gl_draw_pixel(x, y, c);
        }
        x_start += slope_l;
        x_end += slope_r;
    }
}

void gl_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t c) {
    // Put points in order (y1 on top)
    if (y2 > y3) {
        swap(&y2, &y3);
        swap(&x2, &x3);
    }
    if (y1 > y2) {
        swap(&y1, &y2);
        swap(&x1, &x2);
    }
    if (y2 > y3) {
        swap(&y2, &y3);
        swap(&x2, &x3);
    }

    // Draw borders
    gl_draw_line(x1, y1, x2, y2, c);
    gl_draw_line(x2, y2, x3, y3, c);
    gl_draw_line(x3, y3, x1, y1, c);

    // Reciprocal slopes
    // double slope12 = (double)(x2 - x1) / (double)(y2 - y1);
    // double slope23 = (double)(x3 - x2) / (double)(y3 - y2);
    double slope31 = (double)(x1 - x3) / (double)(y1 - y3);

    gl_draw_triangle_top(x1, y1, x1 + round(slope31 * (y2 - y1)), x2, y2, c);
    gl_draw_triangle_bottom(x3, y3, x1 + round(slope31 * (y2 - y1)), x2, y2, c);
}

void gl_swap_interrupt_buffer(void) {
    unsigned int start_tick = timer_get_ticks();
    fb_swap_interrupt_buffer();
}

void gl_draw_interrupt_square (int x, int y, int width, color_t c) {
    // Draw square
    unsigned int (*im)[fb_get_pitch() / 4] = fb_get_interrupt_buffer();
    for (int pix_x = x - width / 2 > 0 ? x - width / 2 : 0; pix_x < x + width / 2 && pix_x < fb_get_width(); pix_x++)
        for (int pix_y = y - width / 2 > 0 ? y - width / 2 : 0; pix_y < y + width / 2 && pix_y < fb_get_height(); pix_y++)
            im[pix_y][pix_x] = c;

}

void gl_interrupt_clear(color_t c) {
    unsigned int* buf = fb_get_interrupt_buffer();
    int npixels = fb_get_pitch()*fb_get_height() / 4;
    for (int i = 0; i < npixels; i++)
        buf[i] = c;
}