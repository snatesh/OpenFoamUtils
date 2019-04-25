#include<OrderOfAccuracy.H>
#include<SplineInterp.H>
OrderOfAccuracy::OrderOfAccuracy(const std::string& cCase, const std::string& mCase, 
                                 const std::string& fCase, int cCaseType, int mCaseType,
                                 int fCaseType, const std::string& contourArray, double contour_val,
                                 std::vector<std::string>& dataNames)
{
  cContour = ContourInterface::Create(cCase,cCaseType,contourArray,contour_val,dataNames);
  mContour = ContourInterface::Create(mCase,mCaseType,contourArray,contour_val,dataNames);
  fContour = ContourInterface::Create(fCase,fCaseType,contourArray,contour_val,dataNames);
  oac.resize(dataNames.size());
  gci_cm.resize(oac.size());
  gci_mf.resize(oac.size());
  asymp.resize(oac.size());
  cData.resize(oac.size());
  mData.resize(oac.size());
  fData.resize(oac.size());
  cSplineData.resize(oac.size());
  mSplineData.resize(oac.size());
  fSplineData.resize(oac.size());
}

std::unique_ptr<OrderOfAccuracy> 
OrderOfAccuracy::Create(const std::string& cCase, const std::string& mCase, 
                        const std::string& fCase, int cCaseType, int mCaseType,
                        int fCaseType, const std::string& contourArray, double contour_val,
                        std::vector<std::string>& dataNames)
{
  return std::unique_ptr<OrderOfAccuracy>
          (new OrderOfAccuracy(cCase,mCase,fCase,cCaseType,mCaseType,fCaseType,
                               contourArray,contour_val,dataNames)); 
}

void OrderOfAccuracy::stepTo(double time)
{
  cContour->stepTo(time);
  mContour->stepTo(time);
  fContour->stepTo(time);
  std::vector<double> cHeights,mHeights,fHeights,cX,mX,fX;
  std::vector<std::vector<double>> cContourDatas;
  std::vector<std::vector<double>> mContourDatas;
  std::vector<std::vector<double>> fContourDatas;
  cContour->getContour(cHeights,cContourDatas,cX);
  mContour->getContour(mHeights,mContourDatas,mX);
  fContour->getContour(fHeights,fContourDatas,fX);
  cAxis = toEigen(cX); mAxis = toEigen(mX); fAxis = toEigen(fX);
  double xmin = 
    std::max<double>(std::max<double>(cAxis.minCoeff(),mAxis.minCoeff()),fAxis.minCoeff());
  double xmax = 
    std::min<double>(std::min<double>(cAxis.maxCoeff(),mAxis.maxCoeff()),fAxis.maxCoeff());
  for (int j = 0; j < cContourDatas.size();++j)
  {
    computeDiff(cContourDatas[j],mContourDatas[j],fContourDatas[j],xmin,xmax,j);
  }
}

void OrderOfAccuracy::computeDiff(std::vector<double>& cY, std::vector<double>& mY,std::vector<double>& fY, 
                                  double xmin, double xmax, int j)
{
  cData[j] = toEigen(cY); mData[j] = toEigen(mY); fData[j] = toEigen(fY);
  // create spline interpolants
  SplineInterp cSpline(cAxis,cData[j]);
  SplineInterp mSpline(mAxis,mData[j]);
  SplineInterp fSpline(fAxis,fData[j]);
  // evaluate interpolants on refined grid (2 times the # of pts in fX)
  funcGrid = VectorXd::LinSpaced(2*fY.size(),xmin,xmax);
  cSplineData[j].resize(funcGrid.size()); mSplineData[j].resize(funcGrid.size()); 
  fSplineData[j].resize(funcGrid.size());
  for (int k = 0; k < funcGrid.size(); ++k)
  {
    double x = funcGrid(k);
    cSplineData[j](k) = cSpline(x); 
    mSplineData[j](k) = mSpline(x);
    fSplineData[j](k) = fSpline(x);
  }
  // evaluate L2-norm of error between coarse and medium
  double diff_cm = (sqrt(funcGrid(1)-funcGrid(0))*((cSplineData[j]-mSplineData[j]).norm()));
  double diff_mf = (sqrt(funcGrid(1)-funcGrid(0))*((mSplineData[j]-fSplineData[j]).norm()));
  oac[j] = log(std::fabs(diff_cm)/std::fabs(diff_mf))/log(2); 
  double rp = std::pow(2,oac[j]);
  gci_cm[j] = (1.25/(rp-1))*std::fabs(diff_cm/mSplineData[j].norm());
  gci_mf[j] = (1.25/(rp-1))*std::fabs(diff_mf/fSplineData[j].norm());
  asymp[j] = gci_cm[j]/(gci_mf[j]*rp);
}

std::vector<double> OrderOfAccuracy::getOAC()
{
  return oac;
}

std::vector<double> OrderOfAccuracy::getAsymp()
{
  return asymp;
}
// helper to convert c++ vector to Eigen 
VectorXd toEigen(std::vector<double>& x)
{
  return Eigen::Map<VectorXd,Eigen::Unaligned>(x.data(),x.size());
}

