/***************************************************************************
 *  wafmeter - Woman Acceptance Factor measurement / image processing class
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

#include "wafmeter.h"
#include "imgutils.h"
#include <highgui.h>

#define MUNORM(a) (((a)<0.f) ? 0.f : ( ((a)>1.f) ? 1.f : (a)))


u8 g_debug_WAFMeter = 0;
u8 g_mode_realtime = 0;


WAFMeter::WAFMeter()
{
	init();
}

void WAFMeter::init() {
	memset(&m_waf_info, 0, sizeof(t_waf_info));
	memset(&m_details, 0, sizeof(t_waf_info_details));
	m_originalImage = NULL;
	m_scaledImage_allocated = false;
	m_scaledImage = m_grayImage = m_HSHistoImage =
	m_HistoImage = m_ColorHistoImage =
				   hsvImage = h_plane = s_plane = v_plane = NULL;
	m_process_counter = 0;
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
	if(m_scaledImage) {
		if(m_scaledImage == m_grayImage) {
			m_scaledImage = NULL;
		} else if(m_scaledImage_allocated) {
			tmReleaseImage(&m_scaledImage);
		}
	}

	if(g_debug_WAFMeter) {
		fprintf(stderr, "WAFMeter::%s:%d : purging scaled images\n",
				__func__, __LINE__);
	}

	m_process_counter = 0;

	m_waf_info.waf_factor = 0.f;
	m_waf_info.color_factor = 0.f;
	m_waf_info.contour_factor = 0.f;

	memset(&m_details, 0, sizeof(t_waf_info_details));

        tmReleaseImage(&m_grayImage);
        tmReleaseImage(&m_cannyImage);
        tmReleaseImage(&m_scaledImage);
	tmReleaseImage(&m_HSHistoImage);
	tmReleaseImage(&m_HistoImage);
	tmReleaseImage(&hsvImage);
	tmReleaseImage(&h_plane);
	tmReleaseImage(&s_plane);
	tmReleaseImage(&v_plane);
	tmReleaseImage(&m_ColorHistoImage);
}

int WAFMeter::setScaledImage(IplImage * img) {
	if(!img) {
		fprintf(stderr, "WAFMeter::%s:%d : no img\n", __func__, __LINE__);

		return -1;
	}

	return processImage(img);
}

int WAFMeter::setUnscaledImage(IplImage * img) {
	if(!img) {
		fprintf(stderr, "WAFMeter::%s:%d : no img\n", __func__, __LINE__);

		return -1;
	}

	if(m_originalImage) {
		if(abs(m_originalImage->width - img->width)>=4
		   || m_originalImage->height != img->height
		   ) {
			purge();
		}
	}

	if(!img->imageData) {
		fprintf(stderr, "WAFMeter::%s:%d : not img->imageData image\n", __func__, __LINE__);

		return -1;
	}

#define IMGINFO_WIDTH	400
#define IMGINFO_HEIGHT	400
	tmReleaseImage(&m_originalImage);
	m_originalImage = tmAddBorder4x(img); // it will purge originalImage
	if(g_debug_WAFMeter) {
		fprintf(stderr, "WAFMeter::%s:%d : loaded %dx%d x %d\n", __func__, __LINE__,
			m_originalImage->width, m_originalImage->height,
			m_originalImage->nChannels );
	}

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
		m_scaledImage_allocated = true;
		m_scaledImage = tmCreateImage(cvSize(sc_w, sc_h),
                                              IPL_DEPTH_8U, m_originalImage->nChannels);
	}

	// Resize to processing scale
	cvResize(m_originalImage, m_scaledImage);

	if(g_debug_WAFMeter) {
                fprintf(stderr, "WAFMeter::%s:%d : scaled to %dx%d x %d\n", __func__, __LINE__,
                        m_scaledImage->width, m_scaledImage->height,
                        m_scaledImage->nChannels);

		fprintf(stderr, "\nWAFMeter::%s:%d : processHSV(m_scaledImage=%dx%d)\n", __func__, __LINE__,
			m_scaledImage->width, m_scaledImage->height);fflush(stderr);
	}

	return processImage(m_scaledImage);
}

int WAFMeter::processImage(IplImage * scaledImage) {
	if(!scaledImage) { return -1; }

	if((m_scaledImage_allocated
			&& m_scaledImage != scaledImage)
		|| !m_scaledImage ) {
		purgeScaled();

		m_scaledImage_allocated = false;
		m_scaledImage = scaledImage;
	}

	// in real-time mode, we do not process every image at every iteration
	bool do_process_hsv_now = true;
	bool do_process_canny_now = true;
	bool do_process_shape_now = true;

	if(g_mode_realtime // only on real time mode
	   && m_process_counter >= 1 // and if we already have a value for both fields
	   ) {
		do_process_hsv_now = ((m_process_counter % 3)==1);
		do_process_canny_now = ((m_process_counter % 3)==2);
		do_process_shape_now = ((m_process_counter % 3)==0);
	}
	m_process_counter ++;

	// Process color analysis
	if(do_process_hsv_now) {
		processHSV();
	}

	// Process contour/shape analysis
	if(do_process_canny_now) {
		processCanny();
	}

	// Process contour/shape analysis
	if(do_process_shape_now) {
		processContour();
	}


	m_details.mu_color = m_waf_info.color_factor;

	m_waf_info.waf_factor =
			//(- (m_waf_info.color_factor )*( m_waf_info.color_factor ))
			// AND = multiply
			// ( (m_waf_info.contour_factor )*( m_waf_info.color_factor ));
			// Quadratic mean :
			sqrtf(
					(double)(m_waf_info.contour_factor )*(double)( m_waf_info.contour_factor )
					+ (double)(m_waf_info.color_factor )*(double)( m_waf_info.color_factor ))
			//1.f -
			//(1.f - m_waf_info.contour_factor )
			//	* (1.f - m_waf_info.color_factor)
			;
	m_waf_info.waf_factor = sqrtf(m_waf_info.contour_factor )
					* sqrtf(m_waf_info.color_factor);
//	m_waf_info.waf_factor = (m_waf_info.contour_factor +m_waf_info.color_factor )*0.5f;
	// ok, we're done
	return 0;
}

/*
 * Normalize vector
 */
