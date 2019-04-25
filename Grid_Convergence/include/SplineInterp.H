#ifndef SPLINEINTERP_H
#define SPLINEINTERP_H

#include<Eigen/Dense>
#include<unsupported/Eigen/Splines>
using Eigen::VectorXd;

// cubic spline interpolation. uses lower degree if fewer points available
class SplineInterp
{
  public:
    // create interpolant for func sampled at xpts with values ypts
    SplineInterp(const VectorXd& xpts, const VectorXd& ypts);
    ~SplineInterp(){};
    // evaluate interpolant at x 
    double operator()(double x) const;
  
  private:
    double x_min;
    double x_max;
    // Spline of one-dimensional "points."
    Eigen::Spline<double, 1> spline;
    // linear scaling of X values down to [0, 1]
    double scaled_value(double x) const;
    // unary application of scaled_value to x_vec
    Eigen::RowVectorXd scaled_values(VectorXd const &x_vec) const;

};   

#endif 
