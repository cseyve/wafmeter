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

#include "wafmainwindow.h"
#include "ui_wafmainwindow.h"


#include "imgutils.h"

#include <QFileDialog>

#include <QDateTime>
#include <QDesktopWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QMutex>

#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

#ifdef _QT5
#include <QStandardPaths>
#endif

QImage iplImageToQImage(IplImage * iplImage);

/// Mutex for protecting image processing
QMutex g_processing_mutex;

u8 mode_file = 0;

/// @brief Debug realtime handling
bool g_debug_realtime = false;

/** @brief Capture object for OpenCV/highgui camera */
CvCapture * g_capture = 0;

/** @brief Index of capture device in OpenCV/highgui cameras list */
int g_index_capture = 0;

/** @brief rotation for display.
 *
 *  Used on Android for switching between the front (-90°) and back camera (+90°)
 */
int g_rotate = 0;

/** @brief Time measurement between 2 timestamps in milliseconds */
int delta_ms(struct timeval tv1, struct timeval tv2)
{
	int dt = (tv2.tv_sec - tv1.tv_sec) * 1000
			+ (tv2.tv_usec - tv1.tv_usec) / 1000 ;

	return dt;
}

#ifndef PI
#define PI 3.1415927
#endif

WAFLabel::WAFLabel(QWidget *parent)
	: QLabel(parent)
{
	// load display image
	m_decorImage.load(":icons/Interface-montage.png");
	m_decorImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

	//

}

void WAFLabel::displayWAFMeasure(t_waf_info waf, IplImage * iplImage)
{
	m_waf = waf;
	struct timeval tv1;
	gettimeofday(&tv1, NULL);

	if(iplImage->nChannels == 3)
	{
		static IplImage * rgbImage = NULL;
		if(rgbImage &&
				(rgbImage->width != iplImage->width
				 || rgbImage->height != iplImage->height))
		{
			cvReleaseImage(&rgbImage);
			rgbImage = NULL;
		}
		if(!rgbImage)
		{
			rgbImage = cvCloneImage(iplImage);
		}
		cvCvtColor(iplImage, rgbImage, CV_RGB2BGR);
		iplImage = rgbImage;
	}
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	int dtscale=delta_ms(tv1, tv2);
	m_procStr = tr(" CvtCol=") + QString::number(dtscale);

	// Then scale
	QRect cr = rect();
	static IplImage * scaledImage = NULL;
	int expected_width = cr.width();
	int expected_height = cr.height();
	// If there is a rotation, switch width/height
	if(abs(g_rotate) == 90) // for Android
	{
		expected_height = cr.width();
		expected_width = cr.height();
	}

	// Resize the right factor to avoid deformation of image (stretching on one axis)
	if(expected_height/iplImage->height > expected_width/iplImage->width)
	{
		// The ratio is higher on height, so limit height
		expected_height = expected_width * iplImage->height / iplImage->width;
	}
	else
	{
		expected_width = expected_height * iplImage->width / iplImage->height;
	}


	if(scaledImage &&
			(scaledImage->width != expected_width
			 || scaledImage->height != expected_height
			 || scaledImage->nChannels != iplImage->nChannels
			 ))
	{
		tmReleaseImage(&scaledImage);
	}

	if(!scaledImage)
	{
		scaledImage = tmCreateImage(cvSize(expected_width, expected_height),
									IPL_DEPTH_8U, iplImage->nChannels);
	}
//	fprintf(stderr, "WAFLabel::%s:%d : resize(%dx%dx%d => %dx%dx%d)\n",
//			__func__, __LINE__,
//			iplImage->width, iplImage->height, iplImage->nChannels,
//			scaledImage->width, scaledImage->height, scaledImage->nChannels
//			);
	cvResize(iplImage, scaledImage);
	m_inputQImage = iplImageToQImage(scaledImage);

	struct timeval tv3;
	gettimeofday(&tv3, NULL);
	dtscale=delta_ms(tv2, tv3);
	m_procStr += tr(" Resize=") + QString::number(dtscale);

	if(g_rotate != 0)
	{
		QImage ImgRot = m_inputQImage;
		QTransform rot;
		m_inputQImage = ImgRot.transformed( rot.rotate( g_rotate ));
	}

	struct timeval tv4;
	gettimeofday(&tv4, NULL);
	dtscale=delta_ms(tv3, tv4);
	m_procStr += tr(" Rotate=") + QString::number(dtscale);

	update();
}



