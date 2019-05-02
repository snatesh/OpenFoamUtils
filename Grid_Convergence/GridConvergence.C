#include<OrderOfAccuracy.H>
#include<jsoncons/json.hpp>
#include<matplotlibcpp.h>
#include<memory>
#include<math.h>

namespace plt = matplotlibcpp;

// input args class
struct Args 
{
  // coarse, medium and fine case names and types
  std::string cCase;
  std::string mCase;
  std::string fCase;
  int cCaseType;
  int mCaseType;
  int fCaseType;
  // refinement ratios
  double cmRefRatio;
  double mfRefRatio;
  // start, stride and end for frames
  // currently, assumes all cases have same write intervals and total time
  double beg;
  double stride;
  double end;
  // interface array (usually alpha.water)
  std::string contourArray;
  // contour by this value (usually 0.5)
  double contour_val;
  // names of solution arrays
  std::vector<std::string> dataNames;
  // plot settings
  std::string title;
  int width;
  int height;
};

// trim extension off of filename 
std::string trim_fname(const std::string& fname, const std::string& ext)
{
  size_t beg = 0;
  size_t end = fname.find_last_of('.');
  std::string name;
  if (end != -1) 
  {
    name = fname.substr(beg,end);
    name.append(ext);
    return name;
  }

  else 
  {
    std::cerr << "Error finding file extension for " << fname << std::endl;
    exit(1);
  }
}  

// construct Args class from input json file
std::unique_ptr<Args> readJSON(jsoncons::json inputjson)
{
  std::unique_ptr<Args> args(new Args());
  jsoncons::json cCaseInp = inputjson["Coarse Case"];
  jsoncons::json mCaseInp = inputjson["Medium Case"];
  jsoncons::json fCaseInp = inputjson["Fine Case"];
  args->cCase = cCaseInp["Name"].as<std::string>();
  args->mCase = mCaseInp["Name"].as<std::string>();
  args->fCase = fCaseInp["Name"].as<std::string>();
  std::string cCaseType = cCaseInp["Type"].as<std::string>();
  std::string mCaseType = mCaseInp["Type"].as<std::string>();
  std::string fCaseType = fCaseInp["Type"].as<std::string>();
  args->cCaseType = !cCaseType.compare("reconstructed") ? 1 : 0;
  args->mCaseType = !mCaseType.compare("reconstructed") ? 1 : 0;
  args->fCaseType = !fCaseType.compare("reconstructed") ? 1 : 0;
  args->cmRefRatio = mCaseInp["Refinement Ratio"].as<double>();
  args->mfRefRatio = fCaseInp["Refinement Ratio"].as<double>();
  args->beg = inputjson["Times"]["start"].as<double>();    
  args->stride = inputjson["Times"]["stride"].as<double>();
  args->end = inputjson["Times"]["end"].as<double>();
  jsoncons::json contouropt = inputjson["Contour"];
  args->contourArray = contouropt["array"].as<std::string>();
  args->contour_val = contouropt["value"].as<double>();
  jsoncons::json dataopt = inputjson["Data"];
  for (const auto& data : dataopt.array_range())
  {
    args->dataNames.push_back(data.as<std::string>());
  }
  args->title = inputjson["Plot"]["title"].as<std::string>();
  args->width = inputjson["Plot"]["width"].as<int>();
  args->height = inputjson["Plot"]["height"].as<int>();
  return args;
}

