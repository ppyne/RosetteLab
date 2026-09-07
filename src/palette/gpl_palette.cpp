#include "rosettelab/palette/gpl_palette.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace rosettelab::palette {
namespace {

int component(const double value)
{
    return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
}

std::string rgba_hex(const document::RgbaColor& color)
{
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0')
        << std::setw(2) << component(color.red)
        << std::setw(2) << component(color.green)
        << std::setw(2) << component(color.blue)
        << std::setw(2) << component(color.alpha);
    return out.str();
}

double byte_component(const int value)
{
    if (value < 0 || value > 255) throw std::invalid_argument("GPL color component is outside 0..255");
    return static_cast<double>(value) / 255.0;
}

} // namespace

std::string serialize_gpl(
    const std::vector<document::RgbaColor>& colors, const std::string_view name)
{
    std::string safe_name(name);
    std::replace(safe_name.begin(), safe_name.end(), '\n', ' ');
    std::replace(safe_name.begin(), safe_name.end(), '\r', ' ');
    std::ostringstream out;
    out << "GIMP Palette\nName: " << safe_name << "\nColumns: 8\n#\n";
    for (const auto& color : colors) {
        out << std::setw(3) << component(color.red) << ' '
            << std::setw(3) << component(color.green) << ' '
            << std::setw(3) << component(color.blue)
            << "  #RRGGBBAA=" << rgba_hex(color) << '\n';
    }
    return out.str();
}

std::vector<document::RgbaColor> parse_gpl(const std::string_view data)
{
    if (data.size() > 4 * 1024 * 1024) throw std::invalid_argument("GPL palette exceeds 4 MB");
    std::istringstream input{std::string(data)};
    std::string line;
    if (!std::getline(input, line) || (line != "GIMP Palette" && line != "GIMP Palette\r"))
        throw std::invalid_argument("File is not a GIMP GPL palette");
    std::vector<document::RgbaColor> colors;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line.starts_with("Name:") || line.starts_with("Columns:"))
            continue;
        std::istringstream row(line);
        int red{}, green{}, blue{};
        if (!(row >> red >> green >> blue)) continue;
        document::RgbaColor color{byte_component(red), byte_component(green), byte_component(blue), 1.0};
        const auto marker = line.find("#RRGGBBAA=");
        if (marker != std::string::npos) {
            const auto value = line.substr(marker + 10, 8);
            if (value.size() != 8) throw std::invalid_argument("Invalid RosetteLab GPL alpha extension");
            unsigned long packed{};
            std::size_t consumed{};
            try { packed = std::stoul(value, &consumed, 16); }
            catch (...) { throw std::invalid_argument("Invalid RosetteLab GPL RGBA value"); }
            if (consumed != 8) throw std::invalid_argument("Invalid RosetteLab GPL RGBA value");
            color = {
                static_cast<double>((packed >> 24) & 0xff) / 255.0,
                static_cast<double>((packed >> 16) & 0xff) / 255.0,
                static_cast<double>((packed >> 8) & 0xff) / 255.0,
                static_cast<double>(packed & 0xff) / 255.0};
        }
        colors.push_back(color);
        if (colors.size() > 10000) throw std::invalid_argument("GPL palette contains too many colors");
    }
    if (colors.empty()) throw std::invalid_argument("GPL palette contains no colors");
    return colors;
}

} // namespace rosettelab::palette