void WAFLabel::mousePressEvent(QMouseEvent *ev)
{
	fprintf(stderr, "WAFLabel::%s:%d : ev=%p\n",
			__func__, __LINE__, ev);
	emit signalMousePressEvent(ev);
}



void WAFLabel::paintEvent(QPaintEvent * )
{
	QRect cr = rect();

	struct timeval tv1;
	gettimeofday(&tv1, NULL);

	// load image to display
	QImage scaledImg;
	if(m_inputQImage.width() != cr.width()
			|| m_inputQImage.height() != cr.height())
	{
		scaledImg = m_inputQImage.scaled(
				cr.width(),
				cr.height(),
				Qt::KeepAspectRatio
				);
	}
	else
	{
		scaledImg = m_inputQImage;
	}
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	int dtscale=delta_ms(tv1, tv2);
	m_procStr += tr(" Scale=") + QString::number(dtscale);

	/*
	QPainter::CompositionMode mode = currentMode();

	QPainter painter(&resultImage);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(resultImage.rect(), Qt::transparent);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.drawImage(0, 0, destinationImage);
	painter.setCompositionMode(mode);
	painter.drawImage(0, 0, sourceImage);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
	painter.fillRect(resultImage.rect(), Qt::white);
	painter.end();

	resultLabel->setPixmap(QPixmap::fromImage(resultImage));
	*/

	if(m_inputQImage.isNull())
	{
		m_inputQImage = QImage( cr.size(), QImage::Format_ARGB32 );
		m_inputQImage.fill(255);
	}

	QImage resultImage(cr.width(),
					   cr.height(),
					   QImage::Format_ARGB32_Premultiplied);
//	resultImage = m_inputImage.scaled(
//				cr.width(),
//				cr.height(),
//				Qt::IgnoreAspectRatio );
	struct timeval tv3;
	gettimeofday(&tv3, NULL);

	dtscale=delta_ms(tv2, tv3);
	m_procStr += tr(" resultScale=") + QString::number(dtscale);

	QPainter painter(&resultImage);
	painter.fillRect(resultImage.rect(), Qt::white);

	if(m_destinationImage.width() != cr.width()) {
		m_destinationImage = m_decorImage.scaled(cr.width(),
												cr.width(), // twice width to force to be dow
												Qt::KeepAspectRatio);
	}
	QImage sourceImage = scaledImg;
	sourceImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

	struct timeval tv4;
	gettimeofday(&tv4, NULL);
	dtscale=delta_ms(tv3, tv4);
	m_procStr += tr("sourceImage=") + QString::number(dtscale);

	//QImage alphaMask = decorImage.createMaskFromColor(qRgb(0,0,0));
	//decorImage.setAlphaChannel(alphaMask);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(resultImage.rect(), Qt::transparent);

	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(resultImage.rect(), Qt::white);
	// Draw input image on top, centered
	painter.drawImage((resultImage.width() - sourceImage.width())/2,
					  0, sourceImage);
	struct timeval tv5;
	gettimeofday(&tv5, NULL);
	dtscale=delta_ms(tv4, tv5);
	m_procStr += tr(" draw sourceImage=") + QString::number(dtscale);

	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	// Draw dials on bottom
	painter.drawImage(0, cr.height()-m_destinationImage.height(),
					  m_destinationImage);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
	//        painter.fillRect(resultImage.rect(), Qt::white);

	painter.setCompositionMode(QPainter::CompositionMode_Source);


	// Display waf with dial ----------------------------------------------
	painter.setRenderHint(QPainter::Antialiasing, true);
	// Margin on both sides of of the dial
	float theta_min = 0.05;
	float theta = (PI - 2.f* theta_min) * (1. - m_waf.waf_factor) + theta_min;

	int radius_in = cr.width()/2 - 10 /* thickness of dial border */;


	QPoint dial_center(cr.width()/2,
					   cr.height());

//	QPoint dial_center(160, 416);

	// Center of background image : 160, 416
	QPen pen(qRgb(255,0,0));
	pen.setWidth(2);
	pen.setCapStyle(Qt::RoundCap);

	// DRAW MAIN RESULT AS THICK BLACK NEEDLE
	// Draw main result first, to draw smaller dial over it
	pen.setColor(qRgb(0,0,0));
	pen.setColor(qRgba(10,10,10, 32));
	pen.setWidth(5);
	painter.setPen(pen);

	float r = radius_in;
	theta = (PI - 2.f*theta_min) * (1.f - m_waf.waf_factor) + theta_min;
	painter.drawLine(QPoint(dial_center.x(), dial_center.y()),
					 QPoint(dial_center.x()+r * cos(theta), dial_center.y()-r*sin(theta)));
	r = radius_in-15;

	// draw shape with gray
	pen.setColor(qRgb(214,214,214));
	pen.setWidth(3);
	painter.setPen(pen);
	r = radius_in-20;
	theta = (PI - 2.f*theta_min) * (1.f - m_waf.contour_factor) + theta_min;
	painter.drawLine(QPoint(dial_center.x(),
							dial_center.y()),
					 QPoint(dial_center.x() +r * cos(theta),
							dial_center.y()-r*sin(theta)));

	// draw color with green
	pen.setColor(qRgb(127,255,0));
	r = radius_in-20;
	theta = (PI - 2.f*theta_min) * (1.f - m_waf.color_factor) + theta_min;
	pen.setWidth(3);
	painter.setPen(pen);
	painter.drawLine(QPoint(dial_center.x(),
							dial_center.y()),
					 QPoint(dial_center.x() + r * cos(theta),
							dial_center.y() - r*sin(theta)));
	r = radius_in-40;
	pen.setWidth(6);
	painter.setPen(pen);
	painter.drawEllipse(QPoint(dial_center.x() + r * cos(theta),
							   dial_center.y() - r * sin(theta)),
						3, 3);



	pen.setWidth(16);
	pen.setColor(qRgb(127,127,127));
	painter.setPen(pen);
	painter.drawEllipse(QPoint(dial_center.x()+1, dial_center.y()-1), 8,8);
	pen.setColor(qRgba(10,10,10, 128));
	painter.setPen(pen);
	painter.drawEllipse(dial_center, 8, 8);
	painter.end();


	//	QPixmap localPixmap = QPixmap::fromImage(resultImage);
	//	ui->imageLabel->setPixmap(localPixmap);

	/*
	 QString dialname=":/icons/Dialer.png";
	 QImage pixmap(dialname);
	 pixmap = pixmap.convertToFormat(QImage::Format_Indexed8);
	 pixmap.setNumColors(256);
	 for(int col = 0; col < 256; col++)
	  pixmap.setColor(col, qRgb(col, col, col));
	 int arrow = (int)(255.f * waf.waf_factor);
	 pixmap.setColor(arrow, qRgb(255, 0, 0));
	 ui->dialLabel->setPixmap(QPixmap(pixmap));
	*/

	// Copy for snap
	m_resultImage = resultImage.copy();

	QPainter p( this );
	p.drawImage(0,0, resultImage);
	p.end();

}

