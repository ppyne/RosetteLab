#pragma once

#include <QColor>
#include <QPushButton>

namespace rosettelab::ui {

class ColorPreviewButton final : public QPushButton {
public:
    explicit ColorPreviewButton(QWidget* parent = nullptr);

    void set_preview_color(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor color_{Qt::white};
};

} // namespace rosettelab::ui