CvPoint2D32f norm_vect(CvPoint pt1, CvPoint pt2) {
	float dx = pt2.x - pt1.x;
	float dy = pt2.y - pt1.y;
	float norm = sqrtf(dx*dx+dy*dy);
	CvPoint2D32f ret;
	ret.x = dx/norm;
	ret.y = dy/norm;

	return ret;
}

float scalar_vect(CvPoint2D32f v1, CvPoint2D32f v2) {
	float scalar = v1.x * v2.x + v1.y * v2.y;

	return scalar;
}

/* Contour/shape analysis */
int WAFMeter::processCanny() {
	if(!m_cannyImage) {
		m_cannyImage = 	tmCreateImage(
				cvSize(m_scaledImage->width, m_scaledImage->height),
				IPL_DEPTH_8U,
				1 );
	}

	cvCanny(m_grayImage, m_cannyImage, 1500, 500, 5);
	return 0;
}

/* Contour/shape analysis */
int WAFMeter::processContour() {
	if(!m_scaledImage) {
		fprintf(stderr, "WAFMeter::%s:%d : no scaled image\n", __func__, __LINE__);

		return -1;
	}

	if(!m_cannyImage) {
		processCanny();
	}


	// Study contour shape
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq * contours = NULL;

	cvFindContours( m_cannyImage, storage, &contours, sizeof(CvContour),
					CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
	if(!contours) return -1;

	// comment this out if you do not want approximation
//	contours = cvApproxPoly( contours, sizeof(CvContour),
//							 storage, CV_POLY_APPROX_DP, 3, 1 );

	int levels = 3;
	CvSeq* _contours = contours;
	int _levels = levels ;//- 3;
	if( _levels <= 0 ) // get to the nearest face to make it look more funny
		_contours = _contours->h_next->h_next->h_next;

	IplImage* cnt_img = NULL;
	if(g_debug_WAFMeter) {
		cnt_img = tmCreateImage( cvSize(m_cannyImage->width,m_cannyImage->height), 8, 3 );
//		cvZero( cnt_img );
//		cvDrawContours( cnt_img, _contours, CV_RGB(255,0,0), CV_RGB(0,255,0), _levels, 3, CV_AA, cvPoint(0,0) );
	}

	// Analyse contour shape
	CvContour* contour = 0;
	CvSeqWriter writer;
	CvTreeNodeIterator iterator;
	int i;

	int nb_contours = 0;
	int nb_segments = 0;
	int maxLevel = 3;

	double scalar_mean = 0.;
	int scalar_mean_nb = 0;

	if(g_debug_WAFMeter) {
		tmSaveImage(TMP_DIRECTORY "Canny.pgm", m_cannyImage);
	}
	cvZero(m_cannyImage);

	double penality_sum = 0.;
	int nb_buttons = 0, nb_corners = 0, nb_closed = 0;
	cvInitTreeNodeIterator( &iterator, contours, maxLevel );
	while( (contours = (CvSeq*)cvNextTreeNode( &iterator )) != 0 ) {
		nb_contours ++;
		nb_segments+= contours->total;

		//printf("nb cont=%d segments=%d\n", nb_contours, nb_segments);
		CvSeqReader reader;
		cvStartReadSeq( contours, &reader, 0 );
		int val;
		int elem_type = CV_MAT_TYPE(contours->flags);
		CvPoint offset = cvPoint(0,0);
		CvPoint pt0 = cvPoint(0,0);
		CvPoint old_pt = cvPoint(0,0);
		int count  = contours->total;
		int thickness = 1;
	//		CV_READ_SEQ_ELEM( val, reader );

		int scalar_nb = 0;
		double scalar_sum = 0.;

		int nb_closed = 0;
		if( CV_IS_SEQ_POLYLINE( contours ))
		{
			if( thickness >= 0 )
			{
				CvPoint pt1, pt2;
				int shift = 0;

				count -= !CV_IS_SEQ_CLOSED(contours);

				bool is_closed = CV_IS_SEQ_CLOSED(contours);
				if(is_closed) {
					nb_closed ++;
				}

				if( elem_type == CV_32SC2 )
				{
					CV_READ_SEQ_ELEM( pt1, reader );
					pt1.x += offset.x;
					pt1.y += offset.y;

				}
				else
				{
					CvPoint2D32f pt1f;
					CV_READ_SEQ_ELEM( pt1f, reader );
				}

				float contour_length = 0.f;
				float penality_mean = 0.f;
				int xmin = m_cannyImage->width, xmax = 0;
				int ymin = m_cannyImage->height, ymax = 0;

				for( i = 0; i < count; i++ )
				{
					if( elem_type == CV_32SC2 )
					{
						CV_READ_SEQ_ELEM( pt2, reader );
						pt2.x += offset.x;
						pt2.y += offset.y;
					}
					else
					{
						CvPoint2D32f pt2f;
						CV_READ_SEQ_ELEM( pt2f, reader );
						pt2.x = cvRound( pt2f.x /* * XY_ONE */);
						pt2.y = cvRound( pt2f.y /* * XY_ONE */ );
					}
					//icvThickLine( mat, pt1, pt2, clr, thickness, line_type, 2, shift );
				
					static u8 line_r = 37, line_g = 67, line_b = 113;
					line_r += 37;
					line_g += 67;
					line_b += 113;



					if(pt1.x < xmin) xmin = pt1.x;
					if(pt1.x > xmax) xmax = pt1.x;
					if(pt1.y < ymin) ymin = pt1.y;
					if(pt1.y > ymax) ymax = pt1.y;

					if(old_pt.x != 0) {
						CvPoint2D32f vect1 = norm_vect(old_pt, pt1);
						CvPoint2D32f vect2 = norm_vect(pt1, pt2) ;

						float dx = pt1.x - pt2.x;
						float dy = pt1.y - pt2.y;
						float len1_2 = sqrtf(dx*dx + dy*dy);
						float dx2 = pt1.x - old_pt.x;
						float dy2 = pt1.y - old_pt.y;
						float len1_old = sqrtf(dx2*dx2 + dy2*dy2);

						contour_length += len1_2;

						// best is round angled, so scalar shloud be near 1
						float scalar = scalar_vect(vect1, vect2);
						float scalar_penality = 1.f - fabs(scalar_vect (vect1, vect2));



						if(len1_2>4
						   && len1_old > 4
						   && scalar<0.1f
						   && scalar > -0.1f
						   && m_cannyImage
						//   && len1_2>3 && len1_old>3
						   ) { // draw a circle around the button
							nb_corners++;
							if(cnt_img)
								cvRectangle( cnt_img ,
										cvPoint(pt1.x-4, pt1.y-4),
										cvPoint(pt1.x+4, pt1.y+4),
										CV_RGB(line_r, line_g, line_b),
										1);
						}


						penality_mean += scalar_penality;
						scalar_sum += scalar;
						scalar_nb++;

					/*	printf("\tscalar = (%g, %g) . (%g,%g) = %g\n",
							   vect1.x, vect1.y,
							   vect2.x, vect2.y,
							   scalar);*/
					}

					if(cnt_img) {
						cvLine(cnt_img, pt1, pt2,
							   CV_RGB(line_r, line_g, line_b), 1);
					}

					old_pt = pt1;
					pt1 = pt2;
				}

				if(scalar_nb>0) {
					scalar_sum /= scalar_nb;
					scalar_nb = 1;
					penality_mean /= scalar_nb;
				}

				/* count buttons
				   e.g. a button is closed and small

				*/
				if((xmax - xmin)<m_cannyImage->width/20
				   && (ymax - ymin)<m_cannyImage->height/20) {
					float ratio_wh = (xmin != ymin ?
							(float)(ymax - ymin)/(float)(xmax - xmin)
							: 0);

					// small enouch to be a button
					if(cnt_img && fabs(ratio_wh-1.f)<0.2f) { // draw a circle around the button
						cvCircle(cnt_img, cvPoint((xmin+xmax)/2, (ymin+ymax)/2),
								 tmmax((xmax-xmin), (ymax-ymin))/2,
								 CV_RGB(255,0,0), 1);
						cvCircle(cnt_img, cvPoint((xmin+xmax)/2, (ymin+ymax)/2),
								 tmmin((xmax-xmin), (ymax-ymin))/2,
								 CV_RGB(255,127,0), 1);
					}
					nb_buttons++;
				}
				if(is_closed) {
				}
				penality_sum += penality_mean ;

				//printf("\tscalar mean : %g\n", scalar_sum);
				scalar_mean += scalar_sum;
				scalar_mean_nb+=scalar_nb;
			}
		}
	}

	if(scalar_mean_nb>0) {
		scalar_mean /= (double)scalar_mean_nb;
		penality_sum /= (double)scalar_mean_nb;
	}

	// 30 countours are ok, 100 are too much
	m_details.nb_contours = nb_contours;
	m_details.mu_contours = MUNORM( 1.f - (m_details.nb_contours - 30.f) /400.f );

	m_details.nb_segments = nb_segments;
	m_details.mu_segments = MUNORM( 1.f - (m_details.nb_segments - 5000.f) /20000.f );

	m_details.scalar_mean = scalar_mean;
	// let's say 0.2 is the best
	m_details.mu_scalar = MUNORM( 1.f - (scalar_mean - 0.2f )/0.6f);

	m_details.nb_corners = nb_corners;
	m_details.mu_corners = MUNORM( 1.f - (m_details.nb_contours - 30.f) /400.f );
	m_details.nb_buttons = nb_buttons;
	m_details.mu_buttons = MUNORM( 1.f - (m_details.nb_contours - 30.f) /400.f );

	m_waf_info.contour_factor = 0.15f + (
		m_details.mu_buttons + m_details.mu_contours + m_details.mu_corners
		+ m_details.mu_scalar + m_details.mu_segments)/5.f*0.85f;
			;

	if(g_debug_WAFMeter) {
		fprintf(stderr, "\tFINAL : penality=%g "
				"nb_contour=%d=>%g "
				"nb_closed = %d nb_buttons=%d corners=%d "
				"nb_segments=%d scalar mean : %g => factor=%g\n",
			penality_sum,
			nb_contours, m_details.mu_contours,
			nb_closed,nb_buttons,nb_corners,
			nb_segments, scalar_mean,
		   m_waf_info.contour_factor );

		tmSaveImage(TMP_DIRECTORY "CannyContours.pgm", cnt_img);
		cvReleaseImage( &cnt_img );
	}

	cvReleaseMemStorage(&storage);
	return 0;
}

static u8 wafscale_init = 0;
static float wafscale_hue_tab[360];

void setWafHueScale(IplImage * huescaleImg) {
    if(!huescaleImg) { return; }

    wafscale_init  = 1;

    for(int c = 0; c<360; c++)
        wafscale_hue_tab[c] = 0.25f;

    int pitch = huescaleImg->widthStep;
    for(int c = 0; c<tmmin(huescaleImg->width, 360); c++) {
            u8 * pix = (u8 *)huescaleImg->imageData + c*huescaleImg->nChannels + 1;
            int val = 0;
            int r = 0;
            for(r = 0; r<huescaleImg->height && *pix>100; r++, pix+=pitch) {

            }

            // value =
            // if r = 0 => 1.f
            // if r = height => 0.f
            float coef = (float)(huescaleImg->height - r )
                                     / (float)huescaleImg->height;
            wafscale_hue_tab[c] = coef;

            if(g_debug_WAFMeter) {
                    fprintf(stderr, "[wafmeter]::%s:%d : huescale[%d]=%d => %g\n",
                            __func__, __LINE__, c, r, coef);
            }
    }

}

float wafscale_H(int h) {
	if(!wafscale_init) {
		wafscale_init  = 1;

		for(int c = 0; c<360; c++)
			wafscale_hue_tab[c] = 0.25f;

		IplImage * huescaleImg = cvLoadImage("huescale.png");
		if(huescaleImg) {
			int pitch = huescaleImg->widthStep;
			for(int c = 0; c<tmmin(huescaleImg->width, 360); c++) {
				u8 * pix = (u8 *)huescaleImg->imageData + c*huescaleImg->nChannels + 1;
				int val = 0;
				int r = 0;
				for(r = 0; r<huescaleImg->height && *pix>100; r++, pix+=pitch) {

				}
				if(g_debug_WAFMeter) {
					fprintf(stderr, "[wafmeter]::%s:%d : huescale[%d]=%d\n",
						__func__, __LINE__, c, r);
				}

				// value =
				// if r = 0 => 1.f
				// if r = height => 0.f
				float coef = (float)(huescaleImg->height - r )
							 / (float)huescaleImg->height;
				wafscale_hue_tab[c] = coef;
			}

		} else {
			fprintf(stderr, "[wafmeter]::%s:%d : ERROR : cannot load huescale.png\n", __func__, __LINE__);
		}
	}

	return wafscale_hue_tab[h*2];
}

float wafscale_SV(int s, int v) {
	if(s < 100) return 0.f; // not saturation = does not count

	if(v < 32) return 0.f; // to dark to have a real color

	float fs = (float)s;
	float valS = MUNORM( (s - 100.f)/100.f);

	return valS;
}

int WAFMeter::processHSV() {
	if(!m_scaledImage) {
		fprintf(stderr, "WAFMeter::%s:%d : no scaled image\n", __func__, __LINE__);

		return -1;
	}

	bool to_HLS = false;

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
				cvGetSize(m_scaledImage),
				IPL_DEPTH_8U,
				3); //m_scaledImage->nChannels);
	} else {

	}

        IplImage * rgbImage = m_scaledImage;
	if(m_scaledImage->nChannels == 4) {
            fprintf(stderr, "WAFMeter::%s:%d : convert to RGB\n", __func__, __LINE__);

            rgbImage = tmCreateImage(cvGetSize(m_scaledImage), IPL_DEPTH_8U, 3);
            // bad on macOSX:		cvCvtColor(m_scaledImage, bgrImage, CV_BGRA2BGR);
            cvCvtColor(m_scaledImage, rgbImage, CV_BGRA2BGR);
            fprintf(stderr, "WAFMeter::%s:%d : converted to RGB\n", __func__, __LINE__);
        }

	if(to_HLS) {
            if(g_debug_WAFMeter) {
                fprintf(stderr, "WAFMeter::%s:%d : convert RGB %dx%dx%d-> HLS %dx%dx%d\n",
                        __func__, __LINE__,
                        rgbImage->width, rgbImage->height, rgbImage->nChannels,
                        hsvImage->width, hsvImage->height, hsvImage->nChannels
                        );
            }
            if(m_scaledImage->nChannels != 4) {

                cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HLS);
            } else {
                cvCvtColor(rgbImage, hsvImage, CV_BGR2HLS);
            }
	} else {
            if(g_debug_WAFMeter) {
                fprintf(stderr, "WAFMeter::%s:%d : convert RGB %dx%dx%d-> HSV %dx%dx%d\n",
                        __func__, __LINE__,
                        rgbImage->width, rgbImage->height, rgbImage->nChannels,
                        hsvImage->width, hsvImage->height, hsvImage->nChannels
                        );
            }
            if(m_scaledImage->nChannels != 4) {
                cvCvtColor(m_scaledImage, hsvImage, CV_BGR2HSV);
            } else {
                cvCvtColor(rgbImage, hsvImage, CV_BGR2HSV);
            }

            //cvCvtColor(m_scaledImage, hsvImage, CV_BGR2Lab);
	}

        if(rgbImage != m_scaledImage) {
                tmReleaseImage(&rgbImage);
	}

	if(!h_plane) h_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!s_plane) s_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	if(!v_plane) v_plane = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );

	if(!m_grayImage) { // grayscaled
		m_grayImage = tmCreateImage( cvGetSize(hsvImage), IPL_DEPTH_8U, 1 );
	}

        if(m_scaledImage->nChannels != 4) {
            if(g_debug_WAFMeter) {
                fprintf(stderr, "WAFMeter::%s:%d : convert BGR %dx%dx%d-> gray %dx%dx%d\n",
                        __func__, __LINE__,
                        m_scaledImage->width, m_scaledImage->height, m_scaledImage->nChannels,
                        m_grayImage->width, m_grayImage->height, m_grayImage->nChannels
                        );
            }
            cvCvtColor(m_scaledImage, m_grayImage, CV_BGR2GRAY);
        } else {
            if(g_debug_WAFMeter) {
                fprintf(stderr, "WAFMeter::%s:%d : convert BGRA %dx%dx%d-> gray %dx%dx%d\n",
                        __func__, __LINE__,
                        m_scaledImage->width, m_scaledImage->height, m_scaledImage->nChannels,
                        m_grayImage->width, m_grayImage->height, m_grayImage->nChannels
                        );
            }
            cvCvtColor(m_scaledImage, m_grayImage, CV_BGRA2GRAY);
        }

	// HSV
	if(!to_HLS) {
		cvCvtPixToPlane( hsvImage, h_plane, s_plane, v_plane, 0 );
		// Lab
//		cvCvtPixToPlane( hsvImage, s_plane, h_plane, v_plane, 0 );

	// HLS
	} else
		cvCvtPixToPlane( hsvImage, h_plane, v_plane, s_plane, 0 );

	IplImage * colorWAFImage = NULL;
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
            } else {
                cvZero(m_HSHistoImage);
            }
            colorWAFImage = tmCreateImage(cvSize(m_scaledImage->width, m_scaledImage->height), IPL_DEPTH_8U, 1);
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


			float waf = //(double)v / 255. *
					( wafscale_H(h) *
						wafscale_SV(s,v)

						);

			if(//s > 64 && // only the saturated tones
1 //				v > 64 // and not dark enough to be a noise color artefact
			) {
				color_factor += waf;
				// FIXME : add WAF colors coef
				color_factor_nb++;
			}

			if(g_debug_WAFMeter) {
				u8 * cWAF = (u8 *)(colorWAFImage->imageData+r*colorWAFImage->widthStep)
							+ c;
				*cWAF = (u8)(waf * 255.f);
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
		if(m_waf_info.color_factor > 1.f)
			m_waf_info.color_factor = 1.f;
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
                if(g_debug_WAFMeter) {
                    fprintf(stderr, "WAFMeter::%s:%d : convert HSV2BGR %dx%dx%d-> gray %dx%dx%d\n",
                            __func__, __LINE__,
                            hsvOutImage->width, hsvOutImage->height, hsvOutImage->nChannels,
                            m_ColorHistoImage->width, m_ColorHistoImage->height, m_ColorHistoImage->nChannels
                            );
                }
		cvCvtColor(hsvOutImage, m_ColorHistoImage, CV_HSV2BGR);

		tmSaveImage(TMP_DIRECTORY "colorWaf.pgm", colorWAFImage);
		if(g_debug_WAFMeter) {
			tmSaveImage(TMP_DIRECTORY "HSHistoHSV.ppm", hsvOutImage);
		}
		tmReleaseImage(&hsvOutImage);
		tmReleaseImage(&colorWAFImage);
	}
	return 0;
}
