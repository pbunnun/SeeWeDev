#include "SobelAndScharrEmbeddedWidget.hpp"
#include "ui_SobelAndScharrEmbeddedWidget.h"

SobelAndScharrEmbeddedWidget::SobelAndScharrEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SobelAndScharrEmbeddedWidget)
{
    ui->setupUi(this);
}

SobelAndScharrEmbeddedWidget::~SobelAndScharrEmbeddedWidget()
{
    delete ui;
}

void SobelAndScharrEmbeddedWidget::on_mpCheckBox_stateChanged(int state)
{
    Q_EMIT checkbox_checked_signal( state );
}

void SobelAndScharrEmbeddedWidget::change_enable_checkbox(const bool enable)
{
    ui->mpCheckBox->setEnabled(enable);
}

void SobelAndScharrEmbeddedWidget::change_check_checkbox(const Qt::CheckState state)
{
    ui->mpCheckBox->setCheckState(state);
}

bool SobelAndScharrEmbeddedWidget::checkbox_is_enabled()
{
    return ui->mpCheckBox->isEnabled();
}

bool SobelAndScharrEmbeddedWidget::checkbox_is_checked()
{
    return ui->mpCheckBox->isChecked();
}
