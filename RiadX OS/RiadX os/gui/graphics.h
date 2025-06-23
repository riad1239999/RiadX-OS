#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../drivers/display.h"
#include <vector>
#include <string>

// Graphics primitives and utilities
class Graphics {
public:
    // 2D Vector
    struct Vector2 {
        float x, y;
        Vector2(float x = 0, float y = 0) : x(x), y(y) {}
        Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
        Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
        Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    };
    
    // Matrix for 2D transformations
    struct Matrix2D {
        float m[3][3];
        
        Matrix2D();
        static Matrix2D identity();
        static Matrix2D translation(float x, float y);
        static Matrix2D rotation(float angle);
        static Matrix2D scaling(float x, float y);
        
        Matrix2D operator*(const Matrix2D& other) const;
        Vector2 transform(const Vector2& point) const;
    };
    
    // Polygon
    struct Polygon {
        std::vector<Vector2> vertices;
        Color color;
        bool filled;
        
        Polygon() : filled(true) {}
        void add_vertex(float x, float y) { vertices.emplace_back(x, y); }
        void clear() { vertices.clear(); }
    };
    
    // Gradient
    struct Gradient {
        Color start_color;
        Color end_color;
        Vector2 start_point;
        Vector2 end_point;
        
        Gradient(const Color& start, const Color& end, const Vector2& start_pt, const Vector2& end_pt)
            : start_color(start), end_color(end), start_point(start_pt), end_point(end_pt) {}
        
        Color get_color_at(const Vector2& point) const;
    };

private:
    PixelBuffer* target_buffer;
    Rect clip_rect;
    Matrix2D transform_matrix;
    
    // Internal drawing functions
    void draw_line_bresenham(int x1, int y1, int x2, int y2, const Color& color);
    void draw_filled_polygon_scanline(const Polygon& polygon);
    void draw_polygon_outline(const Polygon& polygon);
    bool is_point_in_clip_rect(int x, int y) const;
    Color blend_colors(const Color& src, const Color& dst, float alpha) const;

public:
    Graphics(PixelBuffer* buffer);
    ~Graphics();
    
    // Target buffer management
    void set_target(PixelBuffer* buffer);
    PixelBuffer* get_target() const { return target_buffer; }
    
    // Clipping
    void set_clip_rect(const Rect& rect);
    void clear_clip_rect();
    Rect get_clip_rect() const { return clip_rect; }
    
    // Transformations
    void set_transform(const Matrix2D& matrix);
    void reset_transform();
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    Matrix2D get_transform() const { return transform_matrix; }
    
    // Basic drawing
    void clear(const Color& color);
    void set_pixel(int x, int y, const Color& color);
    void draw_line(int x1, int y1, int x2, int y2, const Color& color, int thickness = 1);
    void draw_rect(const Rect& rect, const Color& color, bool filled = false);
    void draw_rounded_rect(const Rect& rect, int radius, const Color& color, bool filled = false);
    void draw_circle(int cx, int cy, int radius, const Color& color, bool filled = false);
    void draw_ellipse(int cx, int cy, int rx, int ry, const Color& color, bool filled = false);
    void draw_arc(int cx, int cy, int radius, float start_angle, float end_angle, const Color& color);
    
    // Advanced drawing
    void draw_polygon(const Polygon& polygon);
    void draw_bezier_curve(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Color& color);
    void draw_gradient_rect(const Rect& rect, const Gradient& gradient);
    void draw_gradient_circle(int cx, int cy, int radius, const Color& center_color, const Color& edge_color);
    
    // Text rendering
    void draw_text(int x, int y, const std::string& text, const Color& color);
    void draw_text_centered(const Rect& rect, const std::string& text, const Color& color);
    void draw_text_with_background(int x, int y, const std::string& text, const Color& text_color, const Color& bg_color);
    
    // Image operations
    void draw_bitmap(int x, int y, const uint32_t* bitmap, int width, int height);
    void draw_bitmap_scaled(const Rect& dest, const uint32_t* bitmap, int src_width, int src_height);
    void draw_bitmap_rotated(int x, int y, const uint32_t* bitmap, int width, int height, float angle);
    
    // Alpha blending
    void set_alpha_blend_mode(bool enabled);
    void draw_rect_alpha(const Rect& rect, const Color& color, float alpha);
    void draw_circle_alpha(int cx, int cy, int radius, const Color& color, float alpha);
    
    // Utility functions
    static Color interpolate_color(const Color& c1, const Color& c2, float t);
    static float distance(const Vector2& p1, const Vector2& p2);
    static float angle_between(const Vector2& p1, const Vector2& p2);
    static bool point_in_rect(const Vector2& point, const Rect& rect);
    static bool point_in_circle(const Vector2& point, const Vector2& center, float radius);
    
    // Color utilities
    static Color rgb_to_hsv(const Color& rgb);
    static Color hsv_to_rgb(float h, float s, float v);
    static Color darken_color(const Color& color, float factor);
    static Color lighten_color(const Color& color, float factor);
    
    // Pattern fills
    void fill_with_pattern(const Rect& rect, const uint32_t* pattern, int pattern_width, int pattern_height);
    void draw_checkered_pattern(const Rect& rect, const Color& color1, const Color& color2, int square_size);
    
    // Debug utilities
    void draw_grid(const Rect& rect, int spacing, const Color& color);
    void draw_coordinate_axes(const Rect& rect, const Color& color);
};

#endif
