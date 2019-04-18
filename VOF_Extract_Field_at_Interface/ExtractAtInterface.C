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
#ifdef ENABLE_MPI
	#include <vtkMPIController.h>
	#include <mpi.h>
#endif

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
    std::cout << "Error finding file extension for " << fname << std::endl;
    exit(1);
  }
}  

struct Args 
{
  std::string caseName;
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
};

int main(int argc, char* argv[])
{
	#ifdef ENABLE_MPI
	MPI_Init(&argc, &argv);
	vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv, 1);
	#endif
	if (argc < 7)
	{
		std::cerr << "Usage: " << argv[0] 
							<< " case.foam beg stride end field_name contour_val"
              << " [xmin xmax ymin ymax data_on_contour ymin ymax]\n"; 
		exit(1);
	}

  Args args = 
    {argv[1],atof(argv[2]),atof(argv[3]),atof(argv[4]),argv[5],atof(argv[6])};
  args.xmin = (argc >= 8 ? atof(argv[7]) : INFINITY);
  args.xmax = (argc >= 9 ? atof(argv[8]) : INFINITY); 
  args.ymin = (argc >= 10 ? atof(argv[9]) : INFINITY);
  args.ymax = (argc >= 11 ? atof(argv[10]) : INFINITY); 
  args.dataOnContour = (argc >= 12 ? argv[11] : "");
  args.dymin = (argc >= 13 ? atof(argv[12]) : INFINITY);
  args.dymax = (argc == 14 ? atof(argv[13]) : INFINITY); 
  std::cout << args.xmin << " " << args.xmax << std::endl;
  std::cout << args.ymin << " " << args.ymax << std::endl;
  // Read the file
  vtkSmartPointer<vtkPOpenFOAMReader> reader =
    vtkSmartPointer<vtkPOpenFOAMReader>::New();
	#ifdef ENABLE_MPI
	reader->SetController(controller);
	reader->SetCaseType(0);
	int procId = controller->GetLocalProcessId();
	#else
	reader->SetCaseType(1);
	int procId = 0;
	#endif
	reader->SetFileName(args.caseName.c_str());
	reader->ListTimeStepsByControlDictOn();
	reader->SkipZeroTimeOn();
	reader->Update();
	// get requested contour and data_on_contour array names, 
	// check if they exist in dataset 
	if (procId == 0)
	{
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
			if (argc >= 12)
			{
				getContour(contourFilter,args.contour_val,args.dataOnContour,heights,contourData,xaxis);
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
				ss << "Contour by " << args.contourArray << "=" << args.contour_val << ", Time: " << time;
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
				plt::clf();
				plt::named_plot("height (m)", xaxis, heights,"b.");
				if (!(isinf(args.xmin) || isinf(args.xmax)))
          plt::xlim(args.xmin,args.xmax);
				if (!(isinf(args.ymin) || isinf(args.ymax)))
          plt::ylim(args.ymin,args.ymax);
				plt::legend();
				std::stringstream ss; 
				ss << "Contour by " << args.contourArray << "=" << args.contour_val << ", Time: " << time;
				std::string titleText(ss.str());
				plt::title(titleText);
				plt::grid(true);
				plt::save(figName);
				plt::pause(0.0001);
			}
		}
	}
	#ifdef ENABLE_MPI
	controller->Finalize();
	controller->Delete();
	#endif
	return 0;
}
