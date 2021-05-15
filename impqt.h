#pragma once

#include <QtWidgets>



#include <QtWidgets/QMainWindow>
#include "ui_impqt.h"

class ImpQt : public QMainWindow
{
    Q_OBJECT

public:
    ImpQt(QWidget *parent = Q_NULLPTR);
    
private slots:
    void onStart();

private:
    QString tagAllLines( const QString &input );

private:
    Ui::impqtClass ui;
};
