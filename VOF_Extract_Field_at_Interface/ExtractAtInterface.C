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

int main(int argc, char* argv[])
{
	#ifdef ENABLE_MPI
	MPI_Init(&argc, &argv);
	vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv, 1);
	#endif
	if (argc != 7 && argc != 8)
	{
		std::cerr << "Usage: " << argv[0] 
							<< " case.foam beg stride end field_name contour_val [data_on_contour]\n"; 
		exit(1);
	}
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
	reader->SetFileName(argv[1]);
	reader->ListTimeStepsByControlDictOn();
	reader->SkipZeroTimeOn();
	reader->Update();
	std::cout << "PROCID: " << procId << std::endl;
	// get requested contour and data_on_contour array names, 
	// check if they exist in dataset 
	if (procId == 0)
	{
		std::string contourArray(argv[5]);
		bool arrayExists = false; 
		std::string dataOnContour(argc == 8 ? argv[7] : "");
		bool dataArrayExists = dataOnContour.empty(); 

		for (int i = 0; i < reader->GetNumberOfCellArrays(); ++i)
		{
			std::string arrayName(reader->GetCellArrayName(i));
			if (arrayName.compare(contourArray) == 0)
			{	
				arrayExists = true;
			}
			if (!dataArrayExists)
			{	
				if (arrayName.compare(dataOnContour) == 0)
				{
					dataArrayExists = true;
				}	
			}
		}
		if (not arrayExists)
		{
			std::cerr << argv[5] << " does not exist in data set" << std::endl;
			exit(1);
		}
		if (not dataArrayExists)
		{
			std::cerr << argv[7] << " does not exist in data set" << std::endl;
			exit(1);
		}
		
		// loop over times and contour
		double beg, stride, end, contour_val;
		beg = atof(argv[2]);
		stride = atof(argv[3]);
   	end = atof(argv[4]);
		contour_val = atof(argv[6]);
		vtkSmartPointer<vtkContourFilter> contourFilter =
			vtkSmartPointer<vtkContourFilter>::New();
		double nT = (end-beg)/stride;
		for (int i = 0; i <= nT; ++i)
		{
			double time = i*stride+beg;
			std::cout << "TIME: " << time << std::endl;
			// update to current time step
			reader->UpdateTimeStep(time);
			// interpolate cell centered data to vertices
  		reader->CreateCellToPointOn();
  		reader->Update();
  		// pull out grid
			vtkUnstructuredGrid * currMesh = 
				vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  		currMesh->GetPointData()->SetActiveScalars(argv[5]);
			contourFilter->SetInputData(currMesh);
			std::stringstream ss; ss << "Time" << time << ".png"; 
			std::string figName(trim_fname(argv[1],ss.str())); 
			std::vector<double> heights, contourData, xaxis;
			if (argc == 8)
			{
				getContour(contourFilter,contour_val,dataOnContour,heights,contourData,xaxis);
				// define names for plt legend
				plt::clf();
				plt::subplot(2,1,1);
				plt::named_plot("height (m)", xaxis, heights,"b.");
				plt::xlim(0,100000);
				plt::ylim(-1000,500);
				plt::legend();
				std::stringstream ss; 
				ss << "Contour by " << contourArray << "=" << contour_val << ", Time: " << time;
				std::string titleText(ss.str());
				plt::title(titleText);
				plt::grid(true);
				plt::subplot(2,1,2);
				plt::named_plot(dataOnContour, xaxis, contourData, "k.");
				plt::xlim(0,100000);
				plt::ylim(-5000000,2000000);
				plt::grid(true);	
				plt::legend();
				plt::save(figName);
				plt::pause(0.0001);
			}
			else
			{
				getContour(contourFilter,contour_val, heights, xaxis);
				plt::clf();
				plt::named_plot("height (m)", xaxis, heights,"b.");
				plt::xlim(0,100000);
				plt::ylim(-1000,500);
				plt::legend();
				std::stringstream ss; 
				ss << "Contour by " << contourArray << "=" << contour_val << ", Time: " << time;
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
