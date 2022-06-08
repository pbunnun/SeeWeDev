#ifndef RGBSETVALUEEMBEDDEDWIDGET_HPP
#define RGBSETVALUEEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class RGBsetValueEmbeddedWidget;
}

class RGBsetValueEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RGBsetValueEmbeddedWidget(QWidget *parent = nullptr);
    ~RGBsetValueEmbeddedWidget();

Q_SIGNALS:
    void
    button_clicked_signal( int button );

private Q_SLOTS:
    void on_mpResetButton_clicked();

private:
    Ui::RGBsetValueEmbeddedWidget *ui;
};

#endif // RGBSETVALUEEMBEDDEDWIDGET_HPP
