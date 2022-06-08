#ifndef ERODEANDDILATEEMBEDDEDWIDGET_HPP
#define ERODEANDDILATEEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class ErodeAndDilateEmbeddedWidget;
}

class ErodeAndDilateEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ErodeAndDilateEmbeddedWidget(QWidget *parent = nullptr);
    ~ErodeAndDilateEmbeddedWidget();
    int getCurrentState() const;
    void setCurrentState(const int state);

Q_SIGNALS:
    void radioButton_clicked_signal();

private Q_SLOTS:

    void on_mpErodeRadioButton_clicked();
    void on_mpDilateRadioButton_clicked();

private:

    Ui::ErodeAndDilateEmbeddedWidget* ui;
    int currentState;

};

#endif // ERODEANDDILATEEMBEDDEDWIDGET_HPP
