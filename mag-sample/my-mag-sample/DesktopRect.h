#pragma once

#include <stdint.h>
#include <algorithm>

// A vector in the 2D integer space. E.g. can be used to represent screen DPI.
class DesktopVector {
public:
    DesktopVector() : x_(0), y_(0) {}
    DesktopVector(int32_t x, int32_t y) : x_(x), y_(y) {}

    int32_t x() const { return x_; }
    int32_t y() const { return y_; }
    bool is_zero() const { return x_ == 0 && y_ == 0; }

    bool equals(const DesktopVector& other) const {
        return x_ == other.x_ && y_ == other.y_;
    }

    void set(int32_t x, int32_t y) {
        x_ = x;
        y_ = y;
    }

    DesktopVector add(const DesktopVector& other) const {
        return DesktopVector(x() + other.x(), y() + other.y());
    }
    DesktopVector subtract(const DesktopVector& other) const {
        return DesktopVector(x() - other.x(), y() - other.y());
    }

    DesktopVector operator-() const { return DesktopVector(-x_, -y_); }

private:
    int32_t x_;
    int32_t y_;
};

// Type used to represent screen/window size.
class DesktopSize {
public:
    DesktopSize() : width_(0), height_(0) {}
    DesktopSize(int32_t width, int32_t height) : width_(width), height_(height) {}

    int32_t width() const { return width_; }
    int32_t height() const { return height_; }

    bool is_empty() const { return width_ <= 0 || height_ <= 0; }

    bool equals(const DesktopSize& other) const {
        return width_ == other.width_ && height_ == other.height_;
    }

    void set(int32_t width, int32_t height) {
        width_ = width;
        height_ = height;
    }

private:
    int32_t width_;
    int32_t height_;
};

// Represents a rectangle on the screen.
class DesktopRect {
public:
    static DesktopRect MakeSize(const DesktopSize& size) {
        return DesktopRect(0, 0, size.width(), size.height());
    }
    static DesktopRect MakeWH(int32_t width, int32_t height) {
        return DesktopRect(0, 0, width, height);
    }
    static DesktopRect MakeXYWH(int32_t x,
        int32_t y,
        int32_t width,
        int32_t height) {
        return DesktopRect(x, y, x + width, y + height);
    }
    static DesktopRect MakeLTRB(int32_t left,
        int32_t top,
        int32_t right,
        int32_t bottom) {
        return DesktopRect(left, top, right, bottom);
    }
    static DesktopRect MakeOriginSize(const DesktopVector& origin,
        const DesktopSize& size) {
        return MakeXYWH(origin.x(), origin.y(), size.width(), size.height());
    }

#if _WIN32
    static DesktopRect MakeRECT(RECT rect)
    {
        return DesktopRect(rect.left, rect.top, rect.right, rect.bottom);
    }

    static DesktopRect MakeRECT(RECT iRect, double dpi)
    {
        DesktopRect rect = MakeRECT(iRect);
        rect.set_width((int32_t)(rect.width() / dpi));
        rect.set_height((int32_t)(rect.height() / dpi));
        return rect;
    }
#endif

    DesktopRect() : left_(0), top_(0), right_(0), bottom_(0) {}

    int32_t left() const { return left_; }
    int32_t top() const { return top_; }
    int32_t right() const { return right_; }
    int32_t bottom() const { return bottom_; }
    int32_t width() const { return right_ - left_; }
    int32_t height() const { return bottom_ - top_; }

    void set_width(int32_t width) { right_ = left_ + width; }
    void set_height(int32_t height) { bottom_ = top_ + height; }

    DesktopVector top_left() const { return DesktopVector(left_, top_); }
    DesktopSize size() const { return DesktopSize(width(), height()); }

    bool is_empty() const { return left_ >= right_ || top_ >= bottom_; }

    bool operator==(const DesktopRect& other) const {
        return left_ == other.left_ && top_ == other.top_ &&
            right_ == other.right_ && bottom_ == other.bottom_;
    }

    #ifdef min
    #undef min
    #undef max
    void DesktopRect::IntersectWith(const DesktopRect &rect)
    {
        left_ = std::max(left(), rect.left());
        top_ = std::max(top(), rect.top());
        right_ = std::min(right(), rect.right());
        bottom_ = std::min(bottom(), rect.bottom());
        if (is_empty()) {
            left_ = 0;
            top_ = 0;
            right_ = 0;
            bottom_ = 0;
        }
    }
    #endif //min


private:
    DesktopRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
        : left_(left), top_(top), right_(right), bottom_(bottom) {}

    int32_t left_;
    int32_t top_;
    int32_t right_;
    int32_t bottom_;
};