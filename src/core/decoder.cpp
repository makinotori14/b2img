#include "b2img/core/decoder.hpp"

#include "b2img/core/analyzer.hpp"
#include "b2img/core/encoding.hpp"
#include "b2img/core/palette.hpp"

#include <array>
#include <stdexcept>
#include <vector>

namespace b2img::core {

namespace {

class BitStreamView {
public:
    BitStreamView(const std::vector<std::uint8_t>& bytes, BitOrder bit_order)
        : m_bytes(bytes), m_bit_order(bit_order) {}

    [[nodiscard]] bool has_range(std::size_t bit_offset, std::uint8_t bit_count) const noexcept {
        return bit_count > 0 &&
            bit_count <= 32 &&
            bit_offset <= m_bytes.size() * 8 &&
            bit_count <= (m_bytes.size() * 8 - bit_offset);
    }

    [[nodiscard]] std::uint32_t read(std::size_t bit_offset, std::uint8_t bit_count) const {
        if (bit_count == 0 || bit_count > 32) {
            throw std::invalid_argument("bit_count must be in range 1..32");
        }

        if (!has_range(bit_offset, bit_count)) {
            throw std::out_of_range("attempted to read beyond the end of the source buffer");
        }

        std::uint32_t value = 0;

        for (std::uint8_t bit_index = 0; bit_index < bit_count; ++bit_index) {
            const std::size_t absolute_bit = bit_offset + bit_index;
            const std::size_t byte_index = absolute_bit / 8;
            const std::size_t bit_in_byte = absolute_bit % 8;
            const std::uint8_t current = m_bytes[byte_index];

            std::uint8_t bit = 0;
            if (m_bit_order == BitOrder::msb_first) {
                bit = static_cast<std::uint8_t>((current >> (7U - bit_in_byte)) & 0x01U);
            } else {
                bit = static_cast<std::uint8_t>((current >> bit_in_byte) & 0x01U);
            }

            value = static_cast<std::uint32_t>((value << 1U) | bit);
        }

        return value;
    }

private:
    const std::vector<std::uint8_t>& m_bytes;
    BitOrder m_bit_order;
};

Rgb rgb_from_index(std::uint32_t index, const std::vector<Rgb>& palette) {
    if (index >= palette.size()) {
        throw std::invalid_argument("palette does not contain enough colors for the decoded index range");
    }

    return palette[index];
}

std::size_t channel_index(std::string_view channels, char channel) {
    const auto index = channels.find(channel);
    if (index == std::string_view::npos) {
        throw std::invalid_argument("pixel format is missing one of the RGB channels");
    }

    return index;
}

void decode_linear_indexed(Image& image, const BitStreamView& bits, const RenderRequest& request, const std::vector<Rgb>& palette) {
    std::size_t cursor = request.bit_offset;

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;

        for (std::size_t x = 0; x < request.width; ++x) {
            if (bits.has_range(cursor, request.bits_per_pixel)) {
                const std::uint32_t index = bits.read(cursor, request.bits_per_pixel);
                image.at(x, target_y) = rgb_from_index(index, palette);
            }
            cursor += request.bits_per_pixel;
        }
    }
}

std::array<std::size_t, 4> plane_bases(const RenderRequest& request) {
    const std::size_t plane_bits = request.width * request.height;

    switch (request.layout) {
        case Layout::planar_image:
            return {
                request.bit_offset,
                request.bit_offset + plane_bits,
                request.bit_offset + plane_bits * 2U,
                request.bit_offset + plane_bits * 3U,
            };

        case Layout::planar_row:
        case Layout::planar_pixel:
        case Layout::linear:
            return {0, 0, 0, 0};
    }

    throw std::invalid_argument("unsupported layout");
}

void decode_planar_image(Image& image, const BitStreamView& bits, const RenderRequest& request, const std::vector<Rgb>& palette) {
    const auto base = plane_bases(request);

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;

        for (std::size_t x = 0; x < request.width; ++x) {
            const std::size_t pixel_index = y * request.width + x;
            if (bits.has_range(base[0] + pixel_index, 1) &&
                bits.has_range(base[1] + pixel_index, 1) &&
                bits.has_range(base[2] + pixel_index, 1) &&
                bits.has_range(base[3] + pixel_index, 1)) {
                const std::uint32_t b0 = bits.read(base[0] + pixel_index, 1);
                const std::uint32_t b1 = bits.read(base[1] + pixel_index, 1);
                const std::uint32_t b2 = bits.read(base[2] + pixel_index, 1);
                const std::uint32_t b3 = bits.read(base[3] + pixel_index, 1);
                const std::uint32_t index = static_cast<std::uint32_t>(b0 | (b1 << 1U) | (b2 << 2U) | (b3 << 3U));
                image.at(x, target_y) = rgb_from_index(index, palette);
            }
        }
    }
}

