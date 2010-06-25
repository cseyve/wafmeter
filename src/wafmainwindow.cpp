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
#include <QtGui/QFileDialog>
#include "imgutils.h"
#include <QPainter>
#include <QDateTime>
#include <QDesktopWidget>

u8 mode_file = 0;

WAFMainWindow::WAFMainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::WAFMainWindow)
{
	char home[512]=".";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	m_path = QString(home);
	ui->setupUi(this);

	m_pWAFMeterThread = NULL;

	QString filename=":/qss/WAFMeter.qss";
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(file.readAll());
	//setStyleSheet(styleSheet);

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_m_timer_timeout()));

        decorImage.load(":icons/Interface-montage.png");
        decorImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

WAFMainWindow::~WAFMainWindow()
{
	delete ui;
}



void WAFMainWindow::on_fileButton_pressed() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
						 m_path,
						 tr("Images (*.png *.xpm *.jpg)"));
	if(fileName.isEmpty()) return;

	QFileInfo fi(fileName);
	if(!fi.exists()) return;

	m_path = fi.absolutePath();

	// load file
	IplImage * iplImage = cvLoadImage(fileName.ascii());
	computeWAF(iplImage);

	tmReleaseImage(&iplImage);
}



typedef uint32_t u32;

static u32 * grayToBGR32 = NULL;

static void init_grayToBGR32()
{
	if(grayToBGR32) {
		return;
	}

	grayToBGR32 = new u32 [256];
	for(int c = 0; c<256; c++) {
		int Y = c;
		u32 B = Y;// FIXME
		u32 G = Y;
		u32 R = Y;
		grayToBGR32[c] = (R << 16) | (G<<8) | (B<<0);
	}

}

