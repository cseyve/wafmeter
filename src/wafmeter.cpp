#include "wafmeter.h"
#include "imgutils.h"
#include <highgui.h>

u8 g_debug_WAFMeter = 1;

WAFMeter::WAFMeter()
{
	init();
}
void WAFMeter::init() {
	memset(&m_waf_info, 0, sizeof(t_waf_info));

	m_originalImage = NULL;

	m_scaledImage = m_grayImage = m_HSHistoImage =
	m_HistoImage = m_ColorHistoImage = hsvImage = h_plane = s_plane = NULL;

	m_cannyImage = 	NULL;
}
WAFMeter::~WAFMeter()
{
	purge();
}
void WAFMeter::purge(){
	tmReleaseImage(&m_originalImage);
	tmReleaseImage(&m_HistoImage); // Delete only at the end, because it's always the same size
	purgeScaled();
}

void WAFMeter::purgeScaled() {
	if(m_scaledImage == m_grayImage) {
		m_grayImage = NULL;
	} else {
		tmReleaseImage(&m_grayImage);
	}

	tmReleaseImage(&m_cannyImage);
	tmReleaseImage(&m_scaledImage);
	tmReleaseImage(&m_HSHistoImage);
	tmReleaseImage(&m_HistoImage);
	tmReleaseImage(&hsvImage);
	tmReleaseImage(&h_plane);
	tmReleaseImage(&s_plane);
	tmReleaseImage(&m_ColorHistoImage);
}

int WAFMeter::setImage(IplImage * img) {
	purge();

	if(!img) return -1;
	if(!img->imageData) {
		fprintf(stderr, "WAFMeter::%s:%d : not img->imageData image\n", __func__, __LINE__);

		return -1;
	}
	m_waf_info.waf_factor = 0.2f;
	m_waf_info.color_factor = 0.4f;
	m_waf_info.contour_factor = 0.5f;


	tmReleaseImage(&m_originalImage);
	m_originalImage = tmAddBorder4x(img); // it will purge originalImage
	fprintf(stderr, "WAFMeter::%s:%d : loaded %dx%d x %d\n", __func__, __LINE__,
			m_originalImage->width, m_originalImage->height,
			m_originalImage->nChannels );
#define IMGINFO_WIDTH	400
#define IMGINFO_HEIGHT	400

	// Scale to analysis size
//		cvResize(tmpDisplayImage, new_displayImage, CV_INTER_LINEAR );
	int sc_w = IMGINFO_WIDTH;
	int sc_h = IMGINFO_HEIGHT;
	float scale_factor_w = (float)m_originalImage->width / (float)IMGINFO_WIDTH;
	float scale_factor_h = (float)m_originalImage->height / (float)IMGINFO_HEIGHT;
	if(scale_factor_w > scale_factor_h) {
		// limit w
		sc_w = m_originalImage->width * IMGINFO_HEIGHT/m_originalImage->height;
	} else {
		sc_h = m_originalImage->height * IMGINFO_WIDTH/m_originalImage->width;
	}

	while((sc_w % 4) != 0) { sc_w++; }
	while((sc_h % 4) != 0) { sc_h++; }

	if(m_scaledImage
	   && (m_scaledImage->width != sc_w || m_scaledImage->height != sc_h)) {
		// purge scaled images
		purgeScaled();
	}

	// Scale original image to smaller image to fasten later processinggs
	if(!m_scaledImage) {
		m_scaledImage = tmCreateImage(cvSize(sc_w, sc_h),
								  IPL_DEPTH_8U, m_originalImage->nChannels);
	}

	cvResize(m_originalImage, m_scaledImage);

	fprintf(stderr, "WAFMeter::%s:%d : scaled to %dx%d\n", __func__, __LINE__,
			m_scaledImage->width, m_scaledImage->height);

	fprintf(stderr, "\nWAFMeter::%s:%d : processHSV(m_scaledImage=%dx%d)\n", __func__, __LINE__,
			m_scaledImage->width, m_scaledImage->height);fflush(stderr);



	// Process color analysis
	processHSV();

	m_waf_info.contour_factor = 0.6f;
	// Process contour/shape analysis
	processContour();

	m_waf_info.waf_factor = m_waf_info.color_factor * m_waf_info.contour_factor;

	// ok, we're done
	return 0;
}
/* Contour/shape analysis */
int WAFMeter::processContour() {
	if(!m_scaledImage) {
		fprintf(stderr, "WAFMeter::%s:%d : no scaled image\n", __func__, __LINE__);

		return -1;
	}

	//
	if(!m_cannyImage) {
		m_cannyImage = 	tmCreateImage(
				cvSize(m_scaledImage->width, m_scaledImage->height),
				IPL_DEPTH_8U,
				1 );
	}

	cvCanny(m_grayImage, m_cannyImage, 1500, 500, 5);
	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "Canny.pgm", m_cannyImage);
	}

	// Study contour shape
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq * contours = NULL;

	cvFindContours( m_cannyImage, storage, &contours, sizeof(CvContour),
					CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

	// comment this out if you do not want approximation
	contours = cvApproxPoly( contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 3, 1 );

	IplImage* cnt_img = tmCreateImage( cvSize(m_cannyImage->width,m_cannyImage->height), 8, 3 );
	int levels = 3;
	CvSeq* _contours = contours;
	int _levels = levels ;//- 3;
	if( _levels <= 0 ) // get to the nearest face to make it look more funny
		_contours = _contours->h_next->h_next->h_next;

	cvZero( cnt_img );
	cvDrawContours( cnt_img, _contours, CV_RGB(255,0,0), CV_RGB(0,255,0), _levels, 3, CV_AA, cvPoint(0,0) );

	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "CannyContours.pgm", cnt_img);
	}
	cvReleaseImage( &cnt_img );

	cvReleaseMemStorage(&storage);
	return 0;
}



