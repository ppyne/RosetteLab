#include "ui/color_editor_dialog.hpp"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace rosettelab::ui {

class ColorPreviewButton final : public QPushButton {
public:
    explicit ColorPreviewButton(QWidget* parent = nullptr)
        : QPushButton(parent)
    {
    }

    void set_preview_color(const QColor& color)
    {
        color_ = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
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

private:
    QColor color_{Qt::white};
};

namespace {

QString rgba_hex(const QColor& color)
{
    return QStringLiteral("#%1%2%3%4")
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'))
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .toUpper();
}

} // namespace

ColorEditorDialog::ColorEditorDialog(QColor initial_color, QString title, QWidget* parent)
    : QDialog(parent)
    , color_(std::move(initial_color))
{
    setWindowTitle(std::move(title));
    setModal(true);
    setMinimumWidth(360);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    format_ = new QComboBox(this);
    format_->addItem("RGBA", static_cast<int>(Format::Rgba));
    format_->addItem("HSLA", static_cast<int>(Format::Hsla));
    form->addRow("Color model", format_);

    for (std::size_t index = 0; index < channels_.size(); ++index) {
        channel_labels_[index] = new QLabel(this);
        channels_[index] = new QSpinBox(this);
        form->addRow(channel_labels_[index], channels_[index]);
    }
    root->addLayout(form);

    preview_ = new ColorPreviewButton(this);
    preview_->setMinimumHeight(42);
    preview_->setToolTip("Open the graphical color picker");
    root->addWidget(preview_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(format_, &QComboBox::currentIndexChanged, this, [this] {
        configure_fields();
        update_fields_from_color();
    });
    for (auto* channel : channels_) {
        connect(channel, &QSpinBox::valueChanged, this, [this] {
            if (!updating_) {
                update_color_from_fields();
            }
        });
    }
    connect(preview_, &QPushButton::clicked, this, [this] { choose_visually(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    configure_fields();
    update_fields_from_color();
}

void ColorEditorDialog::configure_fields()
{
    const auto model = static_cast<Format>(format_->currentData().toInt());
    const std::array<const char*, 4> rgba_labels{"Red", "Green", "Blue", "Alpha"};
    const std::array<const char*, 4> hsla_labels{"Hue", "Saturation", "Lightness", "Alpha"};
    const auto& labels = model == Format::Rgba ? rgba_labels : hsla_labels;

    updating_ = true;
    for (std::size_t index = 0; index < channels_.size(); ++index) {
        channel_labels_[index]->setText(labels[index]);
        channels_[index]->setSuffix(model == Format::Hsla && index > 0 ? " %" : "");
        channels_[index]->setRange(0, model == Format::Rgba ? 255 : (index == 0 ? 359 : 100));
    }
    updating_ = false;
}

void ColorEditorDialog::update_color_from_fields()
{
    const auto model = static_cast<Format>(format_->currentData().toInt());
    if (model == Format::Rgba) {
        color_.setRgb(
            channels_[0]->value(), channels_[1]->value(),
            channels_[2]->value(), channels_[3]->value());
    } else {
        color_ = QColor::fromHslF(
            static_cast<double>(channels_[0]->value()) / 360.0,
            static_cast<double>(channels_[1]->value()) / 100.0,
            static_cast<double>(channels_[2]->value()) / 100.0,
            static_cast<double>(channels_[3]->value()) / 100.0);
    }
    update_preview();
}

void ColorEditorDialog::update_fields_from_color()
{
    updating_ = true;
    const auto model = static_cast<Format>(format_->currentData().toInt());
    if (model == Format::Rgba) {
        channels_[0]->setValue(color_.red());
        channels_[1]->setValue(color_.green());
        channels_[2]->setValue(color_.blue());
        channels_[3]->setValue(color_.alpha());
    } else {
        channels_[0]->setValue(std::max(0, color_.hslHue()));
        channels_[1]->setValue(static_cast<int>(color_.hslSaturationF() * 100.0));
        channels_[2]->setValue(static_cast<int>(color_.lightnessF() * 100.0));
        channels_[3]->setValue(static_cast<int>(color_.alphaF() * 100.0));
    }
    updating_ = false;
    update_preview();
}

void ColorEditorDialog::update_preview()
{
    preview_->setText(rgba_hex(color_) + "  -  Choose visually...");
    preview_->set_preview_color(color_);
}

void ColorEditorDialog::choose_visually()
{
    const auto selected = QColorDialog::getColor(
        color_, this, "Choose color", QColorDialog::ShowAlphaChannel);
    if (selected.isValid()) {
        color_ = selected;
        update_fields_from_color();
    }
}

} // namespace rosettelab::ui
