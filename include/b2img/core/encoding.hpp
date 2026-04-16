#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace b2img::core {

enum class EncodingKind {
    indexed,
    packed24,
    packed32
};

struct EncodingSpec {
    EncodingKind kind;
    std::string_view name;
    std::string_view channels;
};

inline constexpr std::array<EncodingSpec, 31> kEncodingSpecs {{
    {EncodingKind::indexed, "indexed", ""},
    {EncodingKind::packed24, "rgb24", "rgb"},
    {EncodingKind::packed24, "rbg24", "rbg"},
    {EncodingKind::packed24, "grb24", "grb"},
    {EncodingKind::packed24, "gbr24", "gbr"},
    {EncodingKind::packed24, "brg24", "brg"},
    {EncodingKind::packed24, "bgr24", "bgr"},
    {EncodingKind::packed32, "rgba32", "rgba"},
    {EncodingKind::packed32, "rgab32", "rgab"},
    {EncodingKind::packed32, "rbga32", "rbga"},
    {EncodingKind::packed32, "rbag32", "rbag"},
    {EncodingKind::packed32, "ragb32", "ragb"},
    {EncodingKind::packed32, "rabg32", "rabg"},
    {EncodingKind::packed32, "grba32", "grba"},
    {EncodingKind::packed32, "grab32", "grab"},
    {EncodingKind::packed32, "gbra32", "gbra"},
    {EncodingKind::packed32, "gbar32", "gbar"},
    {EncodingKind::packed32, "garb32", "garb"},
    {EncodingKind::packed32, "gabr32", "gabr"},
    {EncodingKind::packed32, "brga32", "brga"},
    {EncodingKind::packed32, "brag32", "brag"},
    {EncodingKind::packed32, "bgra32", "bgra"},
    {EncodingKind::packed32, "bgar32", "bgar"},
    {EncodingKind::packed32, "barg32", "barg"},
    {EncodingKind::packed32, "bagr32", "bagr"},
    {EncodingKind::packed32, "argb32", "argb"},
    {EncodingKind::packed32, "arbg32", "arbg"},
    {EncodingKind::packed32, "agrb32", "agrb"},
    {EncodingKind::packed32, "agbr32", "agbr"},
    {EncodingKind::packed32, "abrg32", "abrg"},
    {EncodingKind::packed32, "abgr32", "abgr"},
}};

inline constexpr std::array<const char*, 32> kEncodingNames {{
    "indexed",
    "rgb24",
    "rbg24",
    "grb24",
    "gbr24",
    "brg24",
    "bgr24",
    "rgba32",
    "rgab32",
    "rbga32",
    "rbag32",
    "ragb32",
    "rabg32",
    "grba32",
    "grab32",
    "gbra32",
    "gbar32",
    "garb32",
    "gabr32",
    "brga32",
    "brag32",
    "bgra32",
    "bgar32",
    "barg32",
    "bagr32",
    "argb32",
    "arbg32",
    "agrb32",
    "agbr32",
    "abrg32",
    "abgr32",
    nullptr,
}};

inline std::optional<EncodingSpec> find_encoding_spec(std::string_view name) {
    for (const auto& spec : kEncodingSpecs) {
        if (spec.name == name) {
            return spec;
        }
    }

    return std::nullopt;
}

inline std::string encoding_names_joined() {
    std::string result;

    for (std::size_t index = 0; index < kEncodingSpecs.size(); ++index) {
        if (index != 0U) {
            result += '|';
        }
        result += kEncodingSpecs[index].name;
    }

    return result;
}

}  // namespace b2img::core
