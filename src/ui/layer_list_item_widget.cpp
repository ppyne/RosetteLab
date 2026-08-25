#include "ui/layer_list_item_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QToolButton>

#include <utility>

namespace rosettelab::ui {

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

    name_label_ = new QLabel(std::move(name), this);
    name_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout->addWidget(visibility_button_);
    layout->addWidget(lock_button_);
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
    name_label_->setText(name);
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
