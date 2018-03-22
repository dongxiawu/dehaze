#include "fastguidedfilter.h"

//static 表示这个函数只能在当前文件使用
static cv::Mat boxfilter(const cv::Mat &I, int r)
{
    cv::Mat result;
    //均值滤波
    cv::blur(I, result, cv::Size(2*r+1, 2*r+1));
    return result;
}

static cv::Mat convertTo(const cv::Mat &mat, int depth)
{
    if (mat.depth() == depth)
        return mat;

    cv::Mat result;
    mat.convertTo(result, depth);

    return result;
}

class FastGuidedFilterImpl
{
public:
    virtual ~FastGuidedFilterImpl() = default;

    cv::Mat filter(const cv::Mat &p, int depth);

protected:
    int Idepth;

private:
    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const = 0;
};

//Gray guided image
class FastGuidedFilterMono : public FastGuidedFilterImpl
{
public:
    FastGuidedFilterMono(const cv::Mat &I, int r, int scaleSize, double eps);

private:
    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;

private:
    int r;
    double eps;
    cv::Mat I, mean_I, var_I;
    cv::Mat I_temp;
    int scaleSize;
    int r_temp;
};

class FastGuidedFilterColor : public FastGuidedFilterImpl
{
public:
    FastGuidedFilterColor(const cv::Mat &I, int r, int scaleSize, double eps);

private:
    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;

private:
    std::vector<cv::Mat> Ichannels;
    int r;
    double eps;
    cv::Mat mean_I_r, mean_I_g, mean_I_b;
    cv::Mat invrr, invrg, invrb, invgg, invgb, invbb;
    std::vector<cv::Mat> tempIchannels;
    int scaleSize;
    int r_temp;
};


cv::Mat FastGuidedFilterImpl::filter(const cv::Mat &p, int depth)
{
    cv::Mat p2 = convertTo(p, Idepth);

    cv::Mat result;
    if (p.channels() == 1)
    {
        result = filterSingleChannel(p2);
    }
    else
    {
        std::vector<cv::Mat> pc;
        cv::split(p2, pc);

        for (std::size_t i = 0; i < pc.size(); ++i){
            pc[i] = filterSingleChannel(pc[i]);
        }

        cv::merge(pc, result);
    }

    return convertTo(result, depth == -1 ? p.depth() : depth);
}

FastGuidedFilterMono::FastGuidedFilterMono(const cv::Mat &origI, int r, int scaleSize, double eps) : r(r), eps(eps), scaleSize(scaleSize)
{

    if (origI.depth() == CV_32F || origI.depth() == CV_64F)
        I = origI.clone();
    else
        I = convertTo(origI, CV_32F);

    Idepth = I.depth();

    cv::resize(I,I_temp,cv::Size(I.cols/scaleSize,I.rows/scaleSize));
    r_temp = r/scaleSize;

    mean_I = boxfilter(I_temp, r_temp);
    cv::Mat mean_II = boxfilter(I_temp.mul(I_temp), r_temp);
    var_I = mean_II - mean_I.mul(mean_I);
}

cv::Mat FastGuidedFilterMono::filterSingleChannel(const cv::Mat &p) const
{
    cv::Mat p_temp;

    cv::resize(p,p_temp,cv::Size(p.cols/scaleSize,p.rows/scaleSize));

    cv::Mat mean_p = boxfilter(p_temp, r_temp);
    cv::Mat mean_Ip = boxfilter(I_temp.mul(p_temp), r_temp);
    cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p); // this is the covariance of (I, p) in each local patch.

    cv::Mat a = cov_Ip / (var_I + eps); // Eqn. (5) in the paper;
    cv::Mat b = mean_p - a.mul(mean_I); // Eqn. (6) in the paper;

    cv::Mat mean_a = boxfilter(a, r_temp);
    cv::Mat mean_b = boxfilter(b, r_temp);

    cv::resize(mean_a,mean_a,cv::Size(I.cols,I.rows));
    cv::resize(mean_b,mean_b,cv::Size(I.cols,I.rows));

    return mean_a.mul(I) + mean_b;
}