QImage iplImageToQImage(IplImage * iplImage) {
	if(!iplImage)
		return QImage();


	int depth = iplImage->nChannels;

	bool rgb24_to_bgr32 = false;
	if(depth == 3  ) {// RGB24 is obsolete on Qt => use 32bit instead
		depth = 4;
		rgb24_to_bgr32 = true;
	}

	u32 * grayToBGR32palette = grayToBGR32;
	bool gray_to_bgr32 = false;

	if(depth == 1) {// GRAY is obsolete on Qt => use 32bit instead
		depth = 4;
		gray_to_bgr32 = true;

		init_grayToBGR32();
		grayToBGR32palette = grayToBGR32;
	}

	int orig_width = iplImage->width;
//	if((orig_width % 2) == 1)
//		orig_width--;


	QImage qImage(orig_width, iplImage->height, 8 * depth);
	memset(qImage.bits(), 0, orig_width*iplImage->height*depth);


	switch(iplImage->depth) {
	default:
		fprintf(stderr, "[TamanoirApp]::%s:%d : Unsupported depth = %d\n", __func__, __LINE__, iplImage->depth);
		break;

	case IPL_DEPTH_8U: {
		if(!rgb24_to_bgr32 && !gray_to_bgr32) {
			if(iplImage->nChannels != 4) {
				for(int r=0; r<iplImage->height; r++) {
					// NO need to swap R<->B
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);
				}
			} else {
				for(int r=0; r<iplImage->height; r++) {
					// need to swap R<->B
					u8 * buf_out = (u8 *)(qImage.bits()) + r*orig_width*depth;
					u8 * buf_in = (u8 *)(iplImage->imageData) + r*iplImage->widthStep;
					memcpy(qImage.bits() + r*orig_width*depth,
						iplImage->imageData + r*iplImage->widthStep,
						orig_width*depth);
/*
					for(int pos4 = 0 ; pos4<orig_width*depth; pos4+=depth,
						buf_out+=4, buf_in+=depth
						 ) {
						buf_out[2] = buf_in[0];
						buf_out[0] = buf_in[2];
                                        }
*/				}
			}
		}
		else if(rgb24_to_bgr32) {
			// RGB24 to BGR32
			u8 * buffer3 = (u8 *)iplImage->imageData;
			u8 * buffer4 = (u8 *)qImage.bits();
			int orig_width4 = 4 * orig_width;

			for(int r=0; r<iplImage->height; r++)
			{
				int pos3 = r * iplImage->widthStep;
				int pos4 = r * orig_width4;
				for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
				{
					//buffer4[pos4 + 2] = buffer3[pos3];
					//buffer4[pos4 + 1] = buffer3[pos3+1];
					//buffer4[pos4    ] = buffer3[pos3+2];
					buffer4[pos4   ] = buffer3[pos3];
					buffer4[pos4 + 1] = buffer3[pos3+1];
					buffer4[pos4 + 2] = buffer3[pos3+2];
				}
			}
		} else if(gray_to_bgr32) {
			for(int r=0; r<iplImage->height; r++)
			{
				u32 * buffer4 = (u32 *)qImage.bits() + r*qImage.width();
				u8 * bufferY = (u8 *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<orig_width; c++) {
					buffer4[c] = grayToBGR32palette[ (int)bufferY[c] ];
				}
			}
		}
		}break;
	case IPL_DEPTH_16S: {
		if(!rgb24_to_bgr32) {

			u8 * buffer4 = (u8 *)qImage.bits();
			short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				short * buffershort = (short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0)
				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData
									+ r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width;
					for(int c=0; c<orig_width; c++, pos3++, pos4++)
					{
						int val = abs((int)buffer3[pos3]) * 255 / valmax;
						if(val > 255) val = 255;
						buffer4[pos4] = (u8)val;
					}
				}
		}
		else {
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3];
						buffer4[pos4 + 1] = buffer3[pos3+1];
						buffer4[pos4 + 2] = buffer3[pos3+2];
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0) {
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				}
			}
		}
		}break;
	case IPL_DEPTH_16U: {
		if(!rgb24_to_bgr32) {

			unsigned short valmax = 0;

			for(int r=0; r<iplImage->height; r++)
			{
				unsigned short * buffershort = (unsigned short *)(iplImage->imageData + r*iplImage->widthStep);
				for(int c=0; c<iplImage->width; c++)
					if(buffershort[c]>valmax)
						valmax = buffershort[c];
			}

			if(valmax>0) {
				if(!gray_to_bgr32) {
					u8 * buffer4 = (u8 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
				} else {
					u32 * buffer4 = (u32 *)qImage.bits();
					for(int r=0; r<iplImage->height; r++)
					{
						unsigned short * buffer3 = (unsigned short *)(iplImage->imageData
										+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = grayToBGR32palette[ val ];
						}
					}
				}
			}
		}
		else {
			fprintf(stderr, "[TamanoirApp]::%s:%d : U16  depth = %d -> BGR32\n", __func__, __LINE__, iplImage->depth);
			u8 * buffer4 = (u8 *)qImage.bits();
			if(depth == 3) {

				for(int r=0; r<iplImage->height; r++)
				{
					short * buffer3 = (short *)(iplImage->imageData + r * iplImage->widthStep);
					int pos3 = 0;
					int pos4 = r * orig_width*4;
					for(int c=0; c<orig_width; c++, pos3+=3, pos4+=4)
					{
						buffer4[pos4   ] = buffer3[pos3]/256;
						buffer4[pos4 + 1] = buffer3[pos3+1]/256;
						buffer4[pos4 + 2] = buffer3[pos3+2]/256;
					}
				}
			} else if(depth == 1) {
				short valmax = 0;
				short * buffershort = (short *)(iplImage->imageData);
				for(int pos=0; pos< iplImage->widthStep*iplImage->height; pos++)
					if(buffershort[pos]>valmax)
						valmax = buffershort[pos];

				if(valmax>0)
					for(int r=0; r<iplImage->height; r++)
					{
						short * buffer3 = (short *)(iplImage->imageData
											+ r * iplImage->widthStep);
						int pos3 = 0;
						int pos4 = r * orig_width;
						for(int c=0; c<orig_width; c++, pos3++, pos4++)
						{
							int val = abs((int)buffer3[pos3]) * 255 / valmax;
							if(val > 255) val = 255;
							buffer4[pos4] = (u8)val;
						}
					}
			}
		}
		}break;
	}

	if(qImage.depth() == 8) {
		qImage.setNumColors(256);

		for(int c=0; c<256; c++) {
			/* False colors
			int R=c, G=c, B=c;
			if(c<128) B = (255-2*c); else B = 0;
			if(c>128) R = 2*(c-128); else R = 0;
			G = abs(128-c)*2;

			qImage.setColor(c, qRgb(R,G,B));
			*/
			qImage.setColor(c, qRgb(c,c,c));
		}
	}
	return qImage;
}


