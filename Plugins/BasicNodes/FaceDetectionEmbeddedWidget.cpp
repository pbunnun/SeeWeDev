#include "FaceDetectionEmbeddedWidget.hpp"
#include "ui_FaceDetectionEmbeddedWidget.h"
#include <QDebug>

FaceDetectionEmbeddedWidget::FaceDetectionEmbeddedWidget( QWidget *parent )
    : QWidget( parent ),
      ui( new Ui::FaceDetectionEmbeddedWidget )
{
    ui->setupUi( this );
}

FaceDetectionEmbeddedWidget::~FaceDetectionEmbeddedWidget() {
    delete ui;
}

void FaceDetectionEmbeddedWidget::on_mpComboBox_currentIndexChanged( int idx ) {

    qDebug() << "ComboBox : current index is " << idx;
    Q_EMIT button_clicked_signal( 3 );
}

QStringList FaceDetectionEmbeddedWidget::get_combobox_string_list() {
    QStringList string_list;
    for( int count = 0; count < ui->mpComboBox->count(); ++count )
        string_list.append( ui->mpComboBox->itemText( count ) );
    return string_list;
}

void FaceDetectionEmbeddedWidget::set_combobox_value( QString value ) {
    ui->mpComboBox->setCurrentText( value );
}

QString FaceDetectionEmbeddedWidget::get_combobox_text() {
    return ui->mpComboBox->currentText();
}
