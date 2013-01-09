#pragma once

#include <QMainWindow>

class QTabWidget;

class ExampleWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ExampleWindow(QWidget *parent = 0);
    ~ExampleWindow();
    
private:
    QTabWidget *_tabWidget;
};
