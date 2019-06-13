#include<ContourInterface.H>
#include<matplotlibcpp.h>
#include<jsoncons/json.hpp>

namespace plt = matplotlibcpp;

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

// input args class
struct Args 
{
  std::string caseName;
  int caseType;
  int dimension;
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
  std::string title;
  int width;
  int height;
	bool plot;
	bool smooth;
};

// construct Args class from input json file
Args readJSON(jsoncons::json inputjson)
{
  Args args;
  // required arguments
  args.caseName = inputjson["Case Name"].as<std::string>();
  std::string casetype = inputjson["Case Type"].as<std::string>();
  args.dimension = inputjson["Dimension"].as<int>();
  if (args.dimension != 2 && args.dimension != 3)
  {
    std::cerr << "Dimension must be 2 or 3" << std::endl;
    exit(1);
  }
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
  args.write = inputjson["Write"].as<bool>();
	
	args.smooth = (args.dimension == 2 ? 0 : inputjson["Smoothing"].as<bool>());
	args.plot = false; 
  if (inputjson.has_key("Plot"))
	{
		args.plot = true;
		args.title = inputjson["Plot"]["title"].as<std::string>();
  	args.width = inputjson["Plot"]["width"].as<int>();
  	args.height = inputjson["Plot"]["height"].as<int>();
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
	}
  if (contouropt.has_key("data"))
  {
    args.dataOnContour = contouropt["data"]["name"].as<std::string>();
    if (args.plot)
		{
			if (contouropt["data"].has_key("ymin"))
    	  args.dymin = contouropt["data"]["ymin"].as<double>();
    	else
    	  args.dymin = INFINITY;
    	if (contouropt["data"].has_key("ymax"))
    	  args.dymax = contouropt["data"]["ymax"].as<double>();
    	else
    	  args.dymax = INFINITY;
  	}
	}
  else
    args.dataOnContour = "";

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

  std::vector<std::string> dataName(1); dataName[0] = args.dataOnContour;
  std::unique_ptr<ContourInterface> contour = 
    ContourInterface::Create(args.caseName,args.caseType,args.contourArray,args.contour_val,dataName);

  double nT = (args.end-args.beg)/args.stride;
  if (args.plot)
	{
		plt::figure();
  	plt::figure_size(args.width,args.height);
  }
	for (int i = 0; i <= nT; ++i)
  {
  	double time = i*args.stride+args.beg;
		std::cout << "Countouring at Time = " << time << std::endl;
    contour->stepTo(time);
    std::string figName;
		if (args.plot)
		{
			std::stringstream ss; 
    	ss << "Time" << std::internal << setw(5) << setfill('0') << time << ".png"; 
    	figName = trim_fname(args.caseName,ss.str()); 
    }
		std::vector<double> heights, smoothHeights, contourData, axis, smoothAxis;
    if (!args.dataOnContour.empty())
		{
      contour->getContour(heights,contourData,axis);
			if (args.plot)
			{
		  	// define names for plt legend
      	plt::clf();
      	plt::subplot(2,1,1);
      	plt::named_plot("height (m)", axis, heights,"b-");
      	if (!(isinf(args.xmin)  || isinf(args.xmax)))
      	  plt::xlim(args.xmin,args.xmax);
      	if (!(isinf(args.ymin) || isinf(args.ymax)))
      	  plt::ylim(args.ymin,args.ymax);
      	plt::legend();
      	std::stringstream ss; 
      	ss << args.title << ", Contour by "<<args.contourArray<<"="<<
      	      args.contour_val<<", Time: "<<time;
      	std::string titleText(ss.str());
      	plt::title(titleText);
      	plt::grid(true);
      	plt::subplot(2,1,2);
      	plt::named_plot(args.dataOnContour, axis, contourData, "k-");
      	if (!(isinf(args.xmin)  || isinf(args.xmax)))
      	  plt::xlim(args.xmin,args.xmax);
      	if (!(isinf(args.dymin) || isinf(args.dymax)))
      	  plt::ylim(args.dymin,args.dymax);
      	plt::grid(true);  
      	plt::xlabel("x (m)");
      	plt::legend();
      	plt::save(figName);
     		// plt::pause(0.0001);
    	}
		}
		else
 	 	{
			std::string marker;
 	   	if (args.dimension == 2)
 	   	{
 	   	  contour->getContour(heights, axis);
 	   	  marker = "b.-";
 	   	}
 	   	else
 	   	{
				if (!args.smooth)
 	   			contour->getRadialContour(heights,axis);
				else
				{
					contour->getSmoothRadialContour(heights,smoothHeights,axis,smoothAxis);
				}
 	   	  marker = "b.";
 	   	}
			if (args.plot)
			{
 	 			plt::clf();
 	  	 	plt::subplot(2,1,1);
 	 			plt::named_plot("height (m)", axis, heights,marker);
 	  	 	if (!(isinf(args.xmin) || isinf(args.xmax)))
 	  	 	  plt::xlim(args.xmin,args.xmax);
 	  	 	if (!(isinf(args.ymin) || isinf(args.ymax)))
 	  	 	  plt::ylim(args.ymin,args.ymax);
 	  	 	plt::legend();
 	  	 	std::stringstream ss; 
 	  	 	ss << args.title << ", Contour by "<< args.contourArray <<"=" <<
 	  	 	      args.contour_val<<", Time: "<< time;
 	  	 	std::string titleText(ss.str());
 	  	 	plt::title(titleText);
 	  	 	plt::grid(true);
 	  	 	plt::subplot(2,1,2);
 	 			plt::named_plot("height (m)", axis, heights,marker);
 	 			if (!(isinf(args.xmin) || isinf(args.xmax)))
 	  	 	  plt::xlim(args.xmin,args.xmax);
 	  	 	plt::ylim(-500,500);
 	  	 	plt::legend();
 	  	 	args.dimension == 2 ? plt::xlabel("x (m)") : plt::xlabel("r (m)");
 	  	 	plt::save(figName);
 	  	 	//plt::pause(1000.);
 	 		}
 	 	}
		if (args.write)
 	 	{
 	   	std::stringstream ss;
 	   	ss << "OpenFoamframeNo" << std::internal << setw(5) << setfill('0') <<  i+1 << ".txt";
			std::ofstream outputStream(ss.str());
 	   	if (!outputStream.good())
 	   	{
 	   	  std::cerr << "Error creating file " << ss.str() << std::endl;
 	   	  exit(1);
 	   	}
 	   	for (int i = 0; i  < heights.size(); ++i)
 	   	{
 	   	  outputStream << axis[i] << " ";
 	   	  outputStream << heights[i] << std::endl;
 	   	}
 	   	outputStream.close();
 	   	if (args.smooth)
			{
				ss.str("");
				ss << "OpenFoamSmoothFrameNo" << std::internal << setw(5) << setfill('0') <<  i+1 << ".txt";
				outputStream.open(ss.str());
 	   		if (!outputStream.good())
 	   		{
 	   		  std::cerr << "Error creating file " << ss.str() << std::endl;
 	   		  exit(1);
 	   		}

				for (int i = 0; i < smoothHeights.size(); ++i)
				{
					outputStream << smoothAxis[i] << " ";
					outputStream << smoothHeights[i] << std::endl;
				}
			}
		}
	}
  return 0;
}
