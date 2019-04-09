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

// asd;lfkjasdfkjadsf;lkj
namespace plt = matplotlibcpp;

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


void getContour(vtkSmartPointer<vtkContourFilter> contourFilter, double contour_val, 
								const std::string& dataName, std::vector<double>& heights, std::vector<double>& datas, 
								std::vector<double>& xaxis)
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

int main(int argc, char* argv[])
{
	if (argc != 4 && argc != 5)
	{
		std::cerr << "Usage: " << argv[0] 
							<< " case.foam field_name contour_val [data_on_contour]" << std::endl;
		exit(1);
	}
	//sdkf
  // Read the file
  vtkSmartPointer<vtkPOpenFOAMReader> reader =
    vtkSmartPointer<vtkPOpenFOAMReader>::New();
	reader->SetCaseType(1);
	reader->SetFileName(argv[1]);
	reader->ListTimeStepsByControlDictOn();
	reader->SkipZeroTimeOn();
	reader->Update();

	// get requested contour and data_on_contour array names, check if they exist in dataset 
	std::string contourArray(argv[2]);
	bool arrayExists = false; 
	std::string dataOnContour(argc == 5 ? argv[4] : "");
	bool dataArrayExists = (dataOnContour.empty() ? true : false); 

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
		std::cerr << argv[2] << " does not exist in data set" << std::endl;
		exit(1);
	}
	if (not dataArrayExists)
	{
		std::cerr << argv[4] << " does not exist in data set" << std::endl;
		exit(1);
	}
	
	// loop over times and contour
	vtkSmartPointer<vtkDoubleArray> times = reader->GetTimeValues();
	double contour_val = atof(argv[3]);
	vtkSmartPointer<vtkContourFilter> contourFilter =
		vtkSmartPointer<vtkContourFilter>::New();
	for (int i = 0; i < times->GetNumberOfTuples(); ++i)
	{
		double time = times->GetValue(i);
		std::cout << time << std::endl;
		// update to current time step
		reader->UpdateTimeStep(time);
		// interpolate cell centered data to vertices
  	reader->CreateCellToPointOn();
  	reader->Update();
  	// pull out grid
		vtkUnstructuredGrid * currMesh = 
			vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  	currMesh->GetPointData()->SetActiveScalars(argv[2]);
		contourFilter->SetInputData(currMesh);
		
		std::vector<double> heights, contourData, xaxis;
		if (argc == 5)
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
			plt::legend();
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
			plt::pause(0.0001);
		}
	}
}