WAFMainWindow::WAFMainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::WAFMainWindow)
{
	char home[512]=".";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	m_pause_display = false;

	m_path = QString(home);
	ui->setupUi(this);
#ifdef HAS_DEBUGLABEL
	ui->debugLabel->hide();
#endif
	m_continuous = false;
	m_lastIteration = -1;
	m_timer_delay_ms = 40;

#if !defined(_ANDROID) && defined(_QT5)
	//setOrientation(Qt::Vertical);
#endif

#if defined(_ANDROID)
	g_rotate = 90; // Camera is rotated with OpenCV when the smartphone is in portrait

	ui->widget_grid_bottom->setMinimumHeight(70);
	ui->movieButton->hide();
	ui->deskButton->hide();
	ui->fileButton->hide();


	ui->stackedWidget->setCurrentIndex(0);

	ui->dirButton->hide();

	QSize iconSize = QSize(64,64);
	ui->camButton->setIconSize(iconSize);
	ui->continuousButton->setIconSize(iconSize);
	ui->frontCamButton->setIconSize(iconSize);
	ui->snapButton->setIconSize(iconSize);
	ui->dirButton->setIconSize(iconSize);
#endif

	m_pWAFMeterThread = NULL;

	QString filename=":/qss/WAFMeter.qss";
	QFile file(filename);
	file.open(QFile::ReadOnly);

	QString styleSheet = QLatin1String(file.readAll());
	//setStyleSheet(styleSheet);

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(slot_m_timer_timeout()));

	m_timer_delay_ms = 40; // 25 fps

	// Load hue scale from ressource file (easier to deploy application)
	QImage huescale(":huescale.png");
	if(!huescale.isNull())
	{
		IplImage * header = cvCreateImageHeader(cvSize(huescale.width(), huescale.height()),
												IPL_DEPTH_8U, huescale.depth()/8);
		header->imageData = (char *)huescale.bits();
		setWafHueScale(header);
		cvReleaseImageHeader(&header);
	}

	// start recording thread
	m_recordingThread.start();

}

