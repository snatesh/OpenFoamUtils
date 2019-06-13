#include<ContourInterface.H>
#include<vtkPoints.h>
#include<vtkPointLocator.h>
#include<vtkIdList.h>
#include<algorithm>
#include<map>

ContourInterface::ContourInterface(const std::string& _caseName, int _caseType, 
                                   const std::string& _contourArray, double _contour_val, 
                                   std::vector<std::string>& _dataNames)
  : caseName(_caseName),caseType(_caseType),contourArray(_contourArray),
    contour_val(_contour_val),dataNames(_dataNames)
{
  initReader();
  contourFilter = vtkSmartPointer<vtkContourFilter>::New();
}

std::unique_ptr<ContourInterface> 
  ContourInterface::Create(const std::string& _caseName, int _caseType,
                           const std::string& _contourArray, double _contour_val, 
                           std::vector<std::string>& _dataNames)
{
  return std::unique_ptr<ContourInterface>
          (new ContourInterface(_caseName,_caseType,_contourArray,_contour_val,_dataNames)); 
}

void ContourInterface::initReader()
{
  // Read the file
  reader = vtkSmartPointer<vtkPOpenFOAMReader>::New();
  reader->SetCaseType(caseType);
  reader->SetFileName(caseName.c_str());
  reader->ListTimeStepsByControlDictOn();
  reader->SkipZeroTimeOff();
  reader->Update();
  bool arrayExists = false; 
  bool dataArrayExists = false; 
  for (int i = 0; i < reader->GetNumberOfCellArrays(); ++i)
  {
    std::string arrayName(reader->GetCellArrayName(i));
    if (arrayName.compare(contourArray) == 0)
    {  
      arrayExists = true;
    }
    if (!dataArrayExists)
    { 
      int count = 0; 
      for (int j = 0; j < dataNames.size(); ++j)
      {
        if (arrayName.compare(dataNames[j]) == 0)
        {
          count = count+1;
        }
      }
      if (count == dataNames.size())
      {
        dataArrayExists = true;
      }  
    }
  }
  if (not arrayExists)
  {
    std::cerr << contourArray << " does not exist in data set" << std::endl;
    exit(1);
  }
  if (not dataArrayExists and not dataNames[0].empty())
  {
    std::cerr << " Could not find one of the requested arrays in data set" << std::endl;
    exit(1);
  }
}

void ContourInterface::stepTo(double time)
{
  // update to current time step
  reader->UpdateTimeStep(time);
  // NOT interpolating cell centered data to vertices
  //reader->CreateCellToPointOff();
  reader->CreateCellToPointOn();
	reader->Update();
  // pull out grid
  vtkUnstructuredGrid * currMesh = 
    vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  currMesh->GetPointData()->SetActiveScalars(contourArray.c_str());
  contourFilter->SetInputData(currMesh);
}

// contour by contour_val, put y coordinates into heights, 
// and x coordinates into xaxis
void ContourInterface::getContour(std::vector<double>& heights, 
                                  std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
	contourFilter->UseScalarTreeOn();
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  //vtkSmartPointer<vtkCellCenters> cellCenters 
	//	= vtkSmartPointer<vtkCellCenters>::New();
	//cellCenters->SetInputData(polys);
	//cellCenters->Update();
	//int numPoints = cellCenters->GetOutput()->GetNumberOfPoints();
  int numPoints = polys->GetNumberOfPoints();
	std::map<double,double> axisHeightMap;
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    //cellCenters->GetOutput()->GetPoint(j,point);
    polys->GetPoint(j,point);
    if (point[2] == 0)
    {
      axisHeightMap[point[0]] = point[1];
    }  
  }
  xaxis.resize(axisHeightMap.size());
  heights.resize(xaxis.size());
  int j = 0;
  for (auto it = axisHeightMap.begin(); it != axisHeightMap.end(); ++it)
  {
    xaxis[j] = it->first;
    heights[j] = it->second;
    j = j + 1;
  }
}

