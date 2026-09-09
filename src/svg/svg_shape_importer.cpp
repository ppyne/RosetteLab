#include "svg/svg_shape_importer.hpp"

#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rosettelab::svg {
namespace {

constexpr std::size_t maximum_segments = 100'000;

std::runtime_error error(const QString& message)
{
    return std::runtime_error(message.toStdString());
}

class PathReader {
public:
    explicit PathReader(QString data) : data_(std::move(data)) {}

    core::BezierPath parse()
    {
        QChar command;
        while (skip_separators(), position_ < data_.size()) {
            if (data_[position_].isLetter()) command = data_[position_++];
            else if (command.isNull()) throw error("SVG path data must begin with a command");
            const bool relative = command.isLower();
            const QChar upper = command.toUpper();
            if (upper == 'Z') {
                if (!has_subpath_) throw error("SVG close-path command has no subpath");
                if (new_subpath_pending_) throw error("SVG path closes an empty subpath");
                if (current_ != subpath_start_) append_line(current_, subpath_start_);
                current_ = subpath_start_;
                result_.subpath_closed.back() = true;
                new_subpath_pending_ = true;
                has_cubic_control_ = has_quadratic_control_ = false;
                command = {};
                continue;
            }
            bool first_set = true;
            do {
                if (upper == 'M') {
                    core::Point point = point_pair(relative);
                    if (first_set) {
                        current_ = point;
                        subpath_start_ = point;
                        has_subpath_ = true;
                        new_subpath_pending_ = true;
                        has_cubic_control_ = has_quadratic_control_ = false;
                    } else {
                        append_line(current_, point);
                    }
                } else if (upper == 'L') {
                    ensure_subpath();
                    append_line(current_, point_pair(relative));
                } else if (upper == 'H') {
                    ensure_subpath();
                    double x = number();
                    if (relative) x += current_.x;
                    append_line(current_, {x, current_.y});
                } else if (upper == 'V') {
                    ensure_subpath();
                    double y = number();
                    if (relative) y += current_.y;
                    append_line(current_, {current_.x, y});
                } else if (upper == 'C') {
                    ensure_subpath();
                    const auto c1 = point_pair(relative);
                    const auto c2 = point_pair(relative);
                    const auto end = point_pair(relative);
                    append({current_, c1, c2, end});
                    cubic_control_ = c2;
                    has_cubic_control_ = true;
                    has_quadratic_control_ = false;
                    current_ = end;
                } else if (upper == 'S') {
                    ensure_subpath();
                    const core::Point c1 = has_cubic_control_
                        ? core::Point{2 * current_.x - cubic_control_.x, 2 * current_.y - cubic_control_.y}
                        : current_;
                    const auto c2 = point_pair(relative);
                    const auto end = point_pair(relative);
                    append({current_, c1, c2, end});
                    cubic_control_ = c2;
                    has_cubic_control_ = true;
                    has_quadratic_control_ = false;
                    current_ = end;
                } else if (upper == 'Q' || upper == 'T') {
                    ensure_subpath();
                    core::Point control;
                    if (upper == 'Q') control = point_pair(relative);
                    else control = has_quadratic_control_
                        ? core::Point{2 * current_.x - quadratic_control_.x,
                                      2 * current_.y - quadratic_control_.y}
                        : current_;
                    const auto end = point_pair(relative);
                    append({current_,
                        {current_.x + (control.x - current_.x) * 2.0 / 3.0,
                         current_.y + (control.y - current_.y) * 2.0 / 3.0},
                        {end.x + (control.x - end.x) * 2.0 / 3.0,
                         end.y + (control.y - end.y) * 2.0 / 3.0}, end});
                    quadratic_control_ = control;
                    has_quadratic_control_ = true;
                    has_cubic_control_ = false;
                    current_ = end;
                } else if (upper == 'A') {
                    ensure_subpath();
                    const double rx = std::abs(number());
                    const double ry = std::abs(number());
                    const double rotation = number();
                    const bool large = flag();
                    const bool sweep = flag();
                    const auto end = point_pair(relative);
                    append_arc(rx, ry, rotation, large, sweep, end);
                    has_cubic_control_ = has_quadratic_control_ = false;
                } else {
                    throw error(QStringLiteral("Unsupported SVG path command: %1").arg(command));
                }
                first_set = false;
                skip_separators();
            } while (position_ < data_.size() && !data_[position_].isLetter());
        }
        if (result_.segments.empty()) throw error("SVG path contains no drawable geometry");
        return result_;
    }

private:
    void skip_separators()
    {
        while (position_ < data_.size() &&
               (data_[position_].isSpace() || data_[position_] == ',')) ++position_;
    }

