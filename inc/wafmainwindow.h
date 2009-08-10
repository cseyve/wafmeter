#ifndef WAFMAINWINDOW_H
#define WAFMAINWINDOW_H

#include <QtGui/QMainWindow>
#include "wafmeter.h"

namespace Ui
{
    class WAFMainWindow;
}

class WAFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    WAFMainWindow(QWidget *parent = 0);
    ~WAFMainWindow();

private:
    Ui::WAFMainWindow *ui;

	WAFMeter m_wafMeter;
private slots:
	void on_fileButton_pressed();
};

#endif // WAFMAINWINDOW_H
