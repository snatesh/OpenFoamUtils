#include<SplineInterp.H>

SplineInterp::SplineInterp(const VectorXd& xpts, const VectorXd& ypts)
    : x_min(xpts.minCoeff()), x_max(xpts.maxCoeff()),
      spline(Eigen::SplineFitting<Eigen::Spline<double,1>>::Interpolate(
        ypts.transpose(),std::min<int>(xpts.rows()-1,3),scaled_values(xpts)))
    {};

double SplineInterp::operator()(double x) const
{
  return spline(scaled_value(x))(0);
}
    
double SplineInterp::scaled_value(double x) const 
{
  return (x - x_min) / (x_max - x_min);
}

Eigen::RowVectorXd SplineInterp::scaled_values(VectorXd const &x_vec) const 
{
  return x_vec.unaryExpr([this](double x) { return scaled_value(x); }).transpose();
}
