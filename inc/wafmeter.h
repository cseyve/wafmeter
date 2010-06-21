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

#ifndef WAFMETER_H
#define WAFMETER_H

#include <cv.h>
#include <cv.hpp>

/** @brief WAF indicators (summary) */
typedef struct {
	float waf_factor;		/*! Resulting WAF */
	float color_factor;
	float contour_factor;
} t_waf_info;

/** @brief WAF indicators (details) */
typedef struct {
	// Color analysis
	float mu_color;

	// Shape analysis
	int nb_corners;
	float mu_corners;
	int nb_contours;
	float mu_contours;
	int nb_segments;
	float mu_segments;
	int nb_buttons;
	float mu_buttons;
	float scalar_mean;
	float mu_scalar;


} t_waf_info_details;


/** @brief WAF factor measuring by image processing


  @author Christophe Seyve cseyve@free.fr
*/
class WAFMeter
{
public:
    WAFMeter();
	~WAFMeter();

	/** @brief get the image WAF info (summary) */
	t_waf_info getWAF() { return m_waf_info; };


	/** @brief get the image WAF info (details) */
	t_waf_info_details getWAFDetails() { return m_details; };

	/** @brief set the unscaled input image */
	int setUnscaledImage(IplImage * img);

	/** @brief set the unscaled input image and process it
		The image is scaled before processing
		*/
	int setScaledImage(IplImage * img);

	/** @brief Directly process one image (must be scaled enough before) */
	int processImage(IplImage * img);

private:
	/** @brief Resulting WAF factors */
	t_waf_info m_waf_info;

	/** @brief Details of WAF factors */
	t_waf_info_details m_details;


	void init();

	void purge();

	/** @brief Process HSV analysis */
	int processHSV();

	// internal images
	/** @brief Original image */
	IplImage * m_originalImage;

	/** @brief Process iteration counter */
	int m_process_counter;

	/** @brief Histogram */
	IplImage * m_HistoImage;

	/** @brief Scaled version of original image */
	IplImage * m_scaledImage;
	/** @brief Flag to tell if scaled image is allocated or not */
	bool m_scaledImage_allocated;

	/** @brief Scaled & grayscaled version of original image */
	IplImage * m_grayImage;

	/** @brief Scaled & HSV version of original image */
	IplImage * hsvImage;
	/** @brief Scaled & HSV version of original image, H plane */
	IplImage * h_plane;
	/** @brief Scaled & HSV version of original image, S plane */
	IplImage * s_plane;
	/** @brief Scaled & HSV/HLS version of original image, V/L plane */
	IplImage * v_plane;

	/** @brief Canny of scaled & grayscaled image */
	IplImage * m_cannyImage;


	/** @brief Canny edge detection */
	int processCanny();

	/** @brief Contour/shape analysis */
	int processContour();

#define H_MAX	180
#define S_MAX	255
	IplImage * m_HSHistoImage;
	IplImage * m_ColorHistoImage;


	/** @brief purge scaled images */
	void purgeScaled();

};





// UTILITIES



#endif // WAFMETER_H
