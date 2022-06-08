#ifndef INFORMATIONDISPLAYEMBEDDEDWIDGET_H
#define INFORMATIONDISPLAYEMBEDDEDWIDGET_H

#include <QWidget>

namespace Ui {
class InformationDisplayEmbeddedWidget;
}

class InformationDisplayEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InformationDisplayEmbeddedWidget( QWidget *parent = nullptr );
    ~InformationDisplayEmbeddedWidget();
    void appendPlainText( QString );

public Q_SLOTS:
    void
    on_mpClearButton_clicked();

    void
    on_mpExportButton_clicked();

private:
    Ui::InformationDisplayEmbeddedWidget *ui;
};

#endif // INFORMATIONDISPLAYEMBEDDEDWIDGET_H