    double number()
    {
        skip_separators();
        const qsizetype start = position_;
        if (position_ < data_.size() && (data_[position_] == '+' || data_[position_] == '-')) ++position_;
        bool digits = false;
        while (position_ < data_.size() && data_[position_].isDigit()) { ++position_; digits = true; }
        if (position_ < data_.size() && data_[position_] == '.') {
            ++position_;
            while (position_ < data_.size() && data_[position_].isDigit()) { ++position_; digits = true; }
        }
        if (!digits) throw error("Invalid number in SVG path");
        if (position_ < data_.size() && (data_[position_] == 'e' || data_[position_] == 'E')) {
            ++position_;
            if (position_ < data_.size() && (data_[position_] == '+' || data_[position_] == '-')) ++position_;
            const qsizetype exponent = position_;
            while (position_ < data_.size() && data_[position_].isDigit()) ++position_;
            if (position_ == exponent) throw error("Invalid exponent in SVG path");
        }
        bool ok = false;
        const double value = data_.mid(start, position_ - start).toDouble(&ok);
        if (!ok || !std::isfinite(value)) throw error("Invalid number in SVG path");
        skip_separators();
        return value;
    }

    bool flag()
    {
        const double value = number();
        if (value != 0.0 && value != 1.0) throw error("SVG arc flags must be zero or one");
        return value == 1.0;
    }

    core::Point point_pair(const bool relative)
    {
        core::Point point{number(), number()};
        if (relative) { point.x += current_.x; point.y += current_.y; }
        return point;
    }

    void ensure_subpath() const
    {
        if (!has_subpath_) throw error("SVG drawing command has no current point");
    }

    void begin_segment()
    {
        if (new_subpath_pending_ || result_.subpath_starts.empty()) {
            result_.subpath_starts.push_back(result_.segments.size());
            result_.subpath_closed.push_back(false);
            new_subpath_pending_ = false;
        }
    }

    void append(const core::CubicBezier& segment)
    {
        if (result_.segments.size() >= maximum_segments) throw error("Imported SVG contains too many segments");
        begin_segment();
        result_.segments.push_back(segment);
    }

    void append_line(const core::Point start, const core::Point end)
    {
        append({start,
            {start.x + (end.x - start.x) / 3.0, start.y + (end.y - start.y) / 3.0},
            {start.x + (end.x - start.x) * 2.0 / 3.0, start.y + (end.y - start.y) * 2.0 / 3.0}, end});
        current_ = end;
        has_cubic_control_ = has_quadratic_control_ = false;
    }

