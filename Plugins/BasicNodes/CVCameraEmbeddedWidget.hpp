#ifndef CVCAMERAEMBEDDEDWIDGET_H
#define CVCAMERAEMBEDDEDWIDGET_H
#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

typedef struct CVCameraParameters{
    int miCameraID{0};
    bool mbCameraStatus{false};
} CVCameraParameters;

namespace Ui {
class CVCameraEmbeddedWidget;
}

class CVCameraEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVCameraEmbeddedWidget( QWidget *parent = nullptr );
    ~CVCameraEmbeddedWidget();

    void
    set_params( CVCameraParameters params );

    CVCameraParameters
    get_params() const { return mParams; }

    void
    set_ready_state( bool );

    void
    set_active( bool );

Q_SIGNALS:
    void
    button_clicked_signal( int button );

public Q_SLOTS:
    void
    camera_status_changed( bool );

    void
    on_mpStartButton_clicked();

    void
    on_mpStopButton_clicked();

    void
    on_mpCameraIDComboBox_currentIndexChanged( int );

private:
    CVCameraParameters mParams;
    QLabel * mpTransparentLabel;
    Ui::CVCameraEmbeddedWidget *ui;
};

#endif // CVCAMERAEMBEDDEDWIDGET_H
