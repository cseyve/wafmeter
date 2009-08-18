#ifndef WAFMAINWINDOW_H
#define WAFMAINWINDOW_H

#include <QtCore/QTimer>
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
	QString m_path;

	QTimer m_timer;
	WAFMeter m_wafMeter;
	/** @brief Compute WAF and display result */
	void computeWAF(IplImage *);

private slots:
	void on_fileButton_pressed();
	void on_camButton_toggled(bool checked);
	void on_m_timer_timeout();
};

#endif // WAFMAINWINDOW_H