void WAFMainWindow::computeWAF(IplImage * iplImage) {
	if(!iplImage) return;


	// compute waf
	if(mode_file) {
		m_wafMeter.setUnscaledImage(iplImage);
	} else {
		m_wafMeter.setScaledImage(iplImage);
	}
	t_waf_info waf = m_wafMeter.getWAF();

	// Display images
	displayWAFMeasure(waf, iplImage);
}

void WAFMainWindow::on_deskButton_clicked() {
    QPoint curpos = QApplication::activeWindow()->pos();
    QRect currect = QApplication::activeWindow()->rect();
	fprintf(stderr, "%s:%d: curpos=%dx%d +%dx%d\n",
            __func__, __LINE__,
            curpos.x(), curpos.y(),
            currect.width(), currect.height());

    m_grabRect = QRect(curpos.x(), curpos.y(),
                       currect.width(), currect.height());
    hide();
    QTimer::singleShot(10, this, SLOT(on_grabTimer_timeout()));
}

void WAFMainWindow::on_grabTimer_timeout() {

    QPixmap desktopPixmap = QPixmap::grabWindow(
            //QApplication::activeWindow()->winId());
            QApplication::desktop()->winId());
    fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
            __func__, __LINE__,
            desktopPixmap.width(), desktopPixmap.height(), desktopPixmap.depth());

    //QApplication::activeWindow()->show();

    // Crop where the window is

	fprintf(stderr, "%s:%d: curpos=%dx%d+%dx%d\n",
            __func__, __LINE__,
            m_grabRect.x(), m_grabRect.y(),
            m_grabRect.width(), m_grabRect.height());

    QImage desktopImage( desktopPixmap.copy(m_grabRect) );

    if(desktopImage.isNull()) return;
    fprintf(stderr, "%s:%d: desktopImage=%dx%dx%d\n",
            __func__, __LINE__,
            desktopImage.width(), desktopImage.height(), desktopImage.depth());


    IplImage * header = cvCreateImageHeader(cvSize(desktopImage.width(), desktopImage.height()),
                                            IPL_DEPTH_8U, desktopImage.depth()/8);
    header->imageData = (char *)desktopImage.bits();
    m_wafMeter.setUnscaledImage(header);
    t_waf_info waf = m_wafMeter.getWAF();

    show();

    // Display images
    displayWAFMeasure(waf, header);

    cvReleaseImageHeader(&header);
}

void WAFMainWindow::on_snapButton_clicked() {
    if(resultImage.isNull()) return;

    // Assemblate file name
    QDir snapDir(QDir::homePath());
    snapDir.cd(tr("Desktop"));


    QString wafStr;
    wafStr.sprintf("%02d", (int)roundf(m_waf.waf_factor*100.f));
    QString dateStr = QDateTime::currentDateTime().toString(tr("yyyy-MM-dd_hhmmss"));

    QString filepath = snapDir.absoluteFilePath("WAF_" + dateStr + "waf=" + wafStr + "perc.png");
    qDebug("Saving '" + filepath + "' as PNG");
    resultImage.save( filepath,
                      "PNG");
}

