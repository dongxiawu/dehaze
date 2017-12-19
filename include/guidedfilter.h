#ifndef GUIDED_FILTER_H
#define GUIDED_FILTER_H

#include <opencv2/opencv.hpp>

class GuidedFilterImpl;

class GuidedFilter
{
public:
	GuidedFilter(const cv::Mat &I, int r, double eps);//构造函数不能是虚函数
	virtual ~GuidedFilter();

	cv::Mat filter(const cv::Mat &p, int depth = -1) const;
private:
	GuidedFilterImpl *impl_;
};

/*
 * @param I: guidance image(should be a gray - scale / single channel image or color image)
 * @param p: filtering input image(should be a gray - scale / single channel image)
 * @param r: local window radius
 * @param eps: regularization parameter
 * @param depth: the output mat's depth, default means same as the p's depth
 *
 * @return cv::Mat: guided filter mat
*/
cv::Mat guidedFilter(const cv::Mat &I, const cv::Mat &p, int r, double eps, int depth = -1);

#endif
