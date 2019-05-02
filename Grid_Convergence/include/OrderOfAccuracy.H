#ifndef ORDERACC_H
#define ORDERACC_H
#include<ContourInterface.H>
#include<Eigen/Dense>
using Eigen::VectorXd;
class OrderOfAccuracy
{
  public:
    // ctors
    OrderOfAccuracy(const std::string& cCase, const std::string& mCase, 
                    const std::string& fCase, int cCaseType, int mCaseType,
                    int fCaseType, const std::string& contourArray, double contour_val,
                    std::vector<std::string>& dataNames, double _cmRefRatio, 
                    double _mfRefRatio);
  
    static std::unique_ptr<OrderOfAccuracy> 
      Create(const std::string& cCase, const std::string& mCase, 
             const std::string& fCase, int cCaseType, int mCaseType,
             int fCaseType, const std::string& contourArray, double contour_val,
             std::vector<std::string>& dataNames,double cmRefRatio,
             double mfRefRatio);
    // compute errors, order of accuracy and asymptotic ratio based on GCI
    void computeDiff(std::vector<double>& cY, std::vector<double>& mY,std::vector<double>& fY, 
                     double xmin, double xmax, int j);

    // step cases to t=time
    void stepTo(double time);    

    ~OrderOfAccuracy(){};
    
    // access
    double getOAC(int j);
    double getAsymp(int j);
    VectorXd getCSpline(int j);
    VectorXd getMSpline(int j);
    VectorXd getFSpline(int j);
    VectorXd getFuncGrid();
    double getFuncGrid(int j);
    VectorXd getCData(int j);
    VectorXd getCAxis();
    VectorXd getMData(int j);
    VectorXd getMAxis();
    VectorXd getFData(int j);
    VectorXd getFAxis();
  
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
    //std::vector<double> gci_cm;
    //std::vector<double> gci_mf;
    // asymptotic ratios
    std::vector<double> asymp;
    // refinement ratios
    double cmRefRatio;  
    double mfRefRatio;
    // helpers for newton iteration to solve for order p
    double Func(double p, double diff_cm, double diff_mf);
    double derivFunc(double p);
    void IterSolveP(int j);
    void ComputeAsympRatios(int j);
};

// helper to convert c++ vector to Eigen 
VectorXd toEigen(std::vector<double>& x);


#endif