WAFMainWindow::~WAFMainWindow()
{
	delete ui;
}



void WAFMainWindow::on_fileButton_clicked() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
													m_path,
													tr("Images (*.png *.xpm *.jpg)"));
	if(fileName.isEmpty()) return;

	QFileInfo fi(fileName);
	if(!fi.exists()) return;

	m_path = fi.absolutePath();

	// load file
	IplImage * iplImage = cvLoadImage(fileName.toUtf8().data());
	computeWAF(iplImage);

	tmReleaseImage(&iplImage);
}




QImage iplImageToQImage(IplImage * iplImage) {
	if(!iplImage) {
		return QImage();
	}

	QImage outImage( (uchar*)iplImage->imageData,
				   iplImage->width, iplImage->height,
				   iplImage->widthStep,
				   ( iplImage->nChannels == 4 ? QImage::Format_ARGB32 :
												(iplImage->nChannels == 3 ?
													 QImage::Format_RGB888 //Set to RGB888 instead of ARGB32 for ignore Alpha Chan
												   : QImage::Format_Indexed8)
												)
				   );
	return outImage;
}


void WAFMainWindow::computeWAF(IplImage * iplImage) {
	if(!iplImage) return;

	g_processing_mutex.lock();
	// compute waf
	if(mode_file) {
		m_wafMeter.setUnscaledImage(iplImage);
	} else {
		m_wafMeter.setScaledImage(iplImage);
	}
	g_processing_mutex.unlock();
	t_waf_info waf = m_wafMeter.getWAF();

	// Display images
	displayWAFMeasure(waf, iplImage);

#ifdef HAS_DEBUGLABEL
	if(g_debug_realtime)
	{
		QString debugStr;
		debugStr.sprintf("%dx%dx%d",
						 iplImage->width,
						 iplImage->height,
						 iplImage->nChannels);

		debugStr += ui->imageLabel->getProcStr();
		ui->debugLabel->setText(debugStr);
	}
#endif

	// Make a copy
	m_resultImage = ui->imageLabel->getResultImage().copy();
	fprintf(stderr, "WAFMainWindow::%s:%d : result=%dx%d\n",
			__func__, __LINE__, m_resultImage.width(), m_resultImage.height());
}

void WAFMainWindow::on_deskButton_clicked()
{
	QPoint imgPos = QApplication::activeWindow()->geometry().topLeft();
	QRect imgRect = ui->imageLabel->rect();

	/*
	QPoint curpos = QApplication::activeWindow()->pos();
	QRect currect = QApplication::activeWindow()->rect();
	fprintf(stderr, "%s:%d: curpos=%dx%d +%dx%d imageLabel=%d,%d rect=%d,%d+%dx%d\n",
			__func__, __LINE__,
			curpos.x(), curpos.y(),
			currect.width(), currect.height(),
			imgPos.x(), imgPos.y(),
			imgRect.x(), imgRect.y(),
			imgRect.width(), imgRect.height()
			);
*/
	m_grabRect = QRect(imgPos.x()+1, imgPos.y()+1,
					   imgRect.width(), imgRect.height());
	hide();
	QTimer::singleShot(500, this, SLOT(slot_grabTimer_timeout()));
}

void WAFMainWindow::on_imageLabel_signalMousePressEvent(QMouseEvent * ev)
{
	if(!g_capture) // no camera, do nothing
	{
		return;
	}

	if(m_continuous)
	{ // return without processing to avoid Data race
		m_pause_display = false;
		return;
	}

	if(!m_pause_display)
	{
		fprintf(stderr, "WAFMainWindow::%s:%d : ev=%p\n",
				__func__, __LINE__, ev);
		IplImage * input = m_pWAFMeterThread->copyInputImage();
		computeWAF(input);
		m_pause_display = true;
		ui->tapLabel->setText(tr("Tap to go back to live"));
	}
	else // unlock the display
	{
		m_pause_display = false;
		ui->tapLabel->setText(tr("Tap to get a new WAF"));
		// Schedule refresh right now
		QTimer::singleShot(10, this, SLOT(slot_m_timer_timeout()));
	}
}

