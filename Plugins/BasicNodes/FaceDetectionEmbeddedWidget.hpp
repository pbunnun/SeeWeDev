#ifndef FACEDETECTIONEMBEDDEDWIDGET_H
#define FACEDETECTIONEMBEDDEDWIDGET_H

#include <QWidget>

namespace Ui {
    class FaceDetectionEmbeddedWidget;
}

class FaceDetectionEmbeddedWidget : public QWidget {
    Q_OBJECT

    public:
        explicit FaceDetectionEmbeddedWidget( QWidget *parent = nullptr );
        ~FaceDetectionEmbeddedWidget();

        QStringList get_combobox_string_list();

        void set_combobox_value( QString value );

        QString get_combobox_text();

    Q_SIGNALS:
        void button_clicked_signal( int button );

    public Q_SLOTS:
        void on_mpComboBox_currentIndexChanged( int );

    private:
        Ui::FaceDetectionEmbeddedWidget *ui;
};

#endif // FACEDETECTIONEMBEDDEDWIDGET_H