std::vector<double> toCVec(VectorXd& x)
{
	return std::vector<double>(x.data(),x.data()+x.size());
}
std::vector<double> toCVec(VectorXd&& x)
{
	return std::vector<double>(x.data(),x.data()+x.size());
}

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << "input.json\n";
    exit(1);
  }
  std::string jsonf(argv[1]);
  std::ifstream inputStream(jsonf);
  if (!inputStream.good())
  {
    std::cerr << "Error opening file " << jsonf << std::endl;
    exit(1);
  }
  jsoncons::json inputjson;
  inputStream >> inputjson;
  std::unique_ptr<Args> args = readJSON(inputjson);
  std::unique_ptr<OrderOfAccuracy> oacObj = 
    OrderOfAccuracy::Create(args->cCase,args->mCase,args->fCase,args->cCaseType,
                            args->mCaseType,args->fCaseType, args->contourArray,
                            args->contour_val,args->dataNames,args->cmRefRatio,
                            args->mfRefRatio);
  std::cout << args->cmRefRatio << " " << args->mfRefRatio << std::endl;
  double nT = (args->end-args->beg)/args->stride;
  std::vector<double> oac; oac.resize(nT+1);
  std::vector<double> asymp; asymp.resize(nT+1);
  int nplts = args->dataNames.size()+1;
  std::vector<std::vector<double>> diffsCM(nplts);
  std::vector<std::vector<double>> diffsMF(nplts);
  std::vector<double> times;
  plt::figure();
  plt::figure_size(args->width,args->height);
  std::string figName("figFrame"); 
  for (int i = 0; i <= nT; ++i)
  {
    double time = i*args->stride+args->beg;
    std::cout << "TIME : " << time << std::endl;
    oacObj->stepTo(time);
    times.push_back(time);

      //plt::figure_size(800,600);
    int k = 0;
    plt::clf();
    for (int j = 0; j < nplts;++j)
    {
      //oac[i] = oacObj->getOAC(j);
      //asymp[i] = oacObj->getAsymp(j);
      //VectorXd cData = oacObj->getCSpline(j);
      //VectorXd mData = oacObj->getMSpline(j);
      //VectorXd fData = oacObj->getFSpline(j);
      //VectorXd diffCM = (cData - mData).array().abs();
      //VectorXd diffMF = (fData - mData).array().abs();
      //VectorXd axis = oacObj->getFuncGrid();
      VectorXd cData = oacObj->getCData(j);
      VectorXd mData = oacObj->getMData(j);
      VectorXd fData = oacObj->getFData(j);
      VectorXd caxis = oacObj->getCAxis();
      VectorXd maxis = oacObj->getMAxis();
      VectorXd faxis = oacObj->getFAxis();
      //VectorXd relDiffCM, relDiffMF; 
      //int curr1 = 0; int curr2 = 0;
      //for (int k = 0; k < diffCM.size(); ++k)
      //{
      //  if (mData(k) > 1e-2)
      //  {
      //    relDiffCM.conservativeResize(curr1+1);
      //    relDiffCM(curr1) = abs(diffCM(k)/mData(k));
      //    curr1 += 1;
      //  }
      //  if (fData(k) > 1e-2)
      //  {
      //    relDiffMF.conservativeResize(curr2+1);
      //    relDiffMF(curr2) = abs(diffMF(k)/fData(k));
      //    curr2 += 1;
      //  }
      //}
      //double max = 
      //  std::max<double>(cData.maxCoeff(),std::max<double>(mData.maxCoeff(),fData.maxCoeff())); 
      //std::cout << diffCM.maxCoeff() << " " << diffMF.maxCoeff() << std::endl;
      ////relDiffCM = relDiffCM/max;
      ////relDiffMF = relDiffMF/max;
      //diffCM = diffCM/max;
      //diffMF = diffMF/max;
      //diffsCM[j].push_back(diffCM.lpNorm<1>()); 
      //diffsMF[j].push_back(diffMF.lpNorm<1>()); 


      //plt::subplot(nplts,1,j+1);
      //plt::named_semilogy("coarse-medium",toCVec(axis),toCVec(diffCM),"r-");
      //plt::legend();
      //plt::subplot(nplts,1,j+1);
      //plt::named_semilogy("medium-fine",toCVec(axis),toCVec(diffMF),"b-");
      //plt::legend();
      plt::subplot(nplts,1,j+1);
      plt::named_plot("coarse", toCVec(caxis), toCVec(cData),"b.-");
      plt::grid(true);
      plt::legend();
      plt::subplot(nplts,1,j+1);
      plt::named_plot("medium", toCVec(maxis), toCVec(mData),"r.-");
      plt::grid(true);
      plt::legend();
      plt::subplot(nplts,1,j+1);
      plt::named_plot("fine", toCVec(faxis), toCVec(fData),"g.-");
      plt::xlim(0,100000);
      plt::ylim(-4000,3000);
      std::stringstream ss; ss << args->title << ", Time="<<time;
      plt::title(ss.str());
      if (j == 0)
        plt::ylabel("Height (m)");
      else
        plt::ylabel(args->dataNames[j-1]);
      plt::xlabel("x (m)");
      plt::legend();

    }
    //for (int j = 0; j < nplts; ++j)
    //{
    //  plt::subplot(nplts,1,j+1);
    //  plt::named_semilogy("coarse-medium",times,diffsCM[j],"r-");
    //  plt::subplot(nplts,1,j+1);
    //  plt::named_semilogy("medium-fine",times,diffsMF[j],"b-");
    //  plt::xlabel("Time");
    //  plt::legend();
    //}
    std::stringstream ss; 
    ss << figName << std::internal << setw(3) << setfill('0') << i+1 << ".png"; 
    plt::save(ss.str());
    plt::pause(0.0001);

  }


  return 0;
}