void WAFMainWindow::slot_grabTimer_timeout() {

	QPixmap desktopPixmap = QPixmap::grabWindow(
				//QApplication::activeWindow()->winId());
				QApplication::desktop()->winId());
	/*  fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
			__func__, __LINE__,
			desktopPixmap.width(), desktopPixmap.height(), desktopPixmap.depth());
*/
	//QApplication::activeWindow()->show();

	// Crop where the window is

	fprintf(stderr, "%s:%d: curpos=%dx%d+%dx%d\n",
			__func__, __LINE__,
			m_grabRect.x(), m_grabRect.y(),
			m_grabRect.width(), m_grabRect.height());

	QImage desktopImage;
	desktopImage = ( desktopPixmap.copy(m_grabRect).toImage() );

	if(desktopImage.isNull()) return;
	/* fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
			__func__, __LINE__,
			desktopImage.width(), desktopImage.height(), desktopImage.depth());
*/

	IplImage * header = cvCreateImageHeader(cvSize(desktopImage.width(), desktopImage.height()),
											IPL_DEPTH_8U, desktopImage.depth()/8);
	header->imageData = (char *)desktopImage.bits();

	// Compute and display waf
	computeWAF(header);

	show();

	cvReleaseImageHeader(&header);
}

void WAFMainWindow::on_continuousButton_toggled(bool checked)
{
	m_continuous = checked;
	if(checked) // on the first time the user go into continuous mode,
	{			// we consider he understood, so we hide the label definitively
		ui->tapLabel->setVisible(false);
	}

	// unlock display in any cases
	m_pause_display = false;

	if(m_pWAFMeterThread) {
		m_pWAFMeterThread->setContinuous(m_continuous);
	}

	// Grab in 10ms, so pretty soon
	QTimer::singleShot(10, this, SLOT(slot_m_timer_timeout()));
}


void WAFMainWindow::startCamera()
{
	if(g_capture) {
		// Already running, so stop first
		stopCamera();
	}
	bool checked = ui->frontCamButton->isChecked();
	fprintf(stderr, "WAFMainWindow::%s:%d : starting camera... (front cam=%c)", __func__, __LINE__,
			(checked ? 'T':'F'));

	// on android cameras, back camera is 0 and front is 1
	g_index_capture = (ui->frontCamButton->isChecked() ? 1 : 0);
#if defined(_ANDROID)
	// On Google/Samsung Nexus S, we have to rotate +90 deg for back camera
	// and -90 for front
	g_rotate = (ui->frontCamButton->isChecked() ? -90 : 90);
#endif
	g_capture = cvCreateCameraCapture( g_index_capture );

	startBackgroundThread();

	if(g_capture && m_pWAFMeterThread)
	{
		m_pWAFMeterThread->setCapture( g_capture );
	}

	QTimer::singleShot(m_timer_delay_ms, this, SLOT(slot_m_timer_timeout()));
}

void WAFMainWindow::stopCamera()
{
	if( g_capture )
	{
		// Stop thread
		if(m_pWAFMeterThread) {
			m_pWAFMeterThread->stop();
			m_pWAFMeterThread->setCapture(NULL);
		}

		cvReleaseCapture(&g_capture);
		g_capture = NULL;
	}
	if(m_timer.isActive())
	{
		m_timer.stop();
	}
}

void WAFMainWindow::on_frontCamButton_toggled(bool )
{
	if(g_capture)
	{
		stopCamera();

		// start camera
		startCamera();
	}
}

QDir getImageDir()
{
	QDir snapDir(QDir::homePath());

#ifndef _QT5
	QString writePath = QObject::tr("Desktop");
	// Before Qt5, no standard path have been described
	snapDir.cd(writePath);
#else

#ifdef _ANDROID
	QString writePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	if(snapDir.cd("/sdcard/WAFmeter"))
	{
		writePath = snapDir.currentPath();
	}
#else
	// Ref: http://qt-project.org/doc/qt-5/qstandardpaths.html
	QStringList stdLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
	QString msg = "pictures={";
	for(int idx=0; idx<stdLocations.count(); ++idx)
	{
		msg += QString::number(idx) + "=" + stdLocations.at(idx) + ";";
	}
	msg += "}\n";

	QString readPath = QStandardPaths::displayName(QStandardPaths::PicturesLocation);
	msg += QObject::tr(" displayName(picturesLocation)=")+readPath;
	QString writePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	if(writePath.isEmpty())
	{
		msg += QObject::tr(" Could not read writable path for Pictures");
		writePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
		qDebug() <<  QObject::tr("No writable locations for saving images, save them in app directory ")
							  << writePath;
		msg += QObject::tr("=> use app path ") + writePath;
	}
	snapDir.cd(writePath);
#endif // ANDROID
#endif // QT5

	qDebug() << "Write path: " << writePath;

	return snapDir;
}