int WAFMeter::processHSV() {
	if(!m_scaledImage) {
		fprintf(stderr, "WAFMeter::%s:%d : no scaled image\n", __func__, __LINE__);

		return -1;
	}

	// Change to HSV
	if(m_scaledImage->nChannels < 3) {
		// Clear histogram and return
		fprintf(stderr, "WAFMeter::%s:%d : not coloured image : nChannels=%d\n", __func__, __LINE__,
			m_scaledImage->nChannels );
		m_grayImage = m_scaledImage;
		return 0;
	}
/*
void cvCvtColor( const CvArr* src, CvArr* dst, int code );

RGB<=>HSV (CV_BGR2HSV, CV_RGB2HSV, CV_HSV2BGR, CV_HSV2RGB)


// In case of 8-bit and 16-bit images
// R, G and B are converted to floating-point format and scaled to fit 0..1 range
The values are then converted to the destination data type:
	8-bit images:
		V <- V*255, S <- S*255, H <- H/2 (to fit to 0..255)

  */


	if(!hsvImage) {
		hsvImage = tmCreateImage(
			cvSize(m_scaledImage->width, m_scaledImage->height),
			IPL_DEPTH_8U,
			m_scaledImage->nChannels);
	}

	if(m_scaledImage->nChannels == 3) {
		//cvCvtColor(m_scaledImage, hsvImage, CV_RGB2HSV);
		cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HSV);
	} else {
		cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HSV);
	}

	if(!h_plane) h_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!s_plane) s_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!m_grayImage) { // vplane
		m_grayImage = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	}

	cvCvtPixToPlane( hsvImage, h_plane, s_plane, m_grayImage, 0 );

	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "HImage.pgm", h_plane);
		tmSaveImage(TMP_DIRECTORY "SImage.pgm", s_plane);
		tmSaveImage(TMP_DIRECTORY "VImage.pgm", m_grayImage);
	}


	// save image for debug
	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "HSVimage.ppm", hsvImage);
		if(!m_HSHistoImage) {
			m_HSHistoImage = tmCreateImage(cvSize(H_MAX, S_MAX), IPL_DEPTH_8U, 1);
		} else
			cvZero(m_HSHistoImage);
	}
	double color_factor = 0.;
	int color_factor_nb = 0;

	// Then process histogram
	for(int r=0; r<hsvImage->height; r++) {
		u8 * hline = (u8 *)(h_plane->imageData
										+ r * h_plane->widthStep);
		u8 * sline = (u8 *)(s_plane->imageData
										+ r * s_plane->widthStep);
		u8 * vline = (u8 *)(m_grayImage->imageData
										+ r * m_grayImage->widthStep);
		for(int c = 0; c<s_plane->width; c++) {
			int h = (int)(hline[c]);
			int s = (int)(sline[c]);
			int v = (int)(vline[c]);

			if(//s > 64 && // only the saturated tones
				v > 64 // and not dark enough to be a noise color artefact
			) {
				color_factor += //(double)v / 255. *
						// wafscale(h) *
						(double)s / 255.;
				// FIXME : add WAF colors coef
				color_factor_nb++;
			}

			if(g_debug_WAFMeter) {
				if(h<m_HSHistoImage->widthStep
				   && s<m_HSHistoImage->height) {
					// Increase image
					u8 * pHisto = // H as columns, S as line
							(u8 *)(m_HSHistoImage->imageData + s * m_HSHistoImage->widthStep)
							+ h;
					u8 val = *pHisto;
					if(val < 255) {
						*pHisto = val+1;
					}
				}
			}
		}
	}

	if(color_factor_nb > 0) {
		m_waf_info.color_factor = color_factor * 3 //* (double)color_factor_nb
								  / (double)(s_plane->width*s_plane->height);
	}

	// save image for debug
	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "HSHisto.pgm", m_HSHistoImage);

		IplImage * hsvOutImage = tmCreateImage(
				cvSize(H_MAX, S_MAX),
				IPL_DEPTH_8U,
				3);
		// Fill with H,S and use Value for highlighting colors
		for(int r=0; r<hsvOutImage->height; r++) {
			u8 * outline = (u8 *)(hsvOutImage->imageData
								  + r * hsvOutImage->widthStep);
			u8 * histoline = (u8 *)(m_HSHistoImage->imageData
									+ r * m_HSHistoImage->widthStep);
			int c1 = 0;
			for(int c = 0; c1<H_MAX;
				c1++, c+=hsvOutImage->nChannels) {
				outline[c] = c1; // H
				outline[c+1] = r; // S
				outline[c+2] = 64 + (int)tmmin( (float)histoline[c1]*2.f, 191.f);
				//histoline[c1]>1 ? 255 : 64; //histoline[c1];
			}
		}
		if(!m_ColorHistoImage) {
			m_ColorHistoImage = tmCreateImage(
					cvSize(H_MAX, S_MAX),
					IPL_DEPTH_8U,
					3);
		} else
			cvZero(m_ColorHistoImage);

		cvCvtColor(hsvOutImage, m_ColorHistoImage, CV_HSV2BGR);
		//tmSaveImage(TMP_DIRECTORY "HSHistoColored.ppm", m_ColorHistoImage);
		if(g_debug_WAFMeter) {
			tmSaveImage(TMP_DIRECTORY "HSHistoHSV.ppm", hsvOutImage);
		}
		tmReleaseImage(&hsvOutImage);
	}
	return 0;
}
