#ifndef BLENDIMAGESEMBEDDEDWIDGET_HPP
#define BLENDIMAGESEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class BlendImagesEmbeddedWidget;
}

class BlendImagesEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BlendImagesEmbeddedWidget(QWidget *parent = nullptr);
    ~BlendImagesEmbeddedWidget();

    int getCurrentState() const;
    void setCurrentState(const int state);

Q_SIGNALS:
    void radioButton_clicked_signal();

private Q_SLOTS:

    void on_mpAddRadioButton_clicked();

    void on_mpAddWeightedRadioButton_clicked();

private:
    Ui::BlendImagesEmbeddedWidget *ui;
    int currentState;
};

#endif // BLENDIMAGESEMBEDDEDWIDGET_HPP
