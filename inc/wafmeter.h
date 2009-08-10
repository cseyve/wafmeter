#ifndef WAFMETER_H
#define WAFMETER_H

#include <cv.h>
#include <cv.hpp>

/** @brief WAF indicators */
typedef struct {
	float waf_factor;		/*! Resulting WAF */
	float color_factor;
	float contour_factor;
} t_waf_info;

class WAFMeter
{
public:
    WAFMeter();
	~WAFMeter();

	/** @brief get the image waf */
	t_waf_info getWAF() { return m_waf_info; };
	/** @brief set the input image */
	int setImage(IplImage * img);

private:
	t_waf_info m_waf_info;
	void init();
	void purge();

	/** @brief Process HSV analysis */
	int processHSV();

	// internal images
	/** @brief Original image */
	IplImage * m_originalImage;

	/** @brief Histogram */
	IplImage * m_HistoImage;

	/** @brief Scaled version of original image */
	IplImage * m_scaledImage;
	/** @brief Scaled & grayscaled version of original image */
	IplImage * m_grayImage;

	/** @brief Scaled & HSV version of original image */
	IplImage * hsvImage;
	/** @brief Scaled & HSV version of original image, H plane */
	IplImage * h_plane;
	/** @brief Scaled & HSV version of original image, S plane */
	IplImage * s_plane;

	/** @brief Canny of scaled & grayscaled image */
	IplImage * m_cannyImage;

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
