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

std::size_t count_occurrences(const std::string& value, const std::string& expected)
{
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = value.find(expected, position)) != std::string::npos) {
        ++count;
        position += expected.size();
    }
    return count;
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
    rosettelab::curves::EllipseParameters ellipse;
    ellipse.radius_x = 46.0;
    ellipse.radius_y = 46.0;
    auto& layer = document.add_ellipse(ellipse);
    layer.appearance.fill_enabled = true;
    layer.appearance.fill.red = 1.0;
    layer.appearance.fill.green = 0.35;
    layer.appearance.fill.blue = 0.1;
    layer.appearance.fill.alpha = 0.4;
    layer.appearance.stroke.alpha = 0.7;
    layer.appearance.opacity = 0.6;
    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    layer.copies.count = 3;
    layer.copies.arrangement = rosettelab::document::CopyArrangement::Circular;
    layer.copies.circular_radius = 46.0;
    layer.copies.circular_angle_step_degrees = 45.0;

    const auto rgb = rosettelab::pdf::serialize_vector_pdf(document);
    require(rgb.starts_with("%PDF-1.7"), "PDF 1.7 header should be emitted");
    require(contains(rgb, "/Subtype /Form"), "a layer should be a Form XObject");
    require(contains(rgb, "/S /Transparency /I true"), "a layer should be an isolated transparency group");
    require(contains(rgb, "/BM /Multiply"), "native blend mode should be emitted");
    require(count_occurrences(rgb, "/BM /Multiply") == 2,
            "blend mode should apply inside the copy group and to the completed layer");
    require(contains(rgb, "/CA 0.6 /ca 0.6"), "layer opacity should apply to the group");
    require(contains(rgb, "/CA 0.7 /ca 0.4"), "stroke and fill alpha should remain distinct");
    require(contains(rgb, " c\n"), "curve geometry should use cubic Bezier operators");
    require(contains(rgb, "B*\n"), "even-odd fill and stroke should use B*");
    require(!contains(rgb, "/Subtype /Image"), "native export should contain no raster image");
    require(contains(rgb, "xref\n0 "), "a cross-reference table should be emitted");
    require(contains(rgb, "startxref\n"), "the cross-reference offset should be emitted");
    require_valid_xref(rgb);

    rosettelab::document::Document scaled_document;
    rosettelab::curves::EllipseParameters scaled_ellipse;
    scaled_ellipse.radius_x = 10.0;
    scaled_ellipse.radius_y = 20.0;
    auto& scaled_layer = scaled_document.add_ellipse(scaled_ellipse);
    scaled_layer.appearance.stroke_width = 0.6;
    scaled_layer.transform.scale_x = 2.0;
    scaled_layer.transform.scale_y = 3.0;
    scaled_layer.transform.mirror_horizontal = true;
    scaled_layer.copies.count = 2;
    scaled_layer.copies.scale_step = 0.5;
    const auto scaled_pdf = rosettelab::pdf::serialize_vector_pdf(scaled_document);
    require(count_occurrences(scaled_pdf, "0.6 w\n") == 2,
            "each scaled PDF copy should retain the configured 0.6 stroke width");
    require(count_occurrences(scaled_pdf, "1 0 0 1 0 0 cm\n") == 2,
            "layer and copy scales should not affect the PDF graphics-state line width");
    require(contains(scaled_pdf, "-20 0 m\n"),
            "layer scale and horizontal mirror should be baked into PDF path geometry");
    require(contains(scaled_pdf, "-10 0 m\n"),
            "scale per copy and mirror should be baked into each PDF path geometry");
    require(!contains(scaled_pdf, "2 0 0 3 0 0 cm\n"),
            "PDF should not scale strokes through its transformation matrix");

    rosettelab::document::Document imported_document;
    rosettelab::document::ImportedSvgParameters imported;
    imported.geometry.subpath_starts = {0};
    imported.geometry.subpath_closed = {true};
    imported.geometry.segments.push_back({{-10, 0}, {-5, -5}, {5, -5}, {10, 0}});
    static_cast<void>(imported_document.add_imported_svg(imported));
    const auto imported_pdf = rosettelab::pdf::serialize_vector_pdf(imported_document);
    require(contains(imported_pdf, "-10 0 m\n-5 -5 5 -5 10 0 c\nh\n"),
            "PDF should export stored imported SVG geometry as vectors");
    require(!contains(imported_pdf, "/Subtype /Image"),
            "imported SVG geometry must not be rasterized in PDF");

    layer.appearance.cyclic_palette.enabled = true;
    layer.appearance.cyclic_palette.scope = rosettelab::document::PaletteScope::Copies;
    layer.appearance.cyclic_palette.target = rosettelab::document::PaletteTarget::Fill;
    layer.appearance.cyclic_palette.colors = {{1, 0, 0, 1}, {0, 1, 0, 0.8}, {0, 0, 1, 0.6}};
    const auto palette_pdf = rosettelab::pdf::serialize_vector_pdf(document);
    require(contains(palette_pdf, "1 0 0 rg\n"), "first copy should use red fill");
    require(contains(palette_pdf, "0 1 0 rg\n"), "second copy should use green fill");
    require(contains(palette_pdf, "0 0 1 rg\n"), "third copy should use blue fill");
    require(contains(palette_pdf, "/ca 0.8"), "palette color alpha should be retained");

    rosettelab::pdf::ExportOptions options;
    options.color_model = rosettelab::pdf::ColorModel::Cmyk;
    const auto cmyk = rosettelab::pdf::serialize_vector_pdf(document, options);
    require(contains(cmyk, "/CS /DeviceCMYK"), "CMYK page transparency group should be declared");
    require(contains(cmyk, " K\n"), "CMYK stroke operator should be emitted");
    require(contains(cmyk, " k\n"), "CMYK fill operator should be emitted");

    rosettelab::document::Document text_document;
    rosettelab::document::TextParameters text;
    text.text = "A";
    text.vectorize = false;
    text.color = {0.25, 0.5, 0.75, 1.0};
    text.outline.closed = true;
    text.outline.subpath_starts = {0};
    text.outline.segments.push_back({{0, 0}, {2, -8}, {8, -8}, {10, 0}});
    static_cast<void>(text_document.add_text(text));
    const auto outlined_pdf = rosettelab::pdf::serialize_vector_pdf(text_document);
    require(contains(outlined_pdf, "0.25 0.5 0.75 rg\n"),
            "PDF text outlines should retain their fill color");
    require(contains(outlined_pdf, " c\n"), "PDF text should use Bezier outlines even when SVG vectorization is disabled");
    require(!contains(outlined_pdf, "/Subtype /Image"),
            "vectorized PDF text should contain no raster image");

    rosettelab::document::Document invalid_text_document;
    rosettelab::document::TextParameters invalid_text;
    invalid_text.text = "Missing outline";
    static_cast<void>(invalid_text_document.add_text(invalid_text));
    bool rejected_missing_outline = false;
    try {
        static_cast<void>(rosettelab::pdf::serialize_vector_pdf(invalid_text_document));
    } catch (const std::invalid_argument&) {
        rejected_missing_outline = true;
    }
    require(rejected_missing_outline, "non-empty PDF text must never be silently omitted");

    if (argc == 2) {
        std::ofstream output(argv[1], std::ios::binary);
        output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        require(output.good(), "requested PDF fixture should be written");
    }

    return EXIT_SUCCESS;
}