void ContourInterface::getRadialContour(std::vector<double>& heights, std::vector<double>& raxis)
{
  contourFilter->SetValue(0,contour_val);
	contourFilter->UseScalarTreeOn();
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  //vtkSmartPointer<vtkCellCenters> cellCenters 
	//	= vtkSmartPointer<vtkCellCenters>::New();
	//cellCenters->SetInputData(polys);
	//cellCenters->Update();
	//int numPoints = cellCenters->GetOutput()->GetNumberOfPoints();
	int numPoints = polys->GetNumberOfPoints();
	heights.resize(numPoints); raxis.resize(numPoints);
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    //cellCenters->GetOutput()->GetPoint(j,point);
    polys->GetPoint(j,point);
  	//if(point[0] < 0.16 || point[2] < 0.16 || std::abs(point[0]-point[2]) < 0.32)
		//{ 
	 		heights[j] = point[1];
			//std::cout << point[1] << std::endl;
    	//heights.push_back(point[1]);
			//raxis.push_back(std::sqrt(std::pow(point[0],2)+std::pow(point[2],2)));
			raxis[j] = std::sqrt(std::pow(point[0],2)+std::pow(point[2],2)); 
  	//}
	}
}    

// contour by contour_val, put inverse weighted distance average of y coordinates
// at given radial distance, with respect to mean of coordinates, into heights
// then, do another round of smoothing on the heights array wrt nearest neighbors
// in (r,h) space. lastly, r= sqrt(x^2+z^2) coordinates go into raxis
// NOTE: Most of the if blocks are for preventing NAN values
void ContourInterface::getSmoothRadialContour(std::vector<double>& heights, 
																							std::vector<double>& smoothHeights,
																							std::vector<double>& raxis,
																							std::vector<double>& raxisSort)
{
  contourFilter->SetValue(0,contour_val);
	contourFilter->UseScalarTreeOn();
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
	int numPoints = polys->GetNumberOfPoints();
	heights.resize(numPoints); raxis.resize(numPoints);
	std::map<double,std::vector<double>> raxisHeightMap;
	for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    polys->GetPoint(j,point);
	 	heights[j] = point[1];
		raxis[j] = std::sqrt(std::pow(point[0],2)+std::pow(point[2],2)); 
		raxisHeightMap[raxis[j]].push_back(heights[j]);
	}
  smoothHeights.resize(raxisHeightMap.size());
	raxisSort.resize(smoothHeights.size());
  int j = 0;
	std::cout << "Smoothing mulivalued radial contour" << std::endl;
  for (auto it = raxisHeightMap.begin(); it != raxisHeightMap.end(); ++it)
  {
		// get average height at this radius
  	double heightsAve = 0;
		for (int i = 0; i < it->second.size(); ++i)
		{
			heightsAve += it->second[i];
		}  
		heightsAve /= it->second.size();
		// get inverse distance weights relative to average
    double totWeight = 0;
		std::vector<double> hInd;
		for (int i = 0; i < it->second.size(); ++i)
		{
			if (!(std::abs(it->second[i] -heightsAve) < 1e-15))
			{
				totWeight += 1./(std::abs(it->second[i]-heightsAve));
				hInd.push_back(i);
			}
		}
		if (!totWeight)
		{
			smoothHeights[j] = it->second[0];
		}	
		else
		{
			// calculate inverse distance weighted average of height
			smoothHeights[j] = 0;
			for (int i = 0; i < hInd.size(); ++i)
			{
				smoothHeights[j] 
					+= it->second[hInd[i]]*1./(std::abs(it->second[hInd[i]]-heightsAve));
			}
			smoothHeights[j] /= totWeight;
		}
		raxisSort[j] = it->first;
		j = j + 1;
  }
	// rounds of smoothing in (r,h) space
	int nCycles = 5;
	std::cout << "Smoothing radial contour in (r,h) space for " << nCycles 
						<< " cycles" << std::endl;
	// do several rounds of inverse distance averaging in (r,h) space with 
	// nearest nNeighbr points to ith radius. This is like convolving a 
	// weighted symmetric box over the scattered heights, starting nNeighbrs deep 
	// into the heights array
	for (int cycle = 1; cycle <= nCycles; ++cycle)
	{
		std::cout << "\t cycle = " << cycle << std::endl;

		int nNeighbrs = 25;
		int lo = (int) nNeighbrs - std::ceil(nNeighbrs/2);	
		int up = (int) nNeighbrs + std::floor(nNeighbrs/2);
		int end = raxisSort.size() - std::floor(nNeighbrs/2);
		for (int i = nNeighbrs; i < end; ++i)
		{
			double totWeight = 0;
			std::vector<double> hInd;
			for (int j = lo; j < up; ++j)
			{
				double dist = std::abs(raxisSort[j]-raxisSort[i]);
				if (!(dist < 1e-15))
				{
					totWeight += 1./dist;
					hInd.push_back(j);
				}
			}
			if (totWeight > 1e-15)
			{
				smoothHeights[i] = 0;
				for (int j = 0; j < hInd.size(); ++j)
				{
					double dist = std::abs(raxisSort[i] - raxisSort[hInd[j]]);
					smoothHeights[i] += smoothHeights[hInd[j]]*1./dist;
				}
				smoothHeights[i] /= totWeight;
			}
			lo += 1;
			up += 1;
		}
	}
}    
// contour by contour_val, put y coords into heights, 
// data with name dataName into datas and x coords into xaxis
void ContourInterface::getContour(std::vector<double>& heights, 
                                  std::vector<double>& datas, 
                                  std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
	contourFilter->UseScalarTreeOn();
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  vtkSmartPointer<vtkPointData> pd = polys->GetPointData();
  int numPoints = polys->GetNumberOfPoints();
  std::map<double,double> axisHeightMap;
  std::map<double,double> axisDataMap;
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    double datum[1];
    polys->GetPoint(j,point);
    if (point[2] == 0)
    {
      axisHeightMap[point[0]] = point[1];
      pd->GetArray(dataNames[0].c_str())->GetTuple(j,datum);
      axisDataMap[point[0]] = datum[0];
    }  
  }
  xaxis.resize(axisHeightMap.size());
  heights.resize(xaxis.size());
  datas.resize(xaxis.size());
  int j = 0;
  for (auto it = axisHeightMap.begin(); it != axisHeightMap.end(); ++it)
  {
    xaxis[j] = it->first;
    heights[j] = it->second;
    datas[j] = axisDataMap[it->first];
    j = j + 1;
  }
}

