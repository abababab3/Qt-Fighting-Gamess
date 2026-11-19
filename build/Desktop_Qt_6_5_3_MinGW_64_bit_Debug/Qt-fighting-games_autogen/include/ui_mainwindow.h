/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QProgressBar *p1Bar;
    QLabel *timerLable;
    QProgressBar *p2Bar;
    QLabel *menuLabel;
    QLabel *koLabel;

    void setupUi(QWidget *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        horizontalLayoutWidget = new QWidget(MainWindow);
        horizontalLayoutWidget->setObjectName("horizontalLayoutWidget");
        horizontalLayoutWidget->setGeometry(QRect(90, 30, 216, 80));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        p1Bar = new QProgressBar(horizontalLayoutWidget);
        p1Bar->setObjectName("p1Bar");
        p1Bar->setValue(24);

        horizontalLayout->addWidget(p1Bar);

        timerLable = new QLabel(horizontalLayoutWidget);
        timerLable->setObjectName("timerLable");

        horizontalLayout->addWidget(timerLable);

        p2Bar = new QProgressBar(horizontalLayoutWidget);
        p2Bar->setObjectName("p2Bar");
        p2Bar->setValue(24);

        horizontalLayout->addWidget(p2Bar);

        menuLabel = new QLabel(MainWindow);
        menuLabel->setObjectName("menuLabel");
        menuLabel->setGeometry(QRect(160, 230, 40, 12));
        koLabel = new QLabel(MainWindow);
        koLabel->setObjectName("koLabel");
        koLabel->setGeometry(QRect(160, 350, 40, 12));

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QWidget *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        timerLable->setText(QCoreApplication::translate("MainWindow", "timelable", nullptr));
        menuLabel->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\346\270\270\346\210\217", nullptr));
        koLabel->setText(QCoreApplication::translate("MainWindow", "KO", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