void WAFMainWindow::on_dirButton_toggled(bool checked)
{
	// on desktop, open the directory fil file browser
	// on android, open a gallery
#if defined(_ANDROID)
	if(checked)
	{ // update images for gallery

	}
	ui->stackedWidget->setCurrentIndex( (checked ? 1 : 0) );
#endif
}

void WAFMainWindow::on_dirButton_clicked()
{
	// on desktop, open the directory fil file browser
	// on android, open a gallery
#ifndef _ANDROID
	QString writePath = getImageDir().absolutePath();

	qDebug() << "Open directory:" << writePath;
	QDesktopServices::openUrl(QUrl::fromLocalFile(writePath));
#endif
}

/** @brief Get the image name */
QString getImagePath(QDateTime dateTime, t_waf_info waf)
{
	// Assemblate file name
	QString msg;

	QDir snapDir = getImageDir();

	QString wafStr;
	wafStr.sprintf("%02d", (int)roundf(waf.waf_factor*100.f));
	QString dateStr = dateTime.toString(QObject::tr("yyyy-MM-dd_hhmmss"));

	QString filepath = snapDir.absoluteFilePath("WAF_" + dateStr + "waf=" + wafStr + "perc.png");

	return filepath;
}

void WAFMainWindow::on_snapButton_clicked() {

	if(m_resultImage.isNull()) {
		fprintf(stderr, "WAFMainWindow::%s:%d : ERROR: result image is null\n",
				__func__, __LINE__);
		return;
	}
	QString path = m_recordingThread.appendImage(m_waf, m_resultImage);
	ui->debugLabel->setText(tr("Saved as ") + path);
	ui->debugLabel->show();

	// Hide in 1 sec
	QTimer::singleShot(1000, ui->debugLabel, SLOT(hide()));
}

void WAFMainWindow::displayWAFMeasure(t_waf_info waf, IplImage * iplImage)
{

	// Display
	if(!iplImage) return;
	m_waf = waf;

	ui->imageLabel->displayWAFMeasure(waf, iplImage);
}


void WAFMainWindow::on_camButton_toggled(bool checked)
{
	fprintf(stderr, "WAFMainWindow::%s:%d : checked='%c'", __func__, __LINE__,
			(checked ? 'T':'F'));
	if(checked)
	{
		startCamera();
	}
	else
	{
		stopCamera();
	}

}

void WAFMainWindow::on_movieButton_toggled(bool checked)
{
	if(!checked)
	{
		if(m_timer.isActive())
		{
			m_timer.stop();
		}

		// Stop thread
		if(m_pWAFMeterThread) {
			m_pWAFMeterThread->stop();
			m_pWAFMeterThread->setCapture(NULL);
		}
		cvReleaseCapture(&g_capture);
		g_capture = NULL;

		return;
	}

	if(!g_capture) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
														m_path,
														tr("Movies (*.avi *.mp* *.mov)"));
		if(fileName.isEmpty()) return;

		QFileInfo fi(fileName);
		if(!fi.exists()) return;

		m_path = fi.absolutePath();

		g_capture = cvCaptureFromFile( fi.absoluteFilePath().toUtf8().data() );

		fprintf(stderr, "%s:%d : capture=%p\n", __func__, __LINE__, g_capture);
		startBackgroundThread();
	}
}

void WAFMainWindow::startBackgroundThread() {

	if(!g_capture) {
		fprintf(stderr, "%s:%d : CANNOT start: capture=%p\n", __func__, __LINE__, g_capture);

		return ; }

	if(!m_pWAFMeterThread) {
		fprintf(stderr, "%s:%d : starting new thread...\n", __func__, __LINE__);
		m_pWAFMeterThread = new WAFMeterThread(&m_wafMeter);
	}

	m_pWAFMeterThread->setCapture(g_capture);

	m_pWAFMeterThread->start();

//	if(!m_timer.isActive()) {
//#if !defined(_ANDROID) && !defined(_IOS)
//		m_timer.start(40); // 25 fps
//#else
//		m_timer.start(250); // 4 fps
//#endif
//	}

}

