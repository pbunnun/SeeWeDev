#ifndef SOBELANDSCHARREMBEDDEDWIDGET_HPP
#define SOBELANDSCHARREMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class SobelAndScharrEmbeddedWidget;
}

class SobelAndScharrEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SobelAndScharrEmbeddedWidget(QWidget *parent = nullptr);
    ~SobelAndScharrEmbeddedWidget();

    void change_enable_checkbox(const bool enable);
    void change_check_checkbox(const Qt::CheckState state);
    bool checkbox_is_enabled();
    bool checkbox_is_checked();

Q_SIGNALS:
    void checkbox_checked_signal( int state );

private Q_SLOTS:
    void on_mpCheckBox_stateChanged(int state);

private:
    Ui::SobelAndScharrEmbeddedWidget *ui;
};

#endif // SOBELANDSCHARREMBEDDEDWIDGET_HPP
