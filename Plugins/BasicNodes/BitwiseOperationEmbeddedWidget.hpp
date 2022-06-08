#ifndef BITWISEOPERATIONEMBEDDEDWIDGET_HPP
#define BITWISEOPERATIONEMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class BitwiseOperationEmbeddedWidget;
}

class BitwiseOperationEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:

    explicit BitwiseOperationEmbeddedWidget(QWidget *parent = nullptr);
    ~BitwiseOperationEmbeddedWidget();

    void set_maskStatus_label(bool active);

private:
    Ui::BitwiseOperationEmbeddedWidget *ui;
};

#endif // BITWISEOPERATIONEMBEDDEDWIDGET_HPP
