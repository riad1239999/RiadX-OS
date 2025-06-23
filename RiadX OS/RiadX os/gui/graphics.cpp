#include "graphics.h"
#include <cmath>
#include <algorithm>

// Matrix2D implementation
Graphics::Matrix2D::Matrix2D() {
    // Initialize as identity matrix
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Graphics::Matrix2D Graphics::Matrix2D::identity() {
    return Matrix2D();
}

Graphics::Matrix2D Graphics::Matrix2D::translation(float x, float y) {
    Matrix2D result;
    result.m[0][2] = x;
    result.m[1][2] = y;
    return result;
}

Graphics::Matrix2D Graphics::Matrix2D::rotation(float angle) {
    Matrix2D result;
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    
    result.m[0][0] = cos_a;
    result.m[0][1] = -sin_a;
    result.m[1][0] = sin_a;
    result.m[1][1] = cos_a;
    
    return result;
}

Graphics::Matrix2D Graphics::Matrix2D::scaling(float x, float y) {
    Matrix2D result;
    result.m[0][0] = x;
    result.m[1][1] = y;
    return result;
}

Graphics::Matrix2D Graphics::Matrix2D::operator*(const Matrix2D& other) const {
    Matrix2D result;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = 0;
            for (int k = 0; k < 3; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    
    return result;
}

Graphics::Vector2 Graphics::Matrix2D::transform(const Vector2& point) const {
    float x = m[0][0] * point.x + m[0][1] * point.y + m[0][2];
    float y = m[1][0] * point.x + m[1][1] * point.y + m[1][2];
    return Vector2(x, y);
}

// Gradient implementation
Color Graphics::Gradient::get_color_at(const Vector2& point) const {
    // Calculate distance from start to end
    Vector2 gradient_vec = end_point - start_point;
    Vector2 point_vec = point - start_point;
    
    float gradient_length = std::sqrt(gradient_vec.x * gradient_vec.x + gradient_vec.y * gradient_vec.y);
    if (gradient_length == 0) return start_color;
    
    // Project point onto gradient line
    float dot_product = (point_vec.x * gradient_vec.x + point_vec.y * gradient_vec.y);
    float t = dot_product / (gradient_length * gradient_length);
    
    // Clamp t to [0, 1]
    t = std::max(0.0f, std::min(1.0f, t));
    
    return interpolate_color(start_color, end_color, t);
}

// Graphics implementation
Graphics::Graphics(PixelBuffer* buffer) 
    : target_buffer(buffer), transform_matrix(Matrix2D::identity()) {
    if (buffer) {
        clip_rect = Rect(0, 0, buffer->get_width(), buffer->get_height());
    }
}

Graphics::~Graphics() {
    // Nothing to clean up
}

void Graphics::set_target(PixelBuffer* buffer) {
    target_buffer = buffer;
    if (buffer) {
        clip_rect = Rect(0, 0, buffer->get_width(), buffer->get_height());
    }
}

void Graphics::set_clip_rect(const Rect& rect) {
    clip_rect = rect;
}

void Graphics::clear_clip_rect() {
    if (target_buffer) {
        clip_rect = Rect(0, 0, target_buffer->get_width(), target_buffer->get_height());
    }
}

void Graphics::set_transform(const Matrix2D& matrix) {
    transform_matrix = matrix;
}

void Graphics::reset_transform() {
    transform_matrix = Matrix2D::identity();
}

void Graphics::translate(float x, float y) {
    transform_matrix = transform_matrix * Matrix2D::translation(x, y);
}

void Graphics::rotate(float angle) {
    transform_matrix = transform_matrix * Matrix2D::rotation(angle);
}

void Graphics::scale(float x, float y) {
    transform_matrix = transform_matrix * Matrix2D::scaling(x, y);
}

void Graphics::clear(const Color& color) {
    if (target_buffer) {
        target_buffer->clear(color);
    }
}

void Graphics::set_pixel(int x, int y, const Color& color) {
    if (!target_buffer || !is_point_in_clip_rect(x, y)) return;
    
    Vector2 transformed = transform_matrix.transform(Vector2(x, y));
    target_buffer->set_pixel(static_cast<int>(transformed.x), static_cast<int>(transformed.y), color);
}

bool Graphics::is_point_in_clip_rect(int x, int y) const {
    return x >= clip_rect.x && x < clip_rect.x + clip_rect.width &&
           y >= clip_rect.y && y < clip_rect.y + clip_rect.height;
}

void Graphics::draw_line(int x1, int y1, int x2, int y2, const Color& color, int thickness) {
    if (!target_buffer) return;
    
    if (thickness == 1) {
        draw_line_bresenham(x1, y1, x2, y2, color);
    } else {
        // Draw thick line by drawing multiple offset lines
        for (int i = -thickness/2; i <= thickness/2; i++) {
            for (int j = -thickness/2; j <= thickness/2; j++) {
                draw_line_bresenham(x1 + i, y1 + j, x2 + i, y2 + j, color);
            }
        }
    }
}

void Graphics::draw_line_bresenham(int x1, int y1, int x2, int y2, const Color& color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    
    while (true) {
        set_pixel(x, y, color);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void Graphics::draw_rect(const Rect& rect, const Color& color, bool filled) {
    if (!target_buffer) return;
    
    if (filled) {
        for (int y = rect.y; y < rect.y + rect.height; y++) {
            for (int x = rect.x; x < rect.x + rect.width; x++) {
                set_pixel(x, y, color);
            }
        }
    } else {
        // Draw outline
        draw_line(rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
        draw_line(rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
        draw_line(rect.x + rect.width - 1, rect.y + rect.height - 1, rect.x, rect.y + rect.height - 1, color);
        draw_line(rect.x, rect.y + rect.height - 1, rect.x, rect.y, color);
    }
}

void Graphics::draw_rounded_rect(const Rect& rect, int radius, const Color& color, bool filled) {
    if (!target_buffer) return;
    
    // Draw rounded rectangle using arcs and lines
    int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    
    if (filled) {
        // Fill center rectangle
        draw_rect(Rect(x + radius, y, w - 2 * radius, h), color, true);
        draw_rect(Rect(x, y + radius, w, h - 2 * radius), color, true);
        
        // Fill corners with circles
        draw_circle(x + radius, y + radius, radius, color, true);
        draw_circle(x + w - radius - 1, y + radius, radius, color, true);
        draw_circle(x + radius, y + h - radius - 1, radius, color, true);
        draw_circle(x + w - radius - 1, y + h - radius - 1, radius, color, true);
    } else {
        // Draw corner arcs
        draw_arc(x + radius, y + radius, radius, M_PI, 3 * M_PI / 2, color);
        draw_arc(x + w - radius - 1, y + radius, radius, 3 * M_PI / 2, 2 * M_PI, color);
        draw_arc(x + w - radius - 1, y + h - radius - 1, radius, 0, M_PI / 2, color);
        draw_arc(x + radius, y + h - radius - 1, radius, M_PI / 2, M_PI, color);
        
        // Draw straight lines
        draw_line(x + radius, y, x + w - radius - 1, y, color);
        draw_line(x + w - 1, y + radius, x + w - 1, y + h - radius - 1, color);
        draw_line(x + w - radius - 1, y + h - 1, x + radius, y + h - 1, color);
        draw_line(x, y + h - radius - 1, x, y + radius, color);
    }
}

void Graphics::draw_circle(int cx, int cy, int radius, const Color& color, bool filled) {
    if (!target_buffer) return;
    
    if (filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    set_pixel(cx + x, cy + y, color);
                }
            }
        }
    } else {
        // Midpoint circle algorithm
        int x = 0;
        int y = radius;
        int d = 1 - radius;
        
        while (x <= y) {
            set_pixel(cx + x, cy + y, color);
            set_pixel(cx - x, cy + y, color);
            set_pixel(cx + x, cy - y, color);
            set_pixel(cx - x, cy - y, color);
            set_pixel(cx + y, cy + x, color);
            set_pixel(cx - y, cy + x, color);
            set_pixel(cx + y, cy - x, color);
            set_pixel(cx - y, cy - x, color);
            
            if (d < 0) {
                d += 2 * x + 3;
            } else {
                d += 2 * (x - y) + 5;
                y--;
            }
            x++;
        }
    }
}

void Graphics::draw_ellipse(int cx, int cy, int rx, int ry, const Color& color, bool filled) {
    if (!target_buffer) return;
    
    if (filled) {
        for (int y = -ry; y <= ry; y++) {
            for (int x = -rx; x <= rx; x++) {
                if ((x * x) / (float)(rx * rx) + (y * y) / (float)(ry * ry) <= 1.0f) {
                    set_pixel(cx + x, cy + y, color);
                }
            }
        }
    } else {
        // Simple ellipse outline using parametric equations
        const int steps = 360;
        for (int i = 0; i < steps; i++) {
            float angle = (2 * M_PI * i) / steps;
            int x = cx + static_cast<int>(rx * std::cos(angle));
            int y = cy + static_cast<int>(ry * std::sin(angle));
            set_pixel(x, y, color);
        }
    }
}

void Graphics::draw_arc(int cx, int cy, int radius, float start_angle, float end_angle, const Color& color) {
    if (!target_buffer) return;
    
    const int steps = 100;
    float angle_step = (end_angle - start_angle) / steps;
    
    for (int i = 0; i <= steps; i++) {
        float angle = start_angle + i * angle_step;
        int x = cx + static_cast<int>(radius * std::cos(angle));
        int y = cy + static_cast<int>(radius * std::sin(angle));
        set_pixel(x, y, color);
    }
}

void Graphics::draw_polygon(const Polygon& polygon) {
    if (!target_buffer || polygon.vertices.size() < 3) return;
    
    if (polygon.filled) {
        draw_filled_polygon_scanline(polygon);
    } else {
        draw_polygon_outline(polygon);
    }
}

void Graphics::draw_filled_polygon_scanline(const Polygon& polygon) {
    // Simple scanline polygon fill algorithm
    std::vector<Vector2> vertices = polygon.vertices;
    
    // Find bounding box
    float min_y = vertices[0].y, max_y = vertices[0].y;
    for (const auto& vertex : vertices) {
        min_y = std::min(min_y, vertex.y);
        max_y = std::max(max_y, vertex.y);
    }
    
    // For each scanline
    for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); y++) {
        std::vector<float> intersections;
        
        // Find intersections with polygon edges
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t j = (i + 1) % vertices.size();
            
            if ((vertices[i].y <= y && vertices[j].y > y) || 
                (vertices[j].y <= y && vertices[i].y > y)) {
                
                float x_intersect = vertices[i].x + 
                    (y - vertices[i].y) / (vertices[j].y - vertices[i].y) * 
                    (vertices[j].x - vertices[i].x);
                intersections.push_back(x_intersect);
            }
        }
        
        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
        
        // Fill between pairs of intersections
        for (size_t i = 0; i < intersections.size(); i += 2) {
            if (i + 1 < intersections.size()) {
                int x1 = static_cast<int>(intersections[i]);
                int x2 = static_cast<int>(intersections[i + 1]);
                for (int x = x1; x <= x2; x++) {
                    set_pixel(x, y, polygon.color);
                }
            }
        }
    }
}