void WAFMainWindow::displayWAFMeasure(t_waf_info waf, IplImage * iplImage)
{
	// Display
	if(!iplImage) return;
        m_waf = waf;
	// load image to display
	QImage qtImage = iplImageToQImage(iplImage);
	QImage scaledImg = qtImage.scaled(
				ui->imageLabel->width(),
				ui->imageLabel->height(),
                                Qt::KeepAspectRatio
				);
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

//	QImage resultImage(ui->imageLabel->width(),
//					   ui->imageLabel->height(),
//                                           QImage::Format_ARGB32_Premultiplied);
        resultImage = qtImage.scaled(
                ui->imageLabel->width(),
                ui->imageLabel->height(),
                Qt::IgnoreAspectRatio
                );

	QPainter painter(&resultImage);

        QImage destinationImage = decorImage;
        QImage sourceImage = scaledImg;
        sourceImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

	//QImage alphaMask = decorImage.createMaskFromColor(qRgb(0,0,0));
	//decorImage.setAlphaChannel(alphaMask);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(resultImage.rect(), Qt::transparent);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(resultImage.rect(), Qt::white);
	painter.drawImage((resultImage.width() - sourceImage.width())/2,
					  0, sourceImage);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.drawImage(0, 0, destinationImage);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
//        painter.fillRect(resultImage.rect(), Qt::white);

	painter.setCompositionMode(QPainter::CompositionMode_Source);


        // Display waf with dial ----------------------------------------------
        painter.setRenderHint(QPainter::Antialiasing, true);
        // Margin on both sides of of the dial
	float theta_min = 0.05;
	float theta = (3.1415927-2*theta_min) * (1. - waf.waf_factor) + theta_min;
	float r = 130.f;

	// Center of background image : 160, 416
	QPen pen(qRgb(255,0,0));
	pen.setWidth(2);
	pen.setCapStyle(Qt::RoundCap);

	// Draw main result first, to draw smaller dial over it
	pen.setColor(qRgb(0,0,0));
	pen.setColor(qRgba(10,10,10, 32));
	pen.setWidth(5);
	painter.setPen(pen);
	r = 140;
	theta = (3.1415927-2*theta_min) * (1. - waf.waf_factor) + theta_min;
	painter.drawLine(QPoint(160, 416),
					 QPoint(160+r * cos(theta), 416-r*sin(theta)));
	r = 125;

	// draw shape with gray
	pen.setColor(qRgb(214,214,214));
	pen.setWidth(3);
	painter.setPen(pen);
	r = 120;
	theta = (3.1415927-2*theta_min) * (1. - waf.contour_factor) + theta_min;
	painter.drawLine(QPoint(160, 416),
					 QPoint(160+r * cos(theta), 416-r*sin(theta)));

	// draw color with green
	pen.setColor(qRgb(127,255,0));
	r = 120;
	theta = (3.1415927-2*theta_min) * (1. - waf.color_factor) + theta_min;
	pen.setWidth(3);
	painter.setPen(pen);
	painter.drawLine(QPoint(160, 416),
					 QPoint(160+r * cos(theta), 416-r*sin(theta)));
	r = 100;
	pen.setWidth(6);
	painter.setPen(pen);
	painter.drawEllipse(QPoint(160+r * cos(theta), 416-r*sin(theta)), 3, 3);



	pen.setWidth(16);
	pen.setColor(qRgb(127,127,127));
	painter.setPen(pen);
        painter.drawEllipse(QPoint(160+1, 416-1), 8,8);
	pen.setColor(qRgba(10,10,10, 128));
	painter.setPen(pen);
	painter.drawEllipse(QPoint(160, 416), 8,8);
        painter.end();


        QPixmap localPixmap = QPixmap::fromImage(resultImage);
        ui->imageLabel->setPixmap(localPixmap);

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
}

CvCapture* capture = 0;

void WAFMainWindow::on_camButton_toggled(bool checked)
{
	if(!checked) {
		if(m_timer.isActive())
			m_timer.stop();
		// Stop thread
		if(m_pWAFMeterThread) {
			m_pWAFMeterThread->stop();
			m_pWAFMeterThread->setCapture(NULL);
		}
		cvReleaseCapture(&capture);
		capture = NULL;

		return;
	}

	if(!capture) {
                capture = cvCreateCameraCapture (CV_CAP_ANY);
                fprintf(stderr, "%s:%d : capture=%p\n", __func__, __LINE__, capture);
	}
	if(!capture) { return ; }

	startBackgroundThread();
}

