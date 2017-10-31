#include "fastguidedfilter.h"

////static 表示这个函数只能在当前文件使用
//static cv::Mat boxfilter(const cv::Mat &I, int r)
//{
//    cv::Mat result;
//    cv::blur(I, result, cv::Size(2*r+1, 2*r+1));
//    return result;
//}
//
//static cv::Mat convertTo(const cv::Mat &mat, int depth)
//{
//    if (mat.depth() == depth)
//        return mat;
//
//    cv::Mat result;
//    mat.convertTo(result, depth);
//
//    return result;
//}
//
////size: cols:rows
//static cv::Mat resize(const cv::Mat &mat, cv::Size size)
//{
//    cv::Mat result;
//    cv::resize(mat,result,size);
//
//    return result;
//}
//
//class FastGuidedFilterImpl
//{
//public:
//    virtual ~FastGuidedFilterImpl() = default;
//
//    cv::Mat filter(const cv::Mat &p, int depth);
//
//protected:
//    int Idepth;
//
//private:
//    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const = 0;
//};
//
////Gray guided image
//class FastGuidedFilterMono : public FastGuidedFilterImpl
//{
//public:
//    FastGuidedFilterMono(const cv::Mat &I, int r, int scale, double eps);
//
//private:
//    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;
//
//private:
//    int r;
//    double eps;
//    cv::Mat I, mean_I, var_I;
//    cv::Mat origI;
//    int scale;
//};
//
//class FastGuidedFilterColor : public FastGuidedFilterImpl
//{
//public:
//    FastGuidedFilterColor(const cv::Mat &I, int r, int scale, double eps);
//
//private:
//    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;
//
//private:
//    std::vector<cv::Mat> origIchannels;
//    std::vector<cv::Mat> Ichannels;
//    int r;
//    double eps;
//    cv::Mat mean_I_r, mean_I_g, mean_I_b;
//    cv::Mat invrr, invrg, invrb, invgg, invgb, invbb;
//    int scale;
//};
//
//
//cv::Mat FastGuidedFilterImpl::filter(const cv::Mat &p, int depth)
//{
//    cv::Mat p2 = convertTo(p, Idepth);
//
//    cv::Mat result;
//    if (p.channels() == 1)
//    {
//        result = filterSingleChannel(p2);
//    }
//    else
//    {
//        std::vector<cv::Mat> pc;
//        cv::split(p2, pc);
//
//        for (std::size_t i = 0; i < pc.size(); ++i){
//            pc[i] = filterSingleChannel(pc[i]);
//        }
//
//        cv::merge(pc, result);
//    }
//
//    return convertTo(result, depth == -1 ? p.depth() : depth);
//}
//
//FastGuidedFilterMono::FastGuidedFilterMono(const cv::Mat &origI, int r, int scale, double eps) : r(r/scale), scale(scale),eps(eps)
//{
//
//    if (origI.depth() == CV_32F || origI.depth() == CV_64F)
//        this->origI = origI.clone();
//    else
//        this->origI = convertTo(origI, CV_32F);
//
//    Idepth = this->origI.depth();
//
//    I = resize(this->origI,cv::Size(this->origI.cols/scale,this->origI.rows/scale));
//
//    mean_I = boxfilter(I, r);
//    cv::Mat mean_II = boxfilter(I.mul(I), r);
//    var_I = mean_II - mean_I.mul(mean_I);
//}
//
//cv::Mat FastGuidedFilterMono::filterSingleChannel(const cv::Mat &origP) const
//{
//    cv::Mat p = resize(origP,cv::Size(origP.cols/scale,origP.rows/scale));
//    cv::Mat mean_p = boxfilter(p, r);
//    cv::Mat mean_Ip = boxfilter(I.mul(p), r);
//    cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p); // this is the covariance of (I, p) in each local patch.
//
//    cv::Mat a = cov_Ip / (var_I + eps); // Eqn. (5) in the paper;
//    cv::Mat b = mean_p - a.mul(mean_I); // Eqn. (6) in the paper;
//
//    cv::Mat mean_a = boxfilter(a, r);
//    cv::Mat mean_b = boxfilter(b, r);
//
//    mean_a = resize(mean_a,cv::Size(origI.cols,origI.rows));
//    mean_b = resize(mean_b,cv::Size(origI.cols,origI.rows));
//
//    return mean_a.mul(origI) + mean_b;
//}
//
//FastGuidedFilterColor::FastGuidedFilterColor(const cv::Mat &origI, int r, int scale, double eps) : r(r/scale), scale(scale), eps(eps)
//{
//    cv::Mat I;
//    if (origI.depth() == CV_32F || origI.depth() == CV_64F)
//        I = origI.clone();
//    else
//        I = convertTo(origI, CV_32F);
//
//    Idepth = I.depth();
//
//    cv::split(I, origIchannels);
//
//    I = resize(I,cv::Size(I.cols/scale,I.rows/scale));
//    cv::split(I, Ichannels);
//
//    mean_I_r = boxfilter(Ichannels[0], r);
//    mean_I_g = boxfilter(Ichannels[1], r);
//    mean_I_b = boxfilter(Ichannels[2], r);
//
//    // variance of I in each local patch: the matrix Sigma in Eqn (14).
//    // Note the variance in each local patch is a 3x3 symmetric matrix:
//    //           rr, rg, rb
//    //   Sigma = rg, gg, gb
//    //           rb, gb, bb
//    cv::Mat var_I_rr = boxfilter(Ichannels[0].mul(Ichannels[0]), r) - mean_I_r.mul(mean_I_r) + eps;
//    cv::Mat var_I_rg = boxfilter(Ichannels[0].mul(Ichannels[1]), r) - mean_I_r.mul(mean_I_g);
//    cv::Mat var_I_rb = boxfilter(Ichannels[0].mul(Ichannels[2]), r) - mean_I_r.mul(mean_I_b);
//    cv::Mat var_I_gg = boxfilter(Ichannels[1].mul(Ichannels[1]), r) - mean_I_g.mul(mean_I_g) + eps;
//    cv::Mat var_I_gb = boxfilter(Ichannels[1].mul(Ichannels[2]), r) - mean_I_g.mul(mean_I_b);
//    cv::Mat var_I_bb = boxfilter(Ichannels[2].mul(Ichannels[2]), r) - mean_I_b.mul(mean_I_b) + eps;
//
//    // Inverse of Sigma + eps * I
//    invrr = var_I_gg.mul(var_I_bb) - var_I_gb.mul(var_I_gb);
//    invrg = var_I_gb.mul(var_I_rb) - var_I_rg.mul(var_I_bb);
//    invrb = var_I_rg.mul(var_I_gb) - var_I_gg.mul(var_I_rb);
//    invgg = var_I_rr.mul(var_I_bb) - var_I_rb.mul(var_I_rb);
//    invgb = var_I_rb.mul(var_I_rg) - var_I_rr.mul(var_I_gb);
//    invbb = var_I_rr.mul(var_I_gg) - var_I_rg.mul(var_I_rg);
//
//    cv::Mat covDet = invrr.mul(var_I_rr) + invrg.mul(var_I_rg) + invrb.mul(var_I_rb);
//
//    invrr /= covDet;
//    invrg /= covDet;
//    invrb /= covDet;
//    invgg /= covDet;
//    invgb /= covDet;
//    invbb /= covDet;
//}
//
//cv::Mat FastGuidedFilterColor::filterSingleChannel(const cv::Mat &origP) const
//{
//    cv::Mat p = resize(origP,cv::Size(origP.cols/scale,origP.rows/scale));
//
//    cv::Mat mean_p = boxfilter(p, r);
//
//    cv::Mat mean_Ip_r = boxfilter(Ichannels[0].mul(p), r);
//    cv::Mat mean_Ip_g = boxfilter(Ichannels[1].mul(p), r);
//    cv::Mat mean_Ip_b = boxfilter(Ichannels[2].mul(p), r);
//
//    // covariance of (I, p) in each local patch.
//    cv::Mat cov_Ip_r = mean_Ip_r - mean_I_r.mul(mean_p);
//    cv::Mat cov_Ip_g = mean_Ip_g - mean_I_g.mul(mean_p);
//    cv::Mat cov_Ip_b = mean_Ip_b - mean_I_b.mul(mean_p);
//
//    cv::Mat a_r = invrr.mul(cov_Ip_r) + invrg.mul(cov_Ip_g) + invrb.mul(cov_Ip_b);
//    cv::Mat a_g = invrg.mul(cov_Ip_r) + invgg.mul(cov_Ip_g) + invgb.mul(cov_Ip_b);
//    cv::Mat a_b = invrb.mul(cov_Ip_r) + invgb.mul(cov_Ip_g) + invbb.mul(cov_Ip_b);
//
//    cv::Mat b = mean_p - a_r.mul(mean_I_r) - a_g.mul(mean_I_g) - a_b.mul(mean_I_b); // Eqn. (15) in the paper;
//
//    cv::Mat temp = (boxfilter(a_r, r).mul(Ichannels[0])
//                 + boxfilter(a_g, r).mul(Ichannels[1])
//                 + boxfilter(a_b, r).mul(Ichannels[2])
//                 + boxfilter(b, r));
//    cv::imshow("temp",temp);
//
//    a_r = resize(a_r,cv::Size(origIchannels[0].cols,origIchannels[0].rows));
//    a_g = resize(a_g,cv::Size(origIchannels[1].cols,origIchannels[1].rows));
//    a_b = resize(a_b,cv::Size(origIchannels[2].cols,origIchannels[2].rows));
//
//    b = resize(b,cv::Size(origIchannels[0].cols,origIchannels[0].rows));
//
//
//    return (boxfilter(a_r, r*scale).mul(origIchannels[0])
//            + boxfilter(a_g, r*scale).mul(origIchannels[1])
//            + boxfilter(a_b, r*scale).mul(origIchannels[2])
//            + boxfilter(b, r*scale));  // Eqn. (16) in the paper;
//}
//
//
//FastGuidedFilter::FastGuidedFilter(const cv::Mat &I, int r, int scale, double eps)
//{
//    CV_Assert(I.channels() == 1 || I.channels() == 3);
//
//    if (I.channels() == 1)
//        impl_ = new FastGuidedFilterMono(I, r, scale, eps);
//    else
//        impl_ = new FastGuidedFilterColor(I, r, scale, eps);
//}
//
//FastGuidedFilter::~FastGuidedFilter()
//{
//    delete impl_;
//}
//
//cv::Mat FastGuidedFilter::filter(const cv::Mat &p, int depth) const
//{
//    return impl_->filter(p, depth);
//}
//
//cv::Mat fastGuidedFilter(const cv::Mat &I, const cv::Mat &p, int r, int scale, double eps, int depth)
//{
//    return FastGuidedFilter(I, r, scale, eps).filter(p, depth);
//}



