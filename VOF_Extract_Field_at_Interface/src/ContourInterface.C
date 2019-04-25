#include<ContourInterface.H>

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
  reader->SkipZeroTimeOn();
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
  // interpolate cell centered data to vertices
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
void ContourInterface::getContour(std::vector<double>& heights, 
                                  std::vector<double>& datas, 
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
      pd->GetArray(dataNames[0].c_str())->GetTuple(j,data);
      datas.push_back(data[0]);
    }  
  }
}

// contour by contour_val, put y coords into heights, 
// data with names dataName into datas and x coords into xaxis
void ContourInterface::getContour(std::vector<double>& heights, 
                                  std::vector<std::vector<double>>& datas, 
                                  std::vector<double>& xaxis)
{
  contourFilter->SetValue(0,contour_val);
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  vtkSmartPointer<vtkPointData> pd = polys->GetPointData();
  int numPoints = polys->GetNumberOfPoints();
  int numDatas = dataNames.size();
  datas.resize(numDatas);
  for (int i = 0; i < numDatas; ++i)
  {
    std::vector<double> data;
    for (int j = 0; j < numPoints; ++j)
    {
      double point[3];
      double datum[1];
      polys->GetPoint(j,point);
      if (point[2] == 0)
      {
        xaxis.push_back(point[0]);
        heights.push_back(point[1]);
        pd->GetArray(dataNames[i].c_str())->GetTuple(j,datum);
        data.push_back(datum[0]);
      }  
    }
    datas[i] = data;
  }
}
