#ifndef INJECTOR_H
#define INJECTOR_H

#include <QMainWindow>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Injector; }
QT_END_NAMESPACE

class Injector : public QMainWindow
{
    Q_OBJECT

public:
    Injector(QWidget *parent = nullptr);
    ~Injector();
    HANDLE setupInjector();
    HANDLE runtime;
    int sendPythonPayload(const char* text);

private slots:
    int on_PhaseCheck_triggered();
    int on_InjectCode_clicked();

    void on_Clear_clicked();
    void on_actionTeam_Legend_triggered();

private:
    Ui::Injector *ui;
};

#endif // INJECTOR_H