void decode_planar_row(Image& image, const BitStreamView& bits, const RenderRequest& request, const std::vector<Rgb>& palette) {
    const std::size_t row_bits = request.width;

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;
        const std::size_t row_base = request.bit_offset + y * row_bits * 4U;

        for (std::size_t x = 0; x < request.width; ++x) {
            if (bits.has_range(row_base + x, 1) &&
                bits.has_range(row_base + row_bits + x, 1) &&
                bits.has_range(row_base + row_bits * 2U + x, 1) &&
                bits.has_range(row_base + row_bits * 3U + x, 1)) {
                const std::uint32_t b0 = bits.read(row_base + x, 1);
                const std::uint32_t b1 = bits.read(row_base + row_bits + x, 1);
                const std::uint32_t b2 = bits.read(row_base + row_bits * 2U + x, 1);
                const std::uint32_t b3 = bits.read(row_base + row_bits * 3U + x, 1);
                const std::uint32_t index = static_cast<std::uint32_t>(b0 | (b1 << 1U) | (b2 << 2U) | (b3 << 3U));
                image.at(x, target_y) = rgb_from_index(index, palette);
            }
        }
    }
}

void decode_planar_pixel(Image& image, const BitStreamView& bits, const RenderRequest& request, const std::vector<Rgb>& palette) {
    std::size_t cursor = request.bit_offset;

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;

        for (std::size_t x = 0; x < request.width; ++x) {
            if (bits.has_range(cursor, 1) &&
                bits.has_range(cursor + 1U, 1) &&
                bits.has_range(cursor + 2U, 1) &&
                bits.has_range(cursor + 3U, 1)) {
                const std::uint32_t b0 = bits.read(cursor, 1);
                const std::uint32_t b1 = bits.read(cursor + 1U, 1);
                const std::uint32_t b2 = bits.read(cursor + 2U, 1);
                const std::uint32_t b3 = bits.read(cursor + 3U, 1);
                const std::uint32_t index = static_cast<std::uint32_t>(b0 | (b1 << 1U) | (b2 << 2U) | (b3 << 3U));
                image.at(x, target_y) = rgb_from_index(index, palette);
            }
            cursor += 4U;
        }
    }
}

void decode_indexed(Image& image, const std::vector<std::uint8_t>& bytes, const RenderRequest& request) {
    const BitStreamView bits(bytes, request.bit_order);
    const auto palette = Palette::make(request.palette, static_cast<std::size_t>(1U << request.bits_per_pixel));

    switch (request.layout) {
        case Layout::linear:
            decode_linear_indexed(image, bits, request, palette);
            return;

        case Layout::planar_image:
            decode_planar_image(image, bits, request, palette);
            return;

        case Layout::planar_row:
            decode_planar_row(image, bits, request, palette);
            return;

        case Layout::planar_pixel:
            decode_planar_pixel(image, bits, request, palette);
            return;
    }
}

void decode_packed24(Image& image, const std::vector<std::uint8_t>& bytes, const RenderRequest& request, std::string_view channels) {
    const BitStreamView bits(bytes, BitOrder::msb_first);
    std::size_t cursor = request.bit_offset;
    const auto red_index = channel_index(channels, 'r');
    const auto green_index = channel_index(channels, 'g');
    const auto blue_index = channel_index(channels, 'b');

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;

        for (std::size_t x = 0; x < request.width; ++x) {
            if (bits.has_range(cursor, 24)) {
                const std::array<std::uint8_t, 3> source {
                    static_cast<std::uint8_t>(bits.read(cursor, 8)),
                    static_cast<std::uint8_t>(bits.read(cursor + 8U, 8)),
                    static_cast<std::uint8_t>(bits.read(cursor + 16U, 8)),
                };
                const auto red = source[red_index];
                const auto green = source[green_index];
                const auto blue = source[blue_index];
                image.at(x, target_y) = Rgb{red, green, blue};
            }
            cursor += 24U;
        }
    }
}

