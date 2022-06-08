#ifndef DATAGENERATOREMBEDDEDWIDGET_HPP
#define DATAGENERATOREMBEDDEDWIDGET_HPP

#include <QWidget>

namespace Ui {
class DataGeneratorEmbeddedWidget;
}

class DataGeneratorEmbeddedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorEmbeddedWidget(QWidget *parent = nullptr);
    ~DataGeneratorEmbeddedWidget();

    QStringList get_combobox_string_list() const;

    int get_combobox_index() const;

    QString get_text_input() const;

    void set_combobox_index(const int index);

    void set_text_input(const QString& input);

Q_SIGNALS :

    void widget_clicked_signal();

private Q_SLOTS :

    void on_mpComboBox_currentIndexChanged( int );

    void on_mpPlainTextEdit_textChanged();

private:
    Ui::DataGeneratorEmbeddedWidget *ui;

    static const QStringList comboxboxStringList;
};

#endif // DATAGENERATOREMBEDDEDWIDGET_HPP