FastGuidedFilterColor::FastGuidedFilterColor(const cv::Mat &origI, int r, int scaleSize, double eps) : r(r), eps(eps), scaleSize(scaleSize)
{
    cv::Mat I;
    if (origI.depth() == CV_32F || origI.depth() == CV_64F){
        I = origI.clone();
    } else {
        I = convertTo(origI, CV_32F);
    }

    Idepth = I.depth();

    cv::split(I, Ichannels);

    cv::Mat I_temp;
    cv::resize(I,I_temp,cv::Size(I.cols/scaleSize,I.rows/scaleSize));
    cv::split(I_temp,tempIchannels);
    r_temp = r/scaleSize;

    mean_I_r = boxfilter(tempIchannels[0], r_temp);
    mean_I_g = boxfilter(tempIchannels[1], r_temp);
    mean_I_b = boxfilter(tempIchannels[2], r_temp);

    // variance of I in each local patch: the matrix Sigma in Eqn (14).
    // Note the variance in each local patch is a 3x3 symmetric matrix:
    //           rr, rg, rb
    //   Sigma = rg, gg, gb
    //           rb, gb, bb
    cv::Mat var_I_rr = boxfilter(tempIchannels[0].mul(tempIchannels[0]), r_temp) - mean_I_r.mul(mean_I_r) + eps;
    cv::Mat var_I_rg = boxfilter(tempIchannels[0].mul(tempIchannels[1]), r_temp) - mean_I_r.mul(mean_I_g);
    cv::Mat var_I_rb = boxfilter(tempIchannels[0].mul(tempIchannels[2]), r_temp) - mean_I_r.mul(mean_I_b);
    cv::Mat var_I_gg = boxfilter(tempIchannels[1].mul(tempIchannels[1]), r_temp) - mean_I_g.mul(mean_I_g) + eps;
    cv::Mat var_I_gb = boxfilter(tempIchannels[1].mul(tempIchannels[2]), r_temp) - mean_I_g.mul(mean_I_b);
    cv::Mat var_I_bb = boxfilter(tempIchannels[2].mul(tempIchannels[2]), r_temp) - mean_I_b.mul(mean_I_b) + eps;

    // Inverse of Sigma + eps * I
    invrr = var_I_gg.mul(var_I_bb) - var_I_gb.mul(var_I_gb);
    invrg = var_I_gb.mul(var_I_rb) - var_I_rg.mul(var_I_bb);
    invrb = var_I_rg.mul(var_I_gb) - var_I_gg.mul(var_I_rb);
    invgg = var_I_rr.mul(var_I_bb) - var_I_rb.mul(var_I_rb);
    invgb = var_I_rb.mul(var_I_rg) - var_I_rr.mul(var_I_gb);
    invbb = var_I_rr.mul(var_I_gg) - var_I_rg.mul(var_I_rg);

    cv::Mat covDet = invrr.mul(var_I_rr) + invrg.mul(var_I_rg) + invrb.mul(var_I_rb);

    invrr /= covDet;
    invrg /= covDet;
    invrb /= covDet;
    invgg /= covDet;
    invgb /= covDet;
    invbb /= covDet;
}

cv::Mat FastGuidedFilterColor::filterSingleChannel(const cv::Mat &p) const
{

    cv::Mat temp_p;
    cv::resize(p,temp_p,cv::Size(p.cols/scaleSize,p.rows/scaleSize));

    cv::Mat mean_p = boxfilter(temp_p, r_temp);

    cv::Mat mean_Ip_r = boxfilter(tempIchannels[0].mul(temp_p), r_temp);
    cv::Mat mean_Ip_g = boxfilter(tempIchannels[1].mul(temp_p), r_temp);
    cv::Mat mean_Ip_b = boxfilter(tempIchannels[2].mul(temp_p), r_temp);

    // covariance of (I, p) in each local patch.
    cv::Mat cov_Ip_r = mean_Ip_r - mean_I_r.mul(mean_p);
    cv::Mat cov_Ip_g = mean_Ip_g - mean_I_g.mul(mean_p);
    cv::Mat cov_Ip_b = mean_Ip_b - mean_I_b.mul(mean_p);

    cv::Mat a_r = invrr.mul(cov_Ip_r) + invrg.mul(cov_Ip_g) + invrb.mul(cov_Ip_b);
    cv::Mat a_g = invrg.mul(cov_Ip_r) + invgg.mul(cov_Ip_g) + invgb.mul(cov_Ip_b);
    cv::Mat a_b = invrb.mul(cov_Ip_r) + invgb.mul(cov_Ip_g) + invbb.mul(cov_Ip_b);

    cv::Mat b = mean_p - a_r.mul(mean_I_r) - a_g.mul(mean_I_g) - a_b.mul(mean_I_b); // Eqn. (15) in the paper;

    cv::resize(a_r,a_r,cv::Size(Ichannels[0].cols,Ichannels[0].rows));
    cv::resize(a_g,a_g,cv::Size(Ichannels[1].cols,Ichannels[1].rows));
    cv::resize(a_b,a_b,cv::Size(Ichannels[2].cols,Ichannels[2].rows));

    cv::resize(b,b,cv::Size(Ichannels[2].cols,Ichannels[2].rows));

    return (boxfilter(a_r, r).mul(Ichannels[0])
            + boxfilter(a_g, r).mul(Ichannels[1])
            + boxfilter(a_b, r).mul(Ichannels[2])
            + boxfilter(b, r));  // Eqn. (16) in the paper;
}


FastGuidedFilter::FastGuidedFilter(const cv::Mat &I, int r, int scaleSize, double eps)
{
    CV_Assert(I.channels() == 1 || I.channels() == 3);

    if (I.channels() == 1)
        impl_ = new FastGuidedFilterMono(I, r, scaleSize, eps);
    else
        impl_ = new FastGuidedFilterColor(I,r, scaleSize, eps);
}

FastGuidedFilter::~FastGuidedFilter()
{
    delete impl_;
}

cv::Mat FastGuidedFilter::filter(const cv::Mat &p, int depth) const
{
    return impl_->filter(p, depth);
}

cv::Mat fastGuidedFilter(const cv::Mat &I, const cv::Mat &p, int r, int scaleSize, double eps, int depth)
{
    return FastGuidedFilter(I, r, scaleSize, eps).filter(p, depth);
}
