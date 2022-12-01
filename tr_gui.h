#ifndef TR_GUI_H
#define TR_GUI_H

#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class TR_GUI; }
QT_END_NAMESPACE

class TR_GUI : public QMainWindow
{
    Q_OBJECT

public:
    TR_GUI(QWidget *parent = nullptr);
    ~TR_GUI();

private slots:
    void on_startButton_clicked();

private:
    Ui::TR_GUI *ui;

    QStandardItemModel* hopResultsModel;

    void Initialize();
    void StartTracing();
    void CleanUp();

};

#endif // TR_GUI_H
