#include "NodeDataTimerEmbeddedWidget.hpp"
#include "ui_NodeDataTimerEmbeddedWidget.h"

NodeDataTimerEmbeddedWidget::NodeDataTimerEmbeddedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NodeDataTimerEmbeddedWidget),
    mpQTimer(new QTimer)
{
    ui->setupUi(this);
    ui->mpSecondSpinbox->setMinimum(0);
    ui->mpSecondSpinbox->setValue(0);
    ui->mpMillisecondSpinbox->setMinimum(1);
    ui->mpMillisecondSpinbox->setMaximum(999);
    ui->mpMillisecondSpinbox->setValue(500);
    ui->mpPFComboBox->setCurrentIndex(0);
    ui->mpPFLabel->setText(QString::number(0.500, 'f', 3));
    ui->mpRemainingLabel->setText(QString::number(0));
    ui->mpStopButton->setEnabled(false);

    connect(mpQTimer,&QTimer::timeout,this,&NodeDataTimerEmbeddedWidget::on_timeout);
}

NodeDataTimerEmbeddedWidget::~NodeDataTimerEmbeddedWidget()
{
    delete mpQTimer;
    delete ui;
}

void NodeDataTimerEmbeddedWidget::on_mpSecondSpinbox_valueChanged(int duration)
{
    miPeriod = duration*1000 + ui->mpMillisecondSpinbox->value();
    this->set_pf_labels(ui->mpPFComboBox->currentIndex());
}

void NodeDataTimerEmbeddedWidget::on_mpMillisecondSpinbox_valueChanged(int duration)
{
    miPeriod = ui->mpSecondSpinbox->value()*1000 + duration;
    this->set_pf_labels(ui->mpPFComboBox->currentIndex());
}

void NodeDataTimerEmbeddedWidget::on_mpPFComboBox_currentIndexChanged(int index)
{
    this->set_pf_labels(index);
}

void NodeDataTimerEmbeddedWidget::on_mpStartButton_clicked()
{
    ui->mpStartButton->setEnabled(false);
    ui->mpStopButton->setEnabled(true);
    ui->mpResetButton->setEnabled(false);
    ui->mpSecondSpinbox->setEnabled(false);
    ui->mpMillisecondSpinbox->setEnabled(false);
    mbIsRunning = true;
    this->run();
}

void NodeDataTimerEmbeddedWidget::on_mpStopButton_clicked()
{
    static bool paused = false;
    static int remainingTime;
    static std::chrono::milliseconds remainingTimeAsDuration;
    paused = !paused;

    ui->mpResetButton->setEnabled(paused);
    QString buttonText = paused? "Resume" : "Stop";
    ui->mpStopButton->setText(buttonText);

    if(paused)
    {
        mbIsRunning = false;
        remainingTime = mpQTimer->remainingTime();
        remainingTimeAsDuration = mpQTimer->remainingTimeAsDuration();
        mpQTimer->stop();
        ui->mpRemainingLabel->setText(QString::number(remainingTime));
    }
    else
    {
        mbIsRunning = true;
        mpQTimer->start(remainingTimeAsDuration);
        while(mpQTimer->isActive() && mbIsRunning)
        {
            mpQTimer->singleShot(miInterval,this,&NodeDataTimerEmbeddedWidget::on_singleShot);
            QCoreApplication::processEvents();
        }
        this->run();
    }
}

void NodeDataTimerEmbeddedWidget::on_mpResetButton_clicked()
{
    ui->mpStartButton->setEnabled(true);
    ui->mpSecondSpinbox->setEnabled(true);
    ui->mpMillisecondSpinbox->setEnabled(true);
    QString buttonText("Stop");
    ui->mpStopButton->setText(buttonText);
    ui->mpStopButton->setEnabled(false);
    this->terminate();
}

int NodeDataTimerEmbeddedWidget::get_second_spinbox() const
{
    return ui->mpSecondSpinbox->value();
}

int NodeDataTimerEmbeddedWidget::get_millisecond_spinbox() const
{
    return ui->mpMillisecondSpinbox->value();
}

int NodeDataTimerEmbeddedWidget::get_pf_combobox() const
{
    return ui->mpPFComboBox->currentIndex();
}

bool NodeDataTimerEmbeddedWidget::get_start_button() const
{
    return ui->mpStartButton->isEnabled();
}

bool NodeDataTimerEmbeddedWidget::get_stop_button() const
{
    return ui->mpStopButton->isEnabled();
}

void NodeDataTimerEmbeddedWidget::set_second_spinbox(const int duration)
{
    ui->mpSecondSpinbox->setValue(duration);
}

void NodeDataTimerEmbeddedWidget::set_millisecond_spinbox(const int duration)
{
    ui->mpMillisecondSpinbox->setValue(duration);
}

void NodeDataTimerEmbeddedWidget::set_pf_combobox(const int index)
{
    ui->mpPFComboBox->setCurrentIndex(index);
}

void NodeDataTimerEmbeddedWidget::set_start_button(const bool enable)
{
    ui->mpStartButton->setEnabled(enable);
    mbIsRunning = !enable;
}

void NodeDataTimerEmbeddedWidget::set_stop_button(const bool enable)
{
    ui->mpStopButton->setEnabled(enable);
    mbIsRunning = enable;
}

void NodeDataTimerEmbeddedWidget::set_widget_bundle(const bool enable)
{
    ui->mpSecondSpinbox->setEnabled(enable);
    ui->mpMillisecondSpinbox->setEnabled(enable);
}

void NodeDataTimerEmbeddedWidget::set_pf_labels(const int index)
{
    if(index == 0)
    {
        ui->mpPFLabel->setText(QString::number(miPeriod/1000.0, 'f', 3));
        ui->mpPFUnitLabel->setText("s");
    }
    else if(index == 1)
    {
        ui->mpPFLabel->setText(QString::number(1.0/miPeriod, 'f', 3));
        ui->mpPFUnitLabel->setText("Hz");
    }
}

void NodeDataTimerEmbeddedWidget::set_remaining_label(const float duration)
{
    ui->mpRemainingLabel->setText(QString::number(duration, 'f', 3));
}

void NodeDataTimerEmbeddedWidget::run() const
{
    while(mbIsRunning)
    {
        mpQTimer->start(miPeriod);
        while(mpQTimer->isActive())
        {
            mpQTimer->singleShot(miInterval,this,&NodeDataTimerEmbeddedWidget::on_singleShot);
            QCoreApplication::processEvents();
        }
    }
}

void NodeDataTimerEmbeddedWidget::terminate() const
{
    mpQTimer->stop();
    ui->mpRemainingLabel->setText(QString::number(0));
}

void NodeDataTimerEmbeddedWidget::on_singleShot()
{
    static bool finished = true;
    if(mpQTimer->isActive() && finished)
    {
        finished = false;
        ui->mpRemainingLabel->setText(QString::number(mpQTimer->remainingTime()));
        finished = true;
    }
}

void NodeDataTimerEmbeddedWidget::on_timeout()
{
    Q_EMIT timeout_signal();
}