void Graphics::draw_polygon_outline(const Polygon& polygon) {
    for (size_t i = 0; i < polygon.vertices.size(); i++) {
        size_t j = (i + 1) % polygon.vertices.size();
        draw_line(static_cast<int>(polygon.vertices[i].x), static_cast<int>(polygon.vertices[i].y),
                 static_cast<int>(polygon.vertices[j].x), static_cast<int>(polygon.vertices[j].y),
                 polygon.color);
    }
}

void Graphics::draw_bezier_curve(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Color& color) {
    if (!target_buffer) return;
    
    const int steps = 100;
    Vector2 prev_point = p0;
    
    for (int i = 1; i <= steps; i++) {
        float t = static_cast<float>(i) / steps;
        float u = 1 - t;
        
        Vector2 point = p0 * (u * u * u) + 
                       p1 * (3 * u * u * t) + 
                       p2 * (3 * u * t * t) + 
                       p3 * (t * t * t);
        
        draw_line(static_cast<int>(prev_point.x), static_cast<int>(prev_point.y),
                 static_cast<int>(point.x), static_cast<int>(point.y), color);
        prev_point = point;
    }
}

void Graphics::draw_gradient_rect(const Rect& rect, const Gradient& gradient) {
    if (!target_buffer) return;
    
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            Color color = gradient.get_color_at(Vector2(x, y));
            set_pixel(x, y, color);
        }
    }
}