// contour by contour_val, put y coords into heights, 
// data with names dataName into datas and x coords into xaxis
void ContourInterface::getContour(std::vector<double>& heights, 
                                  std::vector<std::vector<double>>& datas, 
                                  std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
	contourFilter->UseScalarTreeOn();
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  vtkSmartPointer<vtkPointData> pd = polys->GetPointData();
  int numPoints = polys->GetNumberOfPoints();
  int numDatas = dataNames.size();
  std::map<double,double> axisHeightMap;
  std::map<double,std::vector<double>> axisDataMap;
  std::vector<double> data(numDatas);
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    double datum[1];
    polys->GetPoint(j,point);
    if (point[2] == 0)
    {
      axisHeightMap[point[0]] = point[1];

      for (int i = 0; i < numDatas; ++i)
      {
        pd->GetArray(dataNames[i].c_str())->GetTuple(j,datum);
        data[i] = datum[0];
      }
      axisDataMap[point[0]] = data;
    }  
  }
  xaxis.resize(axisHeightMap.size()); 
  heights.resize(xaxis.size());
  datas.resize(numDatas);
  int j = 0;
  for (auto it = axisHeightMap.begin(); it != axisHeightMap.end(); ++it)
  {
    xaxis[j] = it->first;
    heights[j] = it->second;
    for (int i = 0; i < numDatas; ++i)
    {
      datas[i].push_back(axisDataMap[it->first][i]);
    }
    j = j+1;
  }  
}