    void append_arc(double rx, double ry, const double rotation_degrees,
                    const bool large, const bool sweep, const core::Point end)
    {
        if (rx == 0.0 || ry == 0.0 || end == current_) { append_line(current_, end); return; }
        const double phi = rotation_degrees * std::numbers::pi / 180.0;
        const double cp = std::cos(phi), sp = std::sin(phi);
        const double dx = (current_.x - end.x) / 2.0;
        const double dy = (current_.y - end.y) / 2.0;
        const double xp = cp * dx + sp * dy;
        const double yp = -sp * dx + cp * dy;
        double radii = xp * xp / (rx * rx) + yp * yp / (ry * ry);
        if (radii > 1.0) { const double factor = std::sqrt(radii); rx *= factor; ry *= factor; }
        const double numerator = std::max(0.0,
            (rx * rx * ry * ry - rx * rx * yp * yp - ry * ry * xp * xp) /
            (rx * rx * yp * yp + ry * ry * xp * xp));
        const double sign = large == sweep ? -1.0 : 1.0;
        const double coef = sign * std::sqrt(numerator);
        const double cxp = coef * rx * yp / ry;
        const double cyp = coef * -ry * xp / rx;
        const double cx = cp * cxp - sp * cyp + (current_.x + end.x) / 2.0;
        const double cy = sp * cxp + cp * cyp + (current_.y + end.y) / 2.0;
        const auto angle = [](const double ux, const double uy, const double vx, const double vy) {
            return std::atan2(ux * vy - uy * vx, ux * vx + uy * vy);
        };
        double start = angle(1, 0, (xp - cxp) / rx, (yp - cyp) / ry);
        double delta = angle((xp - cxp) / rx, (yp - cyp) / ry,
                             (-xp - cxp) / rx, (-yp - cyp) / ry);
        if (!sweep && delta > 0) delta -= 2 * std::numbers::pi;
        if (sweep && delta < 0) delta += 2 * std::numbers::pi;
        const int pieces = std::max(1, static_cast<int>(std::ceil(std::abs(delta) / (std::numbers::pi / 2.0))));
        const double step = delta / pieces;
        const auto map = [&](const double x, const double y) {
            return core::Point{cx + rx * cp * x - ry * sp * y,
                               cy + rx * sp * x + ry * cp * y};
        };
        for (int piece = 0; piece < pieces; ++piece) {
            const double a = start + piece * step;
            const double b = a + step;
            const double k = 4.0 / 3.0 * std::tan(step / 4.0);
            const auto p0 = map(std::cos(a), std::sin(a));
            const auto p3 = piece + 1 == pieces ? end : map(std::cos(b), std::sin(b));
            const auto p1 = map(std::cos(a) - k * std::sin(a), std::sin(a) + k * std::cos(a));
            const auto p2 = map(std::cos(b) + k * std::sin(b), std::sin(b) - k * std::cos(b));
            append({piece == 0 ? current_ : p0, p1, p2, p3});
        }
        current_ = end;
    }

    QString data_;
    qsizetype position_{};
    core::BezierPath result_;
    core::Point current_{};
    core::Point subpath_start_{};
    core::Point cubic_control_{};
    core::Point quadratic_control_{};
    bool has_subpath_{};
    bool has_cubic_control_{};
    bool has_quadratic_control_{};
    bool new_subpath_pending_{};
};

double attribute(const QXmlStreamAttributes& attributes, const char* name, const double fallback = 0.0)
{
    const auto value = attributes.value(QString::fromLatin1(name));
    if (value.isNull()) return fallback;
    bool ok = false;
    const double result = value.toDouble(&ok);
    if (!ok || !std::isfinite(result))
        throw error(QStringLiteral("Invalid SVG %1 attribute").arg(QString::fromLatin1(name)));
    return result;
}

void merge(core::BezierPath& destination, const core::BezierPath& source)
{
    if (source.segments.empty()) return;
    const std::size_t offset = destination.segments.size();
    const auto starts = source.subpath_starts.empty() ? std::vector<std::size_t>{0} : source.subpath_starts;
    for (std::size_t index = 0; index < starts.size(); ++index) {
        destination.subpath_starts.push_back(offset + starts[index]);
        destination.subpath_closed.push_back(core::subpath_is_closed(source, index));
    }
    destination.segments.insert(destination.segments.end(), source.segments.begin(), source.segments.end());
    if (destination.segments.size() > maximum_segments) throw error("Imported SVG contains too many segments");
}

core::BezierPath line_path(const core::Point start, const core::Point end)
{
    core::BezierPath result;
    result.subpath_starts = {0}; result.subpath_closed = {false};
    result.segments.push_back({start,
        {start.x + (end.x - start.x) / 3.0, start.y + (end.y - start.y) / 3.0},
        {start.x + (end.x - start.x) * 2.0 / 3.0, start.y + (end.y - start.y) * 2.0 / 3.0}, end});
    return result;
}

core::BezierPath rectangle_path(const double x, const double y, const double width, const double height)
{
    if (width <= 0.0 || height <= 0.0) return {};
    core::BezierPath result;
    merge(result, line_path({x, y}, {x + width, y}));
    result.segments.push_back(line_path({x + width, y}, {x + width, y + height}).segments.front());
    result.segments.push_back(line_path({x + width, y + height}, {x, y + height}).segments.front());
    result.segments.push_back(line_path({x, y + height}, {x, y}).segments.front());
    result.subpath_closed[0] = true;
    return result;
}

core::BezierPath circle_path(const double cx, const double cy, const double radius)
{
    if (radius <= 0.0) return {};
    constexpr double k = 0.5522847498307936;
    core::BezierPath result;
    result.subpath_starts = {0}; result.subpath_closed = {true};
    result.segments = {
        {{cx + radius, cy}, {cx + radius, cy + k * radius}, {cx + k * radius, cy + radius}, {cx, cy + radius}},
        {{cx, cy + radius}, {cx - k * radius, cy + radius}, {cx - radius, cy + k * radius}, {cx - radius, cy}},
        {{cx - radius, cy}, {cx - radius, cy - k * radius}, {cx - k * radius, cy - radius}, {cx, cy - radius}},
        {{cx, cy - radius}, {cx + k * radius, cy - radius}, {cx + radius, cy - k * radius}, {cx + radius, cy}},
    };
    return result;
}

void normalize(core::BezierPath& path, const double page_width, const double page_height)
{
    double left = std::numeric_limits<double>::infinity(), top = left;
    double right = -left, bottom = -left;
    const auto include = [&](const core::Point point) {
        left = std::min(left, point.x); right = std::max(right, point.x);
        top = std::min(top, point.y); bottom = std::max(bottom, point.y);
    };
    for (const auto& segment : path.segments) {
        include(segment.start); include(segment.control1); include(segment.control2); include(segment.end);
    }
    const double width = right - left, height = bottom - top;
    if (width <= 0.0 && height <= 0.0) throw error("Imported SVG geometry has no extent");
    double factor = 1.0;
    if (width > page_width || height > page_height)
        factor = std::min(width > 0.0 ? page_width * 0.9 / width : std::numeric_limits<double>::infinity(),
                          height > 0.0 ? page_height * 0.9 / height : std::numeric_limits<double>::infinity());
    else if (std::max(width / page_width, height / page_height) < 0.1)
        factor = 0.25 / std::max(width / page_width, height / page_height);
    const double center_x = (left + right) / 2.0, center_y = (top + bottom) / 2.0;
    const auto adjust = [&](core::Point& point) {
        point.x = (point.x - center_x) * factor;
        point.y = (point.y - center_y) * factor;
    };
    for (auto& segment : path.segments) {
        adjust(segment.start); adjust(segment.control1); adjust(segment.control2); adjust(segment.end);
    }
}

} // namespace

