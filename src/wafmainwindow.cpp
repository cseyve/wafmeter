#include "wafmainwindow.h"
#include "ui_wafmainwindow.h"
#include <QtGui/QFileDialog>
#include <highgui.h>

WAFMainWindow::WAFMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::WAFMainWindow)
{
	char home[512]=".";
	if(getenv("HOME")) {
		strcpy(home, getenv("HOME"));
	}
	m_path = QString(home);
    ui->setupUi(this);
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
	if(!iplImage) return;

	// load image to display
	QImage qtImage(fileName);
	QImage scaledImg = qtImage.scaled(
				ui->imageLabel->width(),
				ui->imageLabel->height(),
				Qt::KeepAspectRatio
				);
	ui->imageLabel->setPixmap(QPixmap::fromImage(scaledImg));

	// compute waf
	m_wafMeter.setImage(iplImage);

	t_waf_info waf = m_wafMeter.getWAF();

	// Display waf
	ui->wafProgressBar->setValue(100.f * waf.waf_factor);
	ui->colorProgressBar->setValue(100.f * waf.color_factor);
	ui->contourProgressBar->setValue(100.f * waf.contour_factor);

	cvReleaseImage(&iplImage);
}

