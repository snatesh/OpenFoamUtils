#ifndef CONTOURINTERFACE_H
#define CONTOURINTERFACE_H
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
#include<memory>

/* This class contours the solution array "contourArray" by "contour_val" and
 * extracts the height and requested data on the contour. */
class ContourInterface
{
  public:
    // ctor with case name, type, array by which we contour and names of 
    // data arrays on contour
    ContourInterface(const std::string& caseName, int caseType, 
                     const std::string& contourArray, double contour_val,
                     std::vector<std::string>& dataNames);
    
    // static method ctor for RAII handling 
    static std::unique_ptr<ContourInterface> 
      Create(const std::string& caseName, int caseType, 
             const std::string& contourArray, double contour_val, 
             std::vector<std::string>& dataNames);
    // initialize the OpenFOAM reader (checks if requests are valid)
    void initReader();

    ~ContourInterface(){};

    // contour by contour_val, put y coordinates into heights, 
    // and x coordinates into xaxis
    void getContour(std::vector<double>& heights, std::vector<double>& xaxis);
    // contour by counter_val, put y coordinates into heights,
    // sqrt(x^2+z^2) coordinates into raxis
    void getRadialContour(std::vector<double>& heights, std::vector<double>& raxis);    
    
    // contour by contour_val, put y coords into heights, 
    // data with name dataName into datas and x coords into xaxis
    void getContour(std::vector<double>& heights, std::vector<double>& datas, 
                    std::vector<double>& xaxis);
    
    // contour by contour_val, put y coords into heights, 
    // data with names dataName into datas and x coords into xaxis
    // map is used to keep axes and data sorted
    void getContour(std::vector<double>& heights, std::vector<std::vector<double>>& datas, 
                    std::vector<double>& xaxis);
    // step reader and contour filter to t=time
    void stepTo(double time);

  private:
    // OF reader
    vtkSmartPointer<vtkPOpenFOAMReader> reader;
    // contour backend
    vtkSmartPointer<vtkContourFilter> contourFilter;
    // OF caes name
    std::string caseName;
    // OF case type, specifid as reconstructed|decomposed in input
    int caseType;
    // contour using this array
    std::string contourArray;
    // contour by this value
    double contour_val;
    // get data with these names from contour
    std::vector<std::string>& dataNames;
};
#endif
