#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPOpenFOAMReader.h>
#include <vtkContourFilter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <string>
#include <sstream>
#include <ostream>
#include <vector>
#include <stdlib.h>
#include<matplotlibcpp.h>
#include<jsoncons/json.hpp>

namespace plt = matplotlibcpp;

// contour by contour_val, put y coordinates into heights, 
// and x coordinates into xaxis
void getContour(vtkSmartPointer<vtkContourFilter> contourFilter, double contour_val,
								std::vector<double>& heights, std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  int numPoints = polys->GetNumberOfPoints();
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
  	polys->GetPoint(j,point);
  	if (point[2] == 0)
  	{
  		xaxis.push_back(point[0]);
  		heights.push_back(point[1]);
  	}	
  }
}

// contour by contour_val, put y coords into heights, 
// data with name dataName into datas and x coords into xaxis
void getContour(vtkSmartPointer<vtkContourFilter> contourFilter, double contour_val, 
								const std::string& dataName, std::vector<double>& heights, 
								std::vector<double>& datas, std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  vtkSmartPointer<vtkPointData> pd = polys->GetPointData();
  int numPoints = polys->GetNumberOfPoints();
  for (int j = 0; j < numPoints; ++j)
  {
  	double point[3];
  	double data[1];
  	polys->GetPoint(j,point);
  	if (point[2] == 0)
  	{
  		xaxis.push_back(point[0]);
  		heights.push_back(point[1]);
  		pd->GetArray(dataName.c_str())->GetTuple(j,data);
  	  datas.push_back(data[0]);
  	}	
  }
}


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

void write(const std::vector<std::vector<double>>& data, const std::string& name)
{
  std::ofstream outputstream(name);
  if (!outputstream.good())
  {
    std::cerr << "Error creating file " << name << std::endl;
    exit(1);
  }
  for (int j = 0; j < data.size(); ++j)
  {
    for (int i = 0; i < data[j].size(); ++i)
    {
      outputstream << data[j][i] << ",";
    }
    outputstream << std::endl;
  }
}

struct Args 
{
  std::string caseName;
  int caseType;
  double beg;
  double stride;
  double end;
  std::string contourArray;
  double contour_val;
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  std::string dataOnContour;
  double dymin;
  double dymax;
  bool write;
};