void WAFMainWindow::slot_m_timer_timeout()
{
	if(!g_capture)
	{
		m_timer.stop();
		return;
	}

	if(!m_pWAFMeterThread) {
		m_timer.stop();
		return;
	}
	if(m_pause_display)
	{
		fprintf(stderr, "WAFMainWindow::%s:%d: display is locked\n",
				__func__, __LINE__);
		return;
	}
	int next_time = m_timer_delay_ms;

	// Grab image
	int cur_iteration = m_pWAFMeterThread->getIteration();
	if(m_lastIteration != cur_iteration)
	{
		int d_iter = cur_iteration - m_lastIteration;
		float period_ms = m_pWAFMeterThread->getPeriodMS();

		m_lastIteration = cur_iteration;

		if(m_continuous)
		{
			m_waf = m_pWAFMeterThread->getWAF();

			// the processing is done in background thread, so next time is the period time
			next_time = period_ms;
			if(next_time < 40 || next_time > 500)
			{
				next_time = 40;
			}
		} else { // just update display period
			if(period_ms > 0)
			{
				// At perfect speed, we should have 1 iteration
				// but since the CPU may be too slow we measure the delay we should have to
				// update at nice framerate, so next time in difference of frames x next period
				next_time = (int)(d_iter * period_ms);
				if(g_debug_realtime) {
					fprintf(stderr, "WAFMainWindow::%s:%d: dI=%d x period=%g ms = next in %d ms\n",
						__func__, __LINE__,
						d_iter, period_ms,
						next_time);
				}
				if(next_time < 40 || next_time > 500)
				{
					next_time = 40;
				}
				else {
					m_timer_delay_ms = next_time;
				}
			}
		}

		if(g_debug_realtime) {
			fprintf(stderr, "%s:%d: iter %4d\n", __func__, __LINE__, cur_iteration);
		}

		// Display image
		IplImage * processImage = m_pWAFMeterThread->copyInputImage();
		displayWAFMeasure(m_waf, m_pWAFMeterThread->copyInputImage());

		// if continuous, make a copy now
		if(m_continuous) {
			// Make a copy
			m_resultImage = ui->imageLabel->getResultImage().copy();
		}
		if(g_debug_realtime) {
			QString debugStr;
			if(processImage)
			{
				debugStr.sprintf("%dx%dx%d # %4d",
								 processImage->width,
								 processImage->height,
								 processImage->nChannels,
								 cur_iteration);
			}
			debugStr += ui->imageLabel->getProcStr();
			ui->debugLabel->setText(debugStr);
		}

		// just now because the background thread is running and we will already have to wait for img proc in main thread
		next_time = 10; // 1/100s
	} else {
		next_time = 20; // 1/50s from now is enough, because it will be available quickly
		if(g_debug_realtime)
		{
			fprintf(stderr, "%s:%d Refresh too fast fast / thread\n", __func__, __LINE__);
		}
	}

	QTimer::singleShot(next_time, this, SLOT(slot_m_timer_timeout()));
}





/**************************************************************************

	 BACKGROUND ANALYSIS THREAD

**************************************************************************/
WAFMeterThread::WAFMeterThread(WAFMeter * pWAFmeter) {
	m_isRunning = m_run = false;
	m_pWAFmeter = pWAFmeter;
	m_inputImage = m_processedImage = NULL;
	m_iteration = 0;
	m_continuous = false;
	m_period_ms = -1.f;// init <0 so we can set it up at first call
	memset(&m_waf, 0, sizeof(t_waf_info));
}

WAFMeterThread::~WAFMeterThread() {
	stop();
	tmReleaseImage(&m_inputImage);
	tmReleaseImage(&m_processedImage);
}

void WAFMeterThread::setCapture(CvCapture * capture) {
	m_capture = capture;
}

/* Tell the thread to stop */
void WAFMeterThread::stop() {
	m_run = false;
	while(m_isRunning) {
		sleep(1);
	}
}

/** Return a copy of processed image associated with WAF measure */
IplImage * WAFMeterThread::copyInputImage()
{
	if(m_inputImage) {
		m_imageMutex.lock();
		cvCopy(m_inputImage, m_processedImage);
		m_imageMutex.unlock();
	}

	return m_processedImage;
}