core::BezierPath parse_svg_path_data(const QString& data)
{
    return PathReader(data).parse();
}

core::BezierPath import_svg_shapes(
    const QByteArray& data, const double page_width, const double page_height)
{
    if (data.size() > 100 * 1024 * 1024) throw error("SVG exceeds the 100 MB safety limit");
    if (!std::isfinite(page_width) || !std::isfinite(page_height) || page_width <= 0 || page_height <= 0)
        throw error("Document dimensions must be positive");
    const auto lowered = data.toLower();
    if (lowered.contains("<!doctype") || lowered.contains("<!entity"))
        throw error("DTD and entity declarations are not allowed");
    QXmlStreamReader reader(data);
    if (!reader.readNextStartElement() || reader.name() != "svg") throw error("File is not an SVG document");
    core::BezierPath result;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) continue;
        const auto name = reader.name();
        const auto attributes = reader.attributes();
        if (name == "path") {
            const auto d = attributes.value("d");
            if (!d.isEmpty()) merge(result, parse_svg_path_data(d.toString()));
        } else if (name == "line") {
            merge(result, line_path({attribute(attributes, "x1"), attribute(attributes, "y1")},
                                    {attribute(attributes, "x2"), attribute(attributes, "y2")}));
        } else if (name == "circle") {
            merge(result, circle_path(attribute(attributes, "cx"), attribute(attributes, "cy"),
                                      attribute(attributes, "r")));
        } else if (name == "rect") {
            merge(result, rectangle_path(attribute(attributes, "x"), attribute(attributes, "y"),
                attribute(attributes, "width"), attribute(attributes, "height")));
        }
    }
    if (reader.hasError()) throw error(QStringLiteral("Invalid SVG XML: %1").arg(reader.errorString()));
    if (result.segments.empty()) throw error("SVG contains no supported path, line, circle, or rectangle");
    normalize(result, page_width, page_height);
    return result;
}

} // namespace rosettelab::svg
