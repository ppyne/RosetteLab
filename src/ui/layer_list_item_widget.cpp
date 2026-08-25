#include "ui/layer_list_item_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QToolButton>
#include <QTransform>

#include <algorithm>
#include <exception>
#include <utility>

namespace rosettelab::ui {

class ElidedLabel final : public QLabel {
public:
    using QLabel::QLabel;

    void set_full_text(const QString& text)
    {
        full_text_ = text;
        update_tooltip();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(
            contentsRect(),
            alignment() | Qt::TextSingleLine,
            fontMetrics().elidedText(full_text_, Qt::ElideRight, contentsRect().width()));
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QLabel::resizeEvent(event);
        update_tooltip();
    }

private:
    void update_tooltip()
    {
        setToolTip(fontMetrics().horizontalAdvance(full_text_) > contentsRect().width()
            ? full_text_
            : QString{});
    }

    QString full_text_;
};

namespace {

QColor to_qcolor(const document::RgbaColor& color)
{
    return QColor::fromRgbF(color.red, color.green, color.blue, color.alpha);
}

core::BezierPath layer_path(const document::CurveLayer& layer)
{
    if (const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer.parameters)) {
        return curves::generate_polar_rose_bezier(*parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::EllipseParameters>(&layer.parameters)) {
        return curves::generate_ellipse_bezier(*parameters, parameters->bezier_tolerance);
    }
    return {};
}

QPixmap layer_preview(
    const document::CurveLayer& layer,
    const document::RgbaColor& document_background)
{
    constexpr int size = 28;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    const auto background = to_qcolor(document_background);
    if (background.alpha() < 255) {
        constexpr int square = 4;
        for (int y = 0; y < size; y += square) {
            for (int x = 0; x < size; x += square) {
                painter.fillRect(
                    QRect(x, y, square, square),
                    ((x / square + y / square) % 2 != 0)
                        ? QColor(127, 127, 127)
                        : QColor(255, 255, 255));
            }
        }
    }
    painter.fillRect(QRect(0, 0, size, size), background);

    core::BezierPath curve;
    try {
        curve = layer_path(layer);
    } catch (const std::exception&) {
        return pixmap;
    }
    if (curve.segments.empty()) {
        return pixmap;
    }

    QPainterPath path;
    path.moveTo(curve.segments.front().start.x, curve.segments.front().start.y);
    for (const auto& segment : curve.segments) {
        path.cubicTo(
            segment.control1.x, segment.control1.y,
            segment.control2.x, segment.control2.y,
            segment.end.x, segment.end.y);
    }
    if (curve.closed) {
        path.closeSubpath();
    }
    path.setFillRule(layer.appearance.fill_rule == document::FillRule::EvenOdd
        ? Qt::OddEvenFill
        : Qt::WindingFill);

    const auto bounds = path.boundingRect();
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) {
        return pixmap;
    }
    const double scale = 22.0 / std::max(bounds.width(), bounds.height());
    QTransform transform;
    transform.translate(size / 2.0, size / 2.0);
    transform.scale(scale, scale);
    transform.translate(-bounds.center().x(), -bounds.center().y());
    path = transform.map(path);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(std::clamp(layer.appearance.opacity, 0.0, 1.0));
    QPen pen(to_qcolor(layer.appearance.stroke));
    constexpr double minimum_preview_stroke = 0.8;
    constexpr double maximum_preview_stroke = 4.0;
    pen.setWidthF(std::clamp(
        layer.appearance.stroke_width * scale,
        minimum_preview_stroke,
        maximum_preview_stroke));
    painter.setPen(pen);
    painter.setBrush(layer.appearance.fill_enabled
        ? QBrush(to_qcolor(layer.appearance.fill))
        : QBrush(Qt::NoBrush));
    painter.drawPath(path);
    painter.setOpacity(1.0);
    painter.setPen(QColor(0, 0, 0, 80));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, size - 1, size - 1);
    return pixmap;
}

} // namespace

LayerListItemWidget::LayerListItemWidget(
    const document::LayerId id,
    QString name,
    const bool visible,
    const bool locked,
    SelectionCallback selected,
    ToggleCallback visibility_changed,
    ToggleCallback lock_changed,
    QWidget* parent)
    : QWidget(parent)
    , layer_id_(id)
    , visible_(visible)
    , locked_(locked)
    , selected_(std::move(selected))
    , visibility_changed_(std::move(visibility_changed))
    , lock_changed_(std::move(lock_changed))
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 1, 2, 1);
    layout->setSpacing(4);

    visibility_button_ = new QToolButton(this);
    visibility_button_->setAutoRaise(true);
    visibility_button_->setFixedSize(28, 28);
    visibility_button_->setFocusPolicy(Qt::StrongFocus);

    lock_button_ = new QToolButton(this);
    lock_button_->setAutoRaise(true);
    lock_button_->setFixedSize(28, 28);
    lock_button_->setFocusPolicy(Qt::StrongFocus);

    preview_label_ = new QLabel(this);
    preview_label_->setFixedSize(28, 28);
    preview_label_->setAccessibleName("Layer preview");

    name_label_ = new ElidedLabel(this);
    name_label_->set_full_text(std::move(name));
    name_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    name_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout->addWidget(visibility_button_);
    layout->addWidget(lock_button_);
    layout->addWidget(preview_label_);
    layout->addWidget(name_label_, 1);

    connect(visibility_button_, &QToolButton::clicked, this, [this] {
        set_visible_state(!visible_);
        if (visibility_changed_) {
            visibility_changed_(visible_);
        }
    });
    connect(lock_button_, &QToolButton::clicked, this, [this] {
        set_locked_state(!locked_);
        if (lock_changed_) {
            lock_changed_(locked_);
        }
    });

    refresh_visibility_control();
    refresh_lock_control();
}

void LayerListItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (selected_) {
        selected_();
    }
    QWidget::mousePressEvent(event);
}

void LayerListItemWidget::set_name(const QString& name)
{
    name_label_->set_full_text(name);
}

void LayerListItemWidget::set_layer_preview(
    const document::CurveLayer& layer,
    const document::RgbaColor& document_background)
{
    preview_label_->setPixmap(layer_preview(layer, document_background));
}

void LayerListItemWidget::set_visible_state(const bool visible)
{
    visible_ = visible;
    refresh_visibility_control();
}

void LayerListItemWidget::set_locked_state(const bool locked)
{
    locked_ = locked;
    refresh_lock_control();
}

void LayerListItemWidget::refresh_visibility_control()
{
    visibility_button_->setText(visible_ ? QStringLiteral("👁") : QStringLiteral("⊘"));
    visibility_button_->setToolTip(visible_ ? "Hide layer" : "Show layer");
    visibility_button_->setAccessibleName(visible_ ? "Visible layer; click to hide" : "Hidden layer; click to show");
}

void LayerListItemWidget::refresh_lock_control()
{
    lock_button_->setText(locked_ ? QStringLiteral("🔒") : QStringLiteral("🔓"));
    lock_button_->setToolTip(locked_ ? "Unlock layer" : "Lock layer");
    lock_button_->setAccessibleName(locked_ ? "Locked layer; click to unlock" : "Unlocked layer; click to lock");
}

} // namespace rosettelab::ui
