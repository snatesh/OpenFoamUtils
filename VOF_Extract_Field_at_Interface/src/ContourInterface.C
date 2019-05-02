#include<ContourInterface.H>
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
  std::map<double,double> axisHeightMap;
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
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
  contourFilter->Update();
  vtkSmartPointer<vtkPolyData> polys = contourFilter->GetOutput();
  int numPoints = polys->GetNumberOfPoints();
  std::map<double,double> axisHeightMap;
  heights.resize(numPoints); raxis.resize(numPoints);
  for (int j = 0; j < numPoints; ++j)
  {
    double point[3];
    polys->GetPoint(j,point);
    heights[j] = point[1];
    raxis[j] = std::sqrt(std::pow(point[0],2)+std::pow(point[2],2)); 
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
