#include "rosettelab/palette/gpl_palette.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    using rosettelab::document::RgbaColor;
    const std::vector<RgbaColor> source{{1, 0, 0, 0.5}, {0.1, 0.2, 0.3, 1}};
    const auto encoded = rosettelab::palette::serialize_gpl(source, "Test palette");
    if (encoded.find("#RRGGBBAA=FF000080") == std::string::npos)
        throw std::runtime_error("GPL extension must use RGBA order");
    const auto decoded = rosettelab::palette::parse_gpl(encoded);
    if (decoded.size() != 2 || std::abs(decoded[0].alpha - 128.0 / 255.0) > 1e-12)
        throw std::runtime_error("GPL alpha extension should round-trip");
    const auto standard = rosettelab::palette::parse_gpl(
        "GIMP Palette\nName: Standard\n#\n1 2 3 Opaque\n");
    if (standard[0].alpha != 1.0) throw std::runtime_error("standard GPL colors should be opaque");
    std::cout << "All RosetteLab GPL palette tests passed\n";
}
