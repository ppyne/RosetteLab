#include "ui/color_preview_button.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionButton>

namespace rosettelab::ui {

ColorPreviewButton::ColorPreviewButton(QWidget* parent)
    : QPushButton(parent)
{
}

void ColorPreviewButton::set_preview_color(const QColor& color)
{
    color_ = color;
    update();
}

void ColorPreviewButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QStyleOptionButton option;
    initStyleOption(&option);
    const auto text = option.text;
    option.text.clear();
    style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);

    const QRect contents = style()->subElementRect(
        QStyle::SE_PushButtonContents, &option, this);
    painter.save();
    painter.setClipRect(contents);
    constexpr int square = 8;
    for (int y = contents.top(); y <= contents.bottom(); y += square) {
        for (int x = contents.left(); x <= contents.right(); x += square) {
            const bool grey = ((x - contents.left()) / square +
                               (y - contents.top()) / square) % 2 != 0;
            painter.fillRect(
                QRect(x, y, square, square),
                grey ? QColor(127, 127, 127) : QColor(255, 255, 255));
        }
    }
    painter.fillRect(contents, color_);

    constexpr double checker_average = (255.0 + 127.0) / 2.0;
    const double alpha = color_.alphaF();
    const double red = alpha * color_.redF() * 255.0 + (1.0 - alpha) * checker_average;
    const double green = alpha * color_.greenF() * 255.0 + (1.0 - alpha) * checker_average;
    const double blue = alpha * color_.blueF() * 255.0 + (1.0 - alpha) * checker_average;
    const double luminance = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
    painter.setPen(luminance < 140.0 ? Qt::white : Qt::black);
    painter.drawText(contents, Qt::AlignCenter, text);
    painter.restore();
}

} // namespace rosettelab::ui