void Graphics::draw_gradient_circle(int cx, int cy, int radius, const Color& center_color, const Color& edge_color) {
    if (!target_buffer) return;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float distance = std::sqrt(x * x + y * y);
            if (distance <= radius) {
                float t = distance / radius;
                Color color = interpolate_color(center_color, edge_color, t);
                set_pixel(cx + x, cy + y, color);
            }
        }
    }
}

void Graphics::draw_text(int x, int y, const std::string& text, const Color& color) {
    if (!target_buffer) return;
    target_buffer->draw_text(x, y, text, color);
}

void Graphics::draw_text_centered(const Rect& rect, const std::string& text, const Color& color) {
    // Simple centering - assumes 8x8 character size
    int text_width = text.length() * 8;
    int text_height = 8;
    
    int x = rect.x + (rect.width - text_width) / 2;
    int y = rect.y + (rect.height - text_height) / 2;
    
    draw_text(x, y, text, color);
}

void Graphics::draw_text_with_background(int x, int y, const std::string& text, const Color& text_color, const Color& bg_color) {
    // Draw background rectangle
    int text_width = text.length() * 8;
    int text_height = 8;
    draw_rect(Rect(x, y, text_width, text_height), bg_color, true);
    
    // Draw text
    draw_text(x, y, text, text_color);
}

