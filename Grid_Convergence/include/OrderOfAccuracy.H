#ifndef ORDERACC_H
#define ORDERACC_H
#include<ContourInterface.H>
#include<Eigen/Dense>
using Eigen::VectorXd;
class OrderOfAccuracy
{
  public:
    OrderOfAccuracy(const std::string& cCase, const std::string& mCase, 
                    const std::string& fCase, int cCaseType, int mCaseType,
                    int fCaseType, const std::string& contourArray, double contour_val,
                    std::vector<std::string>& dataNames);
  
    static std::unique_ptr<OrderOfAccuracy> 
      Create(const std::string& cCase, const std::string& mCase, 
             const std::string& fCase, int cCaseType, int mCaseType,
             int fCaseType, const std::string& contourArray, double contour_val,
             std::vector<std::string>& dataNames);

    void computeDiff(std::vector<double>& cY, std::vector<double>& mY,std::vector<double>& fY, 
                     double xmin, double xmax, int j);

    void stepTo(double time);    

    ~OrderOfAccuracy(){};
  
    std::vector<double> getOAC();
    std::vector<double> getAsymp();
    VectorXd getfSpline0()
    {
      return fSplineData[0];
    }  
    VectorXd getFuncGrid()
    {
      return funcGrid;
    }
 
  private:
    // contours for coarse, medium and fine grid
    std::unique_ptr<ContourInterface> cContour; 
    std::unique_ptr<ContourInterface> mContour; 
    std::unique_ptr<ContourInterface> fContour; 
    // coarse grid data and axis 
    std::vector<VectorXd> cData;
    VectorXd cAxis;
    // medium grid data and axis 
    std::vector<VectorXd> mData;
    VectorXd mAxis;
    // fine grid data and axis
    std::vector<VectorXd> fData;
    VectorXd fAxis;
    // coarse data interpolated via cubic bspline 
    std::vector<VectorXd> cSplineData;
    // medium data interpolated via cubic bspline
    std::vector<VectorXd> mSplineData;
    // fine data interpolated via cubic bspline
    std::vector<VectorXd> fSplineData;
    // interpolationg grid
    VectorXd funcGrid;
    // observed order of acc
    std::vector<double> oac;
    // grid convergence indices
    std::vector<double> gci_cm;
    std::vector<double> gci_mf;
    // asymptotic ratios
    std::vector<double> asymp;
};

// helper to convert c++ vector to Eigen 
VectorXd toEigen(std::vector<double>& x);

#endif
