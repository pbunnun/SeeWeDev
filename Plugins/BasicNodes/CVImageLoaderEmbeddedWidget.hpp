#ifndef CVIMAGELOADEREMBEDDEDWIDGET_H
#define CVIMAGELOADEREMBEDDEDWIDGET_H
#include "CVDevLibrary.hpp"

#include <QVariant>
#include <QWidget>
#include <QLabel>

namespace Ui {
class CVImageLoaderEmbeddedWidget;
}

class CVImageLoaderEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CVImageLoaderEmbeddedWidget( QWidget *parent = nullptr );
    ~CVImageLoaderEmbeddedWidget();

    void
    set_filename( QString );

    void
    set_active( bool );

    void
    set_flip_pause( bool );

Q_SIGNALS:
    void
    button_clicked_signal( int button );

public Q_SLOTS:

    void
    on_mpForwardButton_clicked();

    void
    on_mpBackwardButton_clicked();

    void
    on_mpPlayPauseButton_clicked();

    void
    on_mpOpenButton_clicked();

    void
    on_mpFilenameButton_clicked();

private:
    Ui::CVImageLoaderEmbeddedWidget *ui;
};

#endif // CVIMAGELOADEREMBEDDEDWIDGET_H
