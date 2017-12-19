//
// Created by dongxia on 17-10-30.
//

#ifndef DEHAZE_FASTGUIDEDFILTER_H
#define DEHAZE_FASTGUIDEDFILTER_H

#include <opencv2/opencv.hpp>

class FastGuidedFilterImpl;

class FastGuidedFilter
{
public:
    FastGuidedFilter(const cv::Mat &I, int r, int scaleSize, double eps);//构造函数不能是虚函数
    virtual ~FastGuidedFilter();

    cv::Mat filter(const cv::Mat &p, int depth = -1) const;
private:
    FastGuidedFilterImpl *impl_;
};

/*
 * @param I: guidance image(should be a gray - scale / single channel image or color image)
 * @param p: filtering input image(should be a gray - scale / single channel image)
 * @param r: local window radius
 * @param scale: scale size (recommend value is 4, must larger than 1 and small than r)
 * @param eps: regularization parameter
 * @param depth: the output mat's depth, default means same as the p's depth
 *
 * @return cv::Mat: guided filter mat
*/
cv::Mat fastGuidedFilter(const cv::Mat &I, const cv::Mat &p, int r, int scaleSize, double eps, int depth=-1);

#endif //DEHAZE_FASTGUIDEDFILTER_H
