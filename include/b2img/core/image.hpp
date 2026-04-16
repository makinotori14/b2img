#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace b2img::core {

struct Rgb {
    std::uint8_t red {};
    std::uint8_t green {};
    std::uint8_t blue {};
};

class Image {
public:
    Image() = default;
    Image(std::size_t width, std::size_t height);

    [[nodiscard]] std::size_t width() const noexcept;
    [[nodiscard]] std::size_t height() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    Rgb& at(std::size_t x, std::size_t y);
    [[nodiscard]] const Rgb& at(std::size_t x, std::size_t y) const;
    [[nodiscard]] const std::vector<Rgb>& pixels() const noexcept;

private:
    std::size_t m_width {};
    std::size_t m_height {};
    std::vector<Rgb> m_pixels;
};

}  // namespace b2img::core