Rgb decode_packed32_pixel(const BitStreamView& bits, std::size_t cursor, std::string_view channels) {
    const std::array<std::uint8_t, 4> source {
        static_cast<std::uint8_t>(bits.read(cursor, 8)),
        static_cast<std::uint8_t>(bits.read(cursor + 8U, 8)),
        static_cast<std::uint8_t>(bits.read(cursor + 16U, 8)),
        static_cast<std::uint8_t>(bits.read(cursor + 24U, 8)),
    };

    const auto red_index = channel_index(channels, 'r');
    const auto green_index = channel_index(channels, 'g');
    const auto blue_index = channel_index(channels, 'b');

    return Rgb{source[red_index], source[green_index], source[blue_index]};
}

void decode_packed32(Image& image, const std::vector<std::uint8_t>& bytes, const RenderRequest& request, std::string_view channels) {
    const BitStreamView bits(bytes, BitOrder::msb_first);
    std::size_t cursor = request.bit_offset;

    for (std::size_t y = 0; y < request.height; ++y) {
        const std::size_t target_y = request.flip_vertical ? (request.height - 1U - y) : y;

        for (std::size_t x = 0; x < request.width; ++x) {
            if (bits.has_range(cursor, 32)) {
                image.at(x, target_y) = decode_packed32_pixel(bits, cursor, channels);
            }
            cursor += 32U;
        }
    }
}

Image rotate_image(const Image& source, int turns) {
    const int normalized_turns = ((turns % 4) + 4) % 4;
    if (normalized_turns == 0) {
        return source;
    }

    if (normalized_turns == 2) {
        Image rotated(source.width(), source.height());
        for (std::size_t y = 0; y < source.height(); ++y) {
            for (std::size_t x = 0; x < source.width(); ++x) {
                rotated.at(source.width() - 1U - x, source.height() - 1U - y) = source.at(x, y);
            }
        }
        return rotated;
    }

    Image rotated(source.height(), source.width());
    for (std::size_t y = 0; y < source.height(); ++y) {
        for (std::size_t x = 0; x < source.width(); ++x) {
            if (normalized_turns == 1) {
                rotated.at(source.height() - 1U - y, x) = source.at(x, y);
            } else {
                rotated.at(y, source.width() - 1U - x) = source.at(x, y);
            }
        }
    }

    return rotated;
}

}  // namespace

Image Decoder::decode(const std::vector<std::uint8_t>& bytes, const RenderRequest& request) {
    const auto encoding = find_encoding_spec(request.encoding_name);
    if (!encoding.has_value()) {
        throw std::invalid_argument("unsupported encoding: " + request.encoding_name);
    }

    Image image(request.width, request.height);
    for (std::size_t y = 0; y < request.height; ++y) {
        for (std::size_t x = 0; x < request.width; ++x) {
            image.at(x, y) = Rgb{255, 255, 255};
        }
    }

    switch (request.encoding_kind) {
        case EncodingKind::indexed:
            decode_indexed(image, bytes, request);
            break;

        case EncodingKind::packed24:
            decode_packed24(image, bytes, request, encoding->channels);
            break;

        case EncodingKind::packed32:
            decode_packed32(image, bytes, request, encoding->channels);
            break;
    }

    return rotate_image(image, request.rotation_turns);
}

Image::Image(std::size_t width, std::size_t height)
    : m_width(width), m_height(height), m_pixels(width * height) {}

std::size_t Image::width() const noexcept {
    return m_width;
}

std::size_t Image::height() const noexcept {
    return m_height;
}

bool Image::empty() const noexcept {
    return m_pixels.empty();
}

Rgb& Image::at(std::size_t x, std::size_t y) {
    return m_pixels.at(y * m_width + x);
}

const Rgb& Image::at(std::size_t x, std::size_t y) const {
    return m_pixels.at(y * m_width + x);
}

const std::vector<Rgb>& Image::pixels() const noexcept {
    return m_pixels;
}

}  // namespace b2img::core