//static 表示这个函数只能在当前文件使用
static cv::Mat boxfilter(const cv::Mat &I, int r)
{
    cv::Mat result;
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
    FastGuidedFilterMono(const cv::Mat &I, int r, double eps);

private:
    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;

private:
    int r;
    double eps;
    cv::Mat I, mean_I, var_I;
};

class FastGuidedFilterColor : public FastGuidedFilterImpl
{
public:
    FastGuidedFilterColor(const cv::Mat &I, int r, double eps);

private:
    virtual cv::Mat filterSingleChannel(const cv::Mat &p) const;

private:
    std::vector<cv::Mat> Ichannels;
    int r;
    double eps;
    cv::Mat mean_I_r, mean_I_g, mean_I_b;
    cv::Mat invrr, invrg, invrb, invgg, invgb, invbb;
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

FastGuidedFilterMono::FastGuidedFilterMono(const cv::Mat &origI, int r, double eps) : r(r), eps(eps)
{

    if (origI.depth() == CV_32F || origI.depth() == CV_64F)
        I = origI.clone();
    else
        I = convertTo(origI, CV_32F);

    Idepth = I.depth();

    mean_I = boxfilter(I, r);
    cv::Mat mean_II = boxfilter(I.mul(I), r);
    var_I = mean_II - mean_I.mul(mean_I);
}

cv::Mat FastGuidedFilterMono::filterSingleChannel(const cv::Mat &p) const
{
    cv::Mat mean_p = boxfilter(p, r);
    cv::Mat mean_Ip = boxfilter(I.mul(p), r);
    cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p); // this is the covariance of (I, p) in each local patch.