/* Thread loop */
void WAFMeterThread::run()
{
	m_isRunning = m_run = true;
	struct timeval tvprev;
	struct timeval tvcur;
	gettimeofday(&tvprev, NULL);
	m_period_ms = -1.f;

	while(m_run) {
		if(m_capture && m_pWAFmeter) {
			IplImage * frame = cvQueryFrame( m_capture );

			if( !frame) { //!cvGrabFrame( m_capture ) ) {
				fprintf(stderr, "%s:%d : capture=%p FAILED\n", __func__, __LINE__, g_capture);
				sleep(1);
				sleep(2);

				m_period_ms = -1.f;
			} else {
//				 fprintf(stderr, "%s:%d : capture=%p ok, iteration=%d\n",
//						__func__, __LINE__, m_capture, m_iteration);
				//IplImage * frame = cvRetrieveFrame( m_capture );
				if(!frame) {
					fprintf(stderr, "%s:%d : cvRetrieveFrame(capture=%p) FAILED\n", __func__, __LINE__, g_capture);
					sleep(1);
				} else {

					if(m_continuous)
					{
						g_processing_mutex.lock();
						m_pWAFmeter->setUnscaledImage(frame);
						m_waf = m_pWAFmeter->getWAF();
						g_processing_mutex.unlock();
					}
					m_iteration++;

					// copy image
					// Check if size changed
					if(m_inputImage &&
							(m_inputImage->width != frame->width
							 || m_inputImage->height != frame->height)) {
						tmReleaseImage(&m_inputImage);
						tmReleaseImage(&m_processedImage);
					}

					if(!m_inputImage) {
						m_inputImage = tmCreateImage(cvGetSize(frame), IPL_DEPTH_8U, frame->nChannels);
						m_processedImage = tmCreateImage(cvGetSize(frame), IPL_DEPTH_8U, frame->nChannels);
						fprintf(stderr, "%s:%d : capture=%p "
								"=> realloc img %dx%dx%d\n", __func__, __LINE__,
								g_capture,
								frame->width, frame->height, frame->nChannels);
					}
					else { // this is not the first acq, so we measure the speed
						gettimeofday(&tvcur, NULL);
						int dt_ms = delta_ms(tvprev, tvcur);
						if(m_period_ms < 0)
						{
							m_period_ms = dt_ms;
						}
						else // slow update
						{
							m_period_ms = 0.9f * m_period_ms + 0.1f * dt_ms;
						}
					}

					m_imageMutex.lock();
					cvCopy(frame, m_inputImage);
					m_imageMutex.unlock();


					tvprev = tvcur;
				}
			}
		} else {
			fprintf(stderr, "%s:%d: have to wait a little (m_capture=%p / m_pWAFmeter=%p)\n",
					__func__, __LINE__,
					m_capture, m_pWAFmeter);
			sleep(1); // Maybe not set yet, wait a little
			m_period_ms = -1.f;
		}
	}

	m_isRunning = false;
}





/*********************************************************
 * Background image recording thread
 *********************************************************/
RecordingThread::RecordingThread()
	: QThread()
{
	m_run = true;
	m_is_running = false;
	start();
}

RecordingThread::~RecordingThread()
{
	/// @todo stop properly ?
	askForStop();

	// Then wait until done
	int retry = 0;
	while(m_is_running && retry < 5)
	{
		retry++;
		sleep(1);
	}

}

void RecordingThread::askForStop()
{
	m_run = false;
}

QString RecordingThread::appendImage(t_waf_info waf, QImage img)
{
	m_imageMutex.lock();

	WAFRecordItem item;
	item.dateTime = QDateTime::currentDateTime();
	item.waf = waf;
	item.img = img.copy();
	item.path = getImagePath(item.dateTime, waf);

	// Append image
	m_saveList.append(item);
	qDebug() << "Added 1 image => " << m_saveList.count() << "Images to save";
	m_imageMutex.unlock();

	m_waitCondition.wakeAll();
	return item.path;
}

void RecordingThread::run()
{
	m_is_running = false;
	while(m_run)
	{
		bool fired = m_waitCondition.wait(&m_waitMutex, 100);
		if(!fired)
		{
			//qDebug() << "timeout";
		} else {
			m_imageMutex.lock();
			fired = (m_saveList.count() > 0);
			m_imageMutex.unlock();
		}

		if(fired) {
			qDebug() << "Recording thread released with " << QString::number(m_saveList.count()) << "items";
			m_imageMutex.lock();
			QList<WAFRecordItem>::iterator it;
			for(it = m_saveList.begin(); it != m_saveList.end(); it++)
			{
				WAFRecordItem item = (*it);
				QImage img = item.img;
				QString img_path = item.path;
				qDebug() << "Saving image " << QString::number(img.width()) << "x" << QString::number(img.height())
							<< " in " << img_path;
				bool ok = img.save(img_path);
				if(!ok)
				{
					qDebug() << "ERROR: cannot save image in " << img_path;
				}
			}
			m_saveList.clear();
			m_imageMutex.unlock();
		}
	}
	fprintf(stderr, "Recording thread ended\n");
}