Color Graphics::blend_colors(const Color& src, const Color& dst, float alpha) const {
    alpha = std::max(0.0f, std::min(1.0f, alpha));
    
    uint8_t r = static_cast<uint8_t>(src.r * alpha + dst.r * (1 - alpha));
    uint8_t g = static_cast<uint8_t>(src.g * alpha + dst.g * (1 - alpha));
    uint8_t b = static_cast<uint8_t>(src.b * alpha + dst.b * (1 - alpha));
    uint8_t a = static_cast<uint8_t>(src.a * alpha + dst.a * (1 - alpha));
    
    return Color(r, g, b, a);
}

void Graphics::draw_rect_alpha(const Rect& rect, const Color& color, float alpha) {
    if (!target_buffer) return;
    
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            if (is_point_in_clip_rect(x, y)) {
                Color existing = target_buffer->get_pixel(x, y);
                Color blended = blend_colors(color, existing, alpha);
                target_buffer->set_pixel(x, y, blended);
            }
        }
    }
}

void Graphics::draw_circle_alpha(int cx, int cy, int radius, const Color& color, float alpha) {
    if (!target_buffer) return;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                int px = cx + x, py = cy + y;
                if (is_point_in_clip_rect(px, py)) {
                    Color existing = target_buffer->get_pixel(px, py);
                    Color blended = blend_colors(color, existing, alpha);
                    target_buffer->set_pixel(px, py, blended);
                }
            }
        }
    }
}

// Static utility functions
Color Graphics::interpolate_color(const Color& c1, const Color& c2, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    
    uint8_t r = static_cast<uint8_t>(c1.r + (c2.r - c1.r) * t);
    uint8_t g = static_cast<uint8_t>(c1.g + (c2.g - c1.g) * t);
    uint8_t b = static_cast<uint8_t>(c1.b + (c2.b - c1.b) * t);
    uint8_t a = static_cast<uint8_t>(c1.a + (c2.a - c1.a) * t);
    
    return Color(r, g, b, a);
}

float Graphics::distance(const Vector2& p1, const Vector2& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Graphics::angle_between(const Vector2& p1, const Vector2& p2) {
    return std::atan2(p2.y - p1.y, p2.x - p1.x);
}

bool Graphics::point_in_rect(const Vector2& point, const Rect& rect) {
    return point.x >= rect.x && point.x < rect.x + rect.width &&
           point.y >= rect.y && point.y < rect.y + rect.height;
}

bool Graphics::point_in_circle(const Vector2& point, const Vector2& center, float radius) {
    return distance(point, center) <= radius;
}

void Graphics::draw_checkered_pattern(const Rect& rect, const Color& color1, const Color& color2, int square_size) {
    if (!target_buffer) return;
    
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            int check_x = (x - rect.x) / square_size;
            int check_y = (y - rect.y) / square_size;
            Color color = ((check_x + check_y) % 2 == 0) ? color1 : color2;
            set_pixel(x, y, color);
        }
    }
}

void Graphics::draw_grid(const Rect& rect, int spacing, const Color& color) {
    if (!target_buffer) return;
    
    // Draw vertical lines
    for (int x = rect.x; x <= rect.x + rect.width; x += spacing) {
        draw_line(x, rect.y, x, rect.y + rect.height, color);
    }
    
    // Draw horizontal lines
    for (int y = rect.y; y <= rect.y + rect.height; y += spacing) {
        draw_line(rect.x, y, rect.x + rect.width, y, color);
    }
}

void Graphics::draw_coordinate_axes(const Rect& rect, const Color& color) {
    if (!target_buffer) return;
    
    int center_x = rect.x + rect.width / 2;
    int center_y = rect.y + rect.height / 2;
    
    // Draw X axis
    draw_line(rect.x, center_y, rect.x + rect.width, center_y, color);
    
    // Draw Y axis
    draw_line(center_x, rect.y, center_x, rect.y + rect.height, color);
}
