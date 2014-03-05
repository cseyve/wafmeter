/***************************************************************************
 *  wafmeter - Woman Acceptance Factor measurement / Qt GUI class
 *
 *  2009-08-10 21:22:13
 *  Copyright  2007  Christophe Seyve
 *  Email cseyve@free.fr
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef WAFMAINWINDOW_H
#define WAFMAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QThread>

#include "wafmeter.h"



namespace Ui
{
class WAFMainWindow;
}

/** @brief Background acquisition and processing thread
  */
class WAFMeterThread : public QThread
{
	Q_OBJECT

public:
	WAFMeterThread(WAFMeter * pWAFmeter);
	~WAFMeterThread();

	/** @brief Set the acquisition source */
	void setCapture(CvCapture * capture);

	/** @brief Return image associated with WAF measure */
	t_waf_info getWAF() { return m_waf; };

	/** @brief Return image associated with WAF measure */
	IplImage * getInputImage() { return m_inputImage; };

	/** @brief Tell the thread to stop */
	void stop();

	/** @brief Return iteration count */
	int getIteration() { return m_iteration; };

	/** @brief Thread loop */
	virtual void run();
private:
	/// Iteration counter
	int m_iteration;

	/// Flag to let the thread loop run
	bool m_run;

	/// Status flag of the running state of thread
	bool m_isRunning;

	WAFMeter * m_pWAFmeter;
	t_waf_info m_waf;

	IplImage * m_inputImage;
	CvCapture * m_capture;
};

/** @brief Main WAFmeter window
  */
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
	WAFMeterThread * m_pWAFMeterThread;
	int m_lastIteration;

	WAFMeter m_wafMeter;
	/** @brief Compute WAF and display result */
	void computeWAF(IplImage *);

	/** @brief Compute WAF and display result */
	void displayWAFMeasure(t_waf_info waf, IplImage * iplImage);

	/** @brief Start background thread for capture source */
	void startBackgroundThread();

	QRect m_grabRect;

	QImage decorImage;
	QImage resultImage;
	t_waf_info m_waf;
private slots:
	void on_fileButton_clicked();
	void on_snapButton_clicked();
	void on_deskButton_clicked();
	void on_camButton_toggled(bool checked);
	void on_movieButton_toggled(bool checked);

	void slot_m_timer_timeout();
	void slot_grabTimer_timeout();

};

#endif // WAFMAINWINDOW_H
