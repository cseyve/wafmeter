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
#include <QLabel>
#include <QMutex>
#include <QWaitCondition>
#include <QDir>
#include <QDateTime>

#include "wafmeter.h"

// For gettimeofday
#include <sys/time.h>

namespace Ui
{
class WAFMainWindow;
}


typedef struct {
	QDateTime dateTime;
	t_waf_info waf;
	QImage img;
	QString path;
} WAFRecordItem;

/** @brief Background image recording thread
  */
class RecordingThread : public QThread
{
	Q_OBJECT

public:
	RecordingThread();
	~RecordingThread();

	void askForStop();
	QString appendImage(t_waf_info waf, QImage img);

private:
	/** @brief Thread loop */
	virtual void run();

	bool m_run; ///< run command
	bool m_is_running; ///< running status

	QWaitCondition m_waitCondition;
	QMutex m_waitMutex;

	QMutex m_imageMutex;
	QList<WAFRecordItem> m_saveList;
};


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

	/** @brief Set continuous measure mode */
	void setContinuous(bool continuous)
	{
		m_continuous = continuous;
	}

	/** @brief Return image associated with WAF measure */
	t_waf_info getWAF() { return m_waf; };

	/** @brief Return image associated with WAF measure */
	IplImage * getInputImage() { return m_inputImage; };

	/** @brief Tell the thread to stop */
	void stop();

	/** @brief Get slow update period */
	float getPeriodMS() { return m_period_ms; }

	/** @brief Return iteration count */
	int getIteration() { return m_iteration; };

	/** @brief Thread loop */
	virtual void run();
private:

	/// Slow update period
	float m_period_ms;

	/// Iteration counter
	int m_iteration;

	/// Continuous processing on highgui thread
	bool m_continuous;

	/// Flag to let the thread loop run
	bool m_run;

	/// Status flag of the running state of thread
	bool m_isRunning;

	WAFMeter * m_pWAFmeter;
	t_waf_info m_waf;

	IplImage * m_inputImage;
	CvCapture * m_capture;

};




/** @brief WAFmeter image display
 *
 * This display draws the image, the dial and the needles
  */
class WAFLabel : public QLabel
{
	Q_OBJECT

public:
	WAFLabel(QWidget *parent = 0);

	/** @brief Get the size of object to resize the image with OpenCV,
	 *
	 *  The OpenCV call to cvResize if faster than QImage::scale */
	QRect getRect() { return rect(); }

	/** @brief Paint event= draw the image, the dial background and the needles */
	void paintEvent(QPaintEvent *);

	/** @brief Set the current WAF measure, the image */
	void displayWAFMeasure(t_waf_info waf, IplImage * iplImage);

	/** @brief Capture mouse event to emit signal for processing the WAF */
	void mousePressEvent(QMouseEvent *ev);

	/** @brief Performance measurement: return processing time string
	 *
	 * It returns a description of all the ms spent in the different steps
	 */
	QString getProcStr() { return m_procStr; }


	/** @brief Performance measurement: return processing time string
	 */
	QImage getResultImage() { return m_resultImage; }

private:
	QString m_procStr;		///< processing time string
	QImage m_decorImage;	///< image used for dials => foreground
	QImage m_inputQImage;	///< image used for processing=> background

	/** @brief Result image, eg the image with the original image, the dial and the needles */
	QImage m_resultImage;
	QImage m_destinationImage; ///< image of decor scaled to display size
	t_waf_info m_waf;

signals:
	void signalMousePressEvent(QMouseEvent * ev);
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
	int m_timer_delay_ms;	///< delay before next update

	bool m_pause_display; ///< after a click, lock display until the user tap on screen
	WAFMeterThread * m_pWAFMeterThread;

	int m_lastIteration; ///< index of last iteration displayed

	/** \brief continuous acquisition / display mode. Default = false
	 *
	 *	This mode is ok for PCs because the processing is fast
	 *  but not for Android devices, too slow for Canny
	 */
	bool m_continuous;

	WAFMeter m_wafMeter;
	/** @brief Compute WAF and display result */
	void computeWAF(IplImage *);

	/** @brief Compute WAF and display result */
	void displayWAFMeasure(t_waf_info waf, IplImage * iplImage);

	/** @brief Start background thread for capture source */
	void startBackgroundThread();

	/** @brief Start camera grabbing */
	void startCamera();
	/** @brief Stop camera grabbing */
	void stopCamera();

	QRect m_grabRect;

	QImage decorImage;
	QImage m_resultImage;
	t_waf_info m_waf;

	RecordingThread m_recordingThread;

private slots:
	void on_fileButton_clicked();
	void on_snapButton_clicked();
	void on_deskButton_clicked();
	void on_camButton_toggled(bool checked);
	void on_movieButton_toggled(bool checked);

	void slot_m_timer_timeout();
	void slot_grabTimer_timeout();

	void on_imageLabel_signalMousePressEvent(QMouseEvent * ev);
	void on_continuousButton_toggled(bool checked);
	void on_frontCamButton_toggled(bool checked);
	void on_dirButton_clicked();
	void on_dirButton_toggled(bool checked);
};

/** @brief Get the image directory when the screenshots are saved */
QDir getImageDir();

/** @brief Get the image name */
QString getImagePath(t_waf_info);



#endif // WAFMAINWINDOW_H
