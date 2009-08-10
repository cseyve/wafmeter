#include "wafmainwindow.h"
#include "ui_wafmainwindow.h"
#include <QtGui/QFileDialog>
#include <highgui.h>

WAFMainWindow::WAFMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::WAFMainWindow)
{
    ui->setupUi(this);
}

WAFMainWindow::~WAFMainWindow()
{
    delete ui;
}

void WAFMainWindow::on_fileButton_pressed() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
						 ".",
						 tr("Images (*.png *.xpm *.jpg)"));
	if(fileName.isEmpty()) return;

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