Args readJSON(jsoncons::json inputjson)
{
  Args args;
  // required arguments
  args.caseName = inputjson["Case Name"].as<std::string>();
  std::string casetype = inputjson["Case Type"].as<std::string>();
  if (!casetype.compare("reconstructed"))
    args.caseType = 1;
  else
    args.caseType = 0;
  args.beg = inputjson["Times"]["start"].as<double>();
  args.stride = inputjson["Times"]["stride"].as<double>();
  args.end = inputjson["Times"]["end"].as<double>();
  jsoncons::json contouropt = inputjson["Contour"];
  args.contourArray = contouropt["array"].as<std::string>();
  args.contour_val = contouropt["value"].as<double>();
  // optional
  if (contouropt.has_key("xmin"))
    args.xmin = contouropt["xmin"].as<double>();
  else
    args.xmin = INFINITY;
  if (contouropt.has_key("xmax"))
    args.xmax = contouropt["xmax"].as<double>();
  else
    args.xmax = INFINITY;
  if (contouropt.has_key("ymin"))
    args.ymin = contouropt["ymin"].as<double>();
  else
    args.ymin = INFINITY;
  if (contouropt.has_key("ymax"))
    args.ymax = contouropt["ymax"].as<double>();
  else
    args.ymax = INFINITY;
  if (contouropt.has_key("data"))
  {
    args.dataOnContour = contouropt["data"]["name"].as<std::string>();
    if (contouropt["data"].has_key("ymin"))
      args.dymin = contouropt["data"]["ymin"].as<double>();
    else
      args.dymin = INFINITY;
    if (contouropt["data"].has_key("ymax"))
      args.dymax = contouropt["data"]["ymax"].as<double>();
    else
      args.dymax = INFINITY;
  }
  else
    args.dataOnContour = "";
  args.write = inputjson["Write"].as<bool>(); 
  return args;
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
  Args args = readJSON(inputjson);

  // Read the file
  vtkSmartPointer<vtkPOpenFOAMReader> reader =
    vtkSmartPointer<vtkPOpenFOAMReader>::New();

	reader->SetCaseType(args.caseType);
	reader->SetFileName(args.caseName.c_str());
	reader->ListTimeStepsByControlDictOn();
	reader->SkipZeroTimeOn();
	reader->Update();
	bool arrayExists = false; 
	bool dataArrayExists = args.dataOnContour.empty(); 

	for (int i = 0; i < reader->GetNumberOfCellArrays(); ++i)
	{
		std::string arrayName(reader->GetCellArrayName(i));
		if (arrayName.compare(args.contourArray) == 0)
		{	
			arrayExists = true;
		}
		if (!dataArrayExists)
		{	
			if (arrayName.compare(args.dataOnContour) == 0)
			{
				dataArrayExists = true;
			}	
		}
	}
	if (not arrayExists)
	{
		std::cerr << args.contourArray << " does not exist in data set" << std::endl;
		exit(1);
	}
	if (not dataArrayExists)
	{
		std::cerr << args.dataOnContour << " does not exist in data set" << std::endl;
		exit(1);
	}
	
	// loop over times and contour
	vtkSmartPointer<vtkContourFilter> contourFilter =
		vtkSmartPointer<vtkContourFilter>::New();
	double nT = (args.end-args.beg)/args.stride;
  std::vector<std::vector<double>> heightsOverTime;
  std::vector<std::vector<double>> xAxesOverTime;
  std::vector<std::vector<double>> dataOverTime;
  for (int i = 0; i <= nT; ++i)
	{
		double time = i*args.stride+args.beg;
		std::cout << "TIME: " << time << std::endl;
		// update to current time step
		reader->UpdateTimeStep(time);
		// interpolate cell centered data to vertices
  	reader->CreateCellToPointOn();
  	reader->Update();
  	// pull out grid
		vtkUnstructuredGrid * currMesh = 
			vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  	currMesh->GetPointData()->SetActiveScalars(args.contourArray.c_str());
		contourFilter->SetInputData(currMesh);
		std::stringstream ss; ss << "Time" << time << ".png"; 
		std::string figName(trim_fname(args.caseName,ss.str())); 
		std::vector<double> heights, contourData, xaxis;
		if (dataArrayExists)
		{
			getContour(contourFilter,args.contour_val,args.dataOnContour,heights,contourData,xaxis);
      if (args.write)
      {
        xAxesOverTime.push_back(xaxis);
        heightsOverTime.push_back(heights);
        dataOverTime.push_back(contourData);
      }
      // define names for plt legend
			plt::clf();
			plt::subplot(2,1,1);
			plt::named_plot("height (m)", xaxis, heights,"b.");
			if (!(isinf(args.xmin)  || isinf(args.xmax)))
        plt::xlim(args.xmin,args.xmax);
			if (!(isinf(args.ymin) || isinf(args.ymax)))
        plt::ylim(args.ymin,args.ymax);
      plt::legend();
      std::stringstream ss; 
			ss<<"Contour by "<<args.contourArray<<"="<<args.contour_val<<", Time: "<<time;
			std::string titleText(ss.str());
			plt::title(titleText);
			plt::grid(true);
			plt::subplot(2,1,2);
			plt::named_plot(args.dataOnContour, xaxis, contourData, "k.");
			if (!(isinf(args.xmin)  || isinf(args.xmax)))
        plt::xlim(args.xmin,args.xmax);
			if (!(isinf(args.dymin) || isinf(args.dymax)))
        plt::ylim(args.dymin,args.dymax);
			plt::grid(true);	
			plt::legend();
			plt::save(figName);
			plt::pause(0.0001);
		}
		else
		{
			getContour(contourFilter,args.contour_val, heights, xaxis);
			if (args.write)
      {
        xAxesOverTime.push_back(xaxis);
        heightsOverTime.push_back(xaxis);
      }
      plt::clf();
			plt::named_plot("height (m)", xaxis, heights,"b.");
			if (!(isinf(args.xmin) || isinf(args.xmax)))
        plt::xlim(args.xmin,args.xmax);
			if (!(isinf(args.ymin) || isinf(args.ymax)))
        plt::ylim(args.ymin,args.ymax);
			plt::legend();
			std::stringstream ss; 
			ss<<"Contour by "<<args.contourArray<<"="<<args.contour_val<<", Time: "<< time;
			std::string titleText(ss.str());
			plt::title(titleText);
			plt::grid(true);
			plt::save(figName);
			plt::pause(0.0001);
		}
	} 
  if(args.write)
  {
    write(xAxesOverTime, "xAxesOverTime.txt");
    write(heightsOverTime,"heightsOverTime.txt"); 
    if (dataArrayExists)
    {
      std::stringstream ss;
      ss << args.dataOnContour << "overTime.txt";
      write(dataOverTime,ss.str());
    } 
  }
	return 0;
}