    cv::Mat a = cov_Ip / (var_I + eps); // Eqn. (5) in the paper;
    cv::Mat b = mean_p - a.mul(mean_I); // Eqn. (6) in the paper;

    cv::Mat mean_a = boxfilter(a, r);
    cv::Mat mean_b = boxfilter(b, r);

    return mean_a.mul(I) + mean_b;
}

FastGuidedFilterColor::FastGuidedFilterColor(const cv::Mat &origI, int r, double eps) : r(r), eps(eps)
{
    cv::Mat I;
    if (origI.depth() == CV_32F || origI.depth() == CV_64F)
        I = origI.clone();
    else
        I = convertTo(origI, CV_32F);

    Idepth = I.depth();

    cv::split(I, Ichannels);

    mean_I_r = boxfilter(Ichannels[0], r);
    mean_I_g = boxfilter(Ichannels[1], r);
    mean_I_b = boxfilter(Ichannels[2], r);

    // variance of I in each local patch: the matrix Sigma in Eqn (14).
    // Note the variance in each local patch is a 3x3 symmetric matrix:
    //           rr, rg, rb
    //   Sigma = rg, gg, gb
    //           rb, gb, bb
    cv::Mat var_I_rr = boxfilter(Ichannels[0].mul(Ichannels[0]), r) - mean_I_r.mul(mean_I_r) + eps;
    cv::Mat var_I_rg = boxfilter(Ichannels[0].mul(Ichannels[1]), r) - mean_I_r.mul(mean_I_g);
    cv::Mat var_I_rb = boxfilter(Ichannels[0].mul(Ichannels[2]), r) - mean_I_r.mul(mean_I_b);
    cv::Mat var_I_gg = boxfilter(Ichannels[1].mul(Ichannels[1]), r) - mean_I_g.mul(mean_I_g) + eps;
    cv::Mat var_I_gb = boxfilter(Ichannels[1].mul(Ichannels[2]), r) - mean_I_g.mul(mean_I_b);
    cv::Mat var_I_bb = boxfilter(Ichannels[2].mul(Ichannels[2]), r) - mean_I_b.mul(mean_I_b) + eps;

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
    cv::Mat mean_p = boxfilter(p, r);

    cv::Mat mean_Ip_r = boxfilter(Ichannels[0].mul(p), r);
    cv::Mat mean_Ip_g = boxfilter(Ichannels[1].mul(p), r);
    cv::Mat mean_Ip_b = boxfilter(Ichannels[2].mul(p), r);

    // covariance of (I, p) in each local patch.
    cv::Mat cov_Ip_r = mean_Ip_r - mean_I_r.mul(mean_p);
    cv::Mat cov_Ip_g = mean_Ip_g - mean_I_g.mul(mean_p);
    cv::Mat cov_Ip_b = mean_Ip_b - mean_I_b.mul(mean_p);

    cv::Mat a_r = invrr.mul(cov_Ip_r) + invrg.mul(cov_Ip_g) + invrb.mul(cov_Ip_b);
    cv::Mat a_g = invrg.mul(cov_Ip_r) + invgg.mul(cov_Ip_g) + invgb.mul(cov_Ip_b);
    cv::Mat a_b = invrb.mul(cov_Ip_r) + invgb.mul(cov_Ip_g) + invbb.mul(cov_Ip_b);

    cv::Mat b = mean_p - a_r.mul(mean_I_r) - a_g.mul(mean_I_g) - a_b.mul(mean_I_b); // Eqn. (15) in the paper;

    return (boxfilter(a_r, r).mul(Ichannels[0])
            + boxfilter(a_g, r).mul(Ichannels[1])
            + boxfilter(a_b, r).mul(Ichannels[2])
            + boxfilter(b, r));  // Eqn. (16) in the paper;
}


FastGuidedFilter::FastGuidedFilter(const cv::Mat &I, int r, double eps)
{
    CV_Assert(I.channels() == 1 || I.channels() == 3);

    if (I.channels() == 1)
        impl_ = new FastGuidedFilterMono(I, r, eps);
    else
        impl_ = new FastGuidedFilterColor(I,r, eps);
}

FastGuidedFilter::~FastGuidedFilter()
{
    delete impl_;
}

cv::Mat FastGuidedFilter::filter(const cv::Mat &p, int depth) const
{
    return impl_->filter(p, depth);
}

cv::Mat fastGuidedFilter(const cv::Mat &I, const cv::Mat &p, int r, double eps, int depth)
{
    cv::Mat tempI;
    cv::Mat tempP;
    cv::resize(I,tempI,cv::Size(I.cols/4,I.rows/4));
    cv::resize(p,tempP,cv::Size(p.cols/4,p.rows/4));
    cv::Mat t = FastGuidedFilter(tempI, r/4, eps).filter(tempP, depth);
    cv::Mat result;
    cv::resize(t,result,cv::Size(I.cols,I.rows));
    return result;
}
