#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "minesweeper_engine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class BoardWidget;
class QComboBox;
class QPushButton;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void startSelection();

    Engine engine_;

    Ui::MainWindow *ui = nullptr;

    BoardWidget *board_ = nullptr;
    QComboBox *difficulty_ = nullptr;
    QPushButton *start_ = nullptr;

    QLabel *timerLabel_ = nullptr;
    QLabel *minesLabel_ = nullptr;
    QTimer *timer_ = nullptr;
};

#endif // MAINWINDOW_H