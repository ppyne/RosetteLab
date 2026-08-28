#include "rosettelab/pdf/pdf_serializer.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void require(const bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool contains(const std::string& value, const std::string& expected)
{
    return value.find(expected) != std::string::npos;
}

void require_valid_xref(const std::string& pdf)
{
    const auto marker = pdf.rfind("startxref\n");
    require(marker != std::string::npos, "startxref should exist");
    const auto offset_start = marker + std::string("startxref\n").size();
    const auto offset_end = pdf.find('\n', offset_start);
    const auto xref_offset = static_cast<std::size_t>(std::stoull(
        pdf.substr(offset_start, offset_end - offset_start)));
    require(pdf.substr(xref_offset, 5) == "xref\n", "startxref should point to xref");

    std::istringstream input(pdf.substr(xref_offset));
    std::string line;
    std::getline(input, line);
    std::getline(input, line);
    std::size_t first = 0;
    std::size_t count = 0;
    std::istringstream section(line);
    section >> first >> count;
    require(first == 0 && count > 1, "xref should describe all objects from zero");
    std::getline(input, line); // free object
    for (std::size_t object = 1; object < count; ++object) {
        std::getline(input, line);
        const auto offset = static_cast<std::size_t>(std::stoull(line.substr(0, 10)));
        require(pdf.substr(offset, std::to_string(object).size() + 6)
                == std::to_string(object) + " 0 obj",
            "xref entry should point to its object");
    }
}

} // namespace

int main(const int argc, char** argv)
{
    rosettelab::document::Document document;
    document.settings().background.alpha = 0.5;
    auto& layer = document.add_polar_rose();
    layer.appearance.fill_enabled = true;
    layer.appearance.fill.alpha = 0.4;
    layer.appearance.stroke.alpha = 0.7;
    layer.appearance.opacity = 0.6;
    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    layer.copies.count = 3;
    layer.copies.rotation_step_degrees = 30.0;

    const auto rgb = rosettelab::pdf::serialize_vector_pdf(document);
    require(rgb.starts_with("%PDF-1.7"), "PDF 1.7 header should be emitted");
    require(contains(rgb, "/Subtype /Form"), "a layer should be a Form XObject");
    require(contains(rgb, "/S /Transparency /I true"), "a layer should be an isolated transparency group");
    require(contains(rgb, "/BM /Multiply"), "native blend mode should be emitted");
    require(contains(rgb, "/CA 0.6 /ca 0.6"), "layer opacity should apply to the group");
    require(contains(rgb, "/CA 0.7 /ca 0.4"), "stroke and fill alpha should remain distinct");
    require(contains(rgb, " c\n"), "curve geometry should use cubic Bezier operators");
    require(contains(rgb, "B*\n"), "even-odd fill and stroke should use B*");
    require(!contains(rgb, "/Subtype /Image"), "native export should contain no raster image");
    require(contains(rgb, "xref\n0 "), "a cross-reference table should be emitted");
    require(contains(rgb, "startxref\n"), "the cross-reference offset should be emitted");
    require_valid_xref(rgb);

    rosettelab::pdf::ExportOptions options;
    options.color_model = rosettelab::pdf::ColorModel::Cmyk;
    const auto cmyk = rosettelab::pdf::serialize_vector_pdf(document, options);
    require(contains(cmyk, "/CS /DeviceCMYK"), "CMYK page transparency group should be declared");
    require(contains(cmyk, " K\n"), "CMYK stroke operator should be emitted");
    require(contains(cmyk, " k\n"), "CMYK fill operator should be emitted");

    if (argc == 2) {
        std::ofstream output(argv[1], std::ios::binary);
        output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        require(output.good(), "requested PDF fixture should be written");
    }

    return EXIT_SUCCESS;
}
