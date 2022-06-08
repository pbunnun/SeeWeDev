#include "ErodeAndDilateEmbeddedWidget.hpp"
#include "ui_ErodeAndDilateEmbeddedWidget.h"

ErodeAndDilateEmbeddedWidget::ErodeAndDilateEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ErodeAndDilateEmbeddedWidget)
{
    ui->setupUi(this);
    ui->mpErodeRadioButton->setChecked(true);
}

ErodeAndDilateEmbeddedWidget::~ErodeAndDilateEmbeddedWidget()
{
    delete ui;
}

void ErodeAndDilateEmbeddedWidget::on_mpErodeRadioButton_clicked()
{
    currentState = 0;
    Q_EMIT radioButton_clicked_signal();
}

void ErodeAndDilateEmbeddedWidget::on_mpDilateRadioButton_clicked()
{
    currentState = 1;
    Q_EMIT radioButton_clicked_signal();
}

int ErodeAndDilateEmbeddedWidget::getCurrentState() const
{
    return currentState;
}

void ErodeAndDilateEmbeddedWidget::setCurrentState(const int state)
{
    currentState = state;
    switch(currentState)
    {
    case 0:
        ui->mpErodeRadioButton->setChecked(true);
        break;

    case 1:
        ui->mpDilateRadioButton->setChecked(true);
        break;
    }
}