void WAFMainWindow::on_movieButton_toggled(bool checked)
{
	if(!checked) {
		if(m_timer.isActive())
			m_timer.stop();

		// Stop thread
		if(m_pWAFMeterThread) {
			m_pWAFMeterThread->stop();
			m_pWAFMeterThread->setCapture(NULL);
		}
		cvReleaseCapture(&capture);
		capture = NULL;

		return;
	}

	if(!capture) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
							 m_path,
							 tr("Movies (*.avi *.mp* *.mov)"));
		if(fileName.isEmpty()) return;

		QFileInfo fi(fileName);
		if(!fi.exists()) return;

		m_path = fi.absolutePath();

		capture = cvCaptureFromFile( fi.absoluteFilePath().toUtf8().data() );

		fprintf(stderr, "%s:%d : capture=%p\n", __func__, __LINE__, capture);
		startBackgroundThread();
	}
}

void WAFMainWindow::startBackgroundThread() {

	if(!capture) { return ; }

	if(!m_pWAFMeterThread) {
		m_pWAFMeterThread = new WAFMeterThread(&m_wafMeter);
	}

	m_pWAFMeterThread->setCapture(capture);
	m_pWAFMeterThread->start();

	if(!m_timer.isActive()) {
		m_timer.start(100);
	}

}

void WAFMainWindow::on_m_timer_timeout() {
	if(!capture) {
		m_timer.stop();
		return;
	}
	if(!m_pWAFMeterThread) {
		m_timer.stop();
		return;
	}

	// Grab image
	int cur_iteration = m_pWAFMeterThread->getIteration();
	if(m_lastIteration != cur_iteration) {
		m_lastIteration = cur_iteration;
		// Display image
		displayWAFMeasure(m_pWAFMeterThread->getWAF(), m_pWAFMeterThread->getInputImage());
	} else {
		//
		fprintf(stderr, "%s:%d no fast enough\n", __func__, __LINE__);
	}
}





/**************************************************************************

					BACKGROUND ANALYSIS THREAD

**************************************************************************/
WAFMeterThread::WAFMeterThread(WAFMeter * pWAFmeter) {
	m_isRunning = m_run = false;
	m_pWAFmeter = pWAFmeter;
	m_inputImage = NULL;
	m_iteration = 0;
	memset(&m_waf, 0, sizeof(t_waf_info));
}

WAFMeterThread::~WAFMeterThread() {
	stop();
	tmReleaseImage(&m_inputImage);
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

/* Thread loop */
void WAFMeterThread::run()
{
	m_isRunning = m_run = true;

	while(m_run) {
		if(m_capture && m_pWAFmeter) {
                       IplImage * frame = cvQueryFrame( m_capture );

                       if( !frame) { //!cvGrabFrame( m_capture ) ) {
				fprintf(stderr, "%s:%d : capture=%p FAILED\n", __func__, __LINE__, capture);
                                sleep(1);
				sleep(2);

			} else {
				// fprintf(stderr, "%s:%d : capture=%p ok, iteration=%d\n",
				//		__func__, __LINE__, m_capture, m_iteration);
                                //IplImage * frame = cvRetrieveFrame( m_capture );
                                 if(!frame) {
                                    fprintf(stderr, "%s:%d : cvRetrieveFrame(capture=%p) FAILED\n", __func__, __LINE__, capture);
                                    sleep(1);
                                } else {
                                    m_pWAFmeter->setUnscaledImage(frame);
                                    m_waf = m_pWAFmeter->getWAF();
                                    m_iteration++;

                                    // copy image
                                    // Check if size changed
                                    if(m_inputImage &&
                                       (m_inputImage->width != frame->width
                                            || m_inputImage->height != frame->height)) {
                                            tmReleaseImage(&m_inputImage);
                                    }

                                    if(!m_inputImage) {
                                            m_inputImage = tmCreateImage(cvGetSize(frame), IPL_DEPTH_8U, frame->nChannels);
                                            fprintf(stderr, "%s:%d : capture=%p "
                                                            "=> realloc img %dx%dx%d\n", __func__, __LINE__,
                                                            capture,
                                                            frame->width, frame->height, frame->nChannels);
                                    }
                                    cvCopy(frame, m_inputImage);
                                }
			}
		} else {
			sleep(1);
		}
	}

	m_isRunning = false;
}


