#include<OrderOfAccuracy.H>
#include<SplineInterp.H>
OrderOfAccuracy::OrderOfAccuracy(const std::string& cCase, const std::string& mCase, 
                                 const std::string& fCase, int cCaseType, int mCaseType,
                                 int fCaseType, const std::string& contourArray, double contour_val,
                                 std::vector<std::string>& dataNames,double _cmRefRatio,
                                 double _mfRefRatio)
{
  cContour = ContourInterface::Create(cCase,cCaseType,contourArray,contour_val,dataNames);
  mContour = ContourInterface::Create(mCase,mCaseType,contourArray,contour_val,dataNames);
  fContour = ContourInterface::Create(fCase,fCaseType,contourArray,contour_val,dataNames);
  oac.resize(dataNames.size()+1);
  //gci_cm.resize(oac.size());
  //gci_mf.resize(oac.size());
  asymp.resize(oac.size());
  cData.resize(oac.size());
  mData.resize(oac.size());
  fData.resize(oac.size());
  cSplineData.resize(oac.size());
  mSplineData.resize(oac.size());
  fSplineData.resize(oac.size());
  cmRefRatio = _cmRefRatio;
  mfRefRatio = _mfRefRatio;
}

std::unique_ptr<OrderOfAccuracy> 
OrderOfAccuracy::Create(const std::string& cCase, const std::string& mCase, 
                        const std::string& fCase, int cCaseType, int mCaseType,
                        int fCaseType, const std::string& contourArray, double contour_val,
                        std::vector<std::string>& dataNames,
                        double cmRefRatio, double mfRefRatio)
{
  return std::unique_ptr<OrderOfAccuracy>
          (new OrderOfAccuracy(cCase,mCase,fCase,cCaseType,mCaseType,fCaseType,
                               contourArray,contour_val,dataNames,cmRefRatio,
                               mfRefRatio)); 
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
  for (int j = 0; j < oac.size();++j)
  {
    if (j == 0)
    {
      computeDiff(cHeights,mHeights,fHeights,xmin,xmax,j);
    }
    else
    {
      computeDiff(cContourDatas[j-1],mContourDatas[j-1],fContourDatas[j-1],xmin,xmax,j);
    }
  }
}

void OrderOfAccuracy::computeDiff(std::vector<double>& cY, std::vector<double>& mY,
                                  std::vector<double>& fY, double xmin, double xmax, int j)
{
  cData[j] = toEigen(cY); mData[j] = toEigen(mY); fData[j] = toEigen(fY);
  // create spline interpolants
  SplineInterp cSpline(cAxis,cData[j],xmin,xmax);
  SplineInterp mSpline(mAxis,mData[j],xmin,xmax);
  SplineInterp fSpline(fAxis,fData[j],xmin,xmax);
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
  IterSolveP(j);
  ComputeAsympRatios(j);
}

void OrderOfAccuracy::IterSolveP(int j)
{
  double dx = funcGrid(1)-funcGrid(0);
  double diff_cm = dx*(cSplineData[j]-mSplineData[j]).lpNorm<1>();
  double diff_mf = dx*(mSplineData[j]-fSplineData[j]).lpNorm<1>();
  int k = 0; double p = 1.;
  while (Func(p,diff_cm,diff_mf) > 1e-16 && k < 1000)
  {
    p = p - Func(p,diff_cm,diff_mf)/derivFunc(p);
    k += 1;
  }
  oac[j] = p; 
}

void OrderOfAccuracy::ComputeAsympRatios(int j)
{
  double rmfp = std::pow(mfRefRatio,oac[j]); 
  double rcmp = std::pow(cmRefRatio,oac[j]);
  VectorXd diff_cm = cSplineData[j]-mSplineData[j];
  VectorXd diff_mf = mSplineData[j]-fSplineData[j];
  double gci_cm = (1.25/(rcmp-1))*diff_cm.norm()/mSplineData[j].lpNorm<1>();
  double gci_mf = (1.25/(rmfp-1))*diff_mf.norm()/fSplineData[j].lpNorm<1>();
  asymp[j] = gci_cm/(gci_mf*rmfp);
}



// helpers for newton iteration to solve for order p
double OrderOfAccuracy::Func(double p, double diff_cm, double diff_mf)
{
  double r21p = std::pow(mfRefRatio,p);
  double r32p = std::pow(cmRefRatio,p);
  //return log(diff_cm/diff_mf)-log(r21p*((r32p-1)/(r21p-1)));
  return p*log(mfRefRatio)+log(r32p-1)-log(r21p-1)+log(diff_mf)-log(diff_cm);
}

double OrderOfAccuracy::derivFunc(double p)
{
  double r21p = std::pow(mfRefRatio,p);
  double r32p = std::pow(cmRefRatio,p);
  //return log(r21p)/(r21p-1)-r32p*log(r32p)/(r32p-1);
  return log(mfRefRatio)-r21p*log(mfRefRatio)/(r21p-1)+r32p*log(cmRefRatio)/(r32p-1);
}

// helper to convert c++ vector to Eigen 
VectorXd toEigen(std::vector<double>& x)
{
  return Eigen::Map<VectorXd,Eigen::Unaligned>(x.data(),x.size());
}

double OrderOfAccuracy::getOAC(int j)
{
  return oac[j];
}

double OrderOfAccuracy::getAsymp(int j)
{
  return asymp[j];
}
VectorXd OrderOfAccuracy::getCSpline(int j)
{
  return cSplineData[j];
}  
VectorXd OrderOfAccuracy::getMSpline(int j)
{
  return mSplineData[j];
}  
VectorXd OrderOfAccuracy::getFSpline(int j)
{
  return fSplineData[j];
}  
VectorXd OrderOfAccuracy::getFuncGrid()
{
  return funcGrid;
}
VectorXd OrderOfAccuracy::getCData(int j)
{
  return cData[j];
}
VectorXd OrderOfAccuracy::getCAxis()
{
  return cAxis;
}
VectorXd OrderOfAccuracy::getMData(int j)
{
  return mData[j];
}
VectorXd OrderOfAccuracy::getMAxis()
{
  return mAxis;
}
VectorXd OrderOfAccuracy::getFData(int j)
{
  return fData[j];
}
VectorXd OrderOfAccuracy::getFAxis()
{
  return fAxis;
}


double OrderOfAccuracy::getFuncGrid(int j)
{
  return funcGrid(j);
}
