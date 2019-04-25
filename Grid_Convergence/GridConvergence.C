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
  //double mRefRatio;
  //double fRefRatio;
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
  //args->mRefRatio = mCaseInp["Refinement Ratio"].as<double>();
  //args->fRefRatio = mCaseInp["Refinement Ratio"].as<double>();
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
    OrderOfAccuracy::Create(args->cCase,args->mCase,args->fCase,args->cCaseType,args->mCaseType,
                            args->fCaseType, args->contourArray,args->contour_val,args->dataNames);

  double nT = (args->end-args->beg)/args->stride;
  std::vector<std::vector<double>> oac; oac.resize(nT);
  std::vector<std::vector<double>> asymp; asymp.resize(nT);
  for (int i = 0; i <= nT; ++i)
  {
    double time = i*args->stride+args->beg;
    oacObj->stepTo(time);
    oac[i] = oacObj->getOAC();
    asymp[i] = oacObj->getAsymp();
    std::vector<double> grid = toCVec(oacObj->getFuncGrid());
    std::vector<double> data = toCVec(oacObj->getfSpline0());
    plt::clf;
    plt::plot(grid,data); 
    plt::pause(0.001);
  }
  return 0;
}
