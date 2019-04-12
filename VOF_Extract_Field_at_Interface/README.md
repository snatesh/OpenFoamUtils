VOF Extract Field at Interface
------------------------------
This tool is intdended for extracting position and field data from 
the interface of a 2-phase flow simulation conducted with OpenFOAM's
interFoam solver in serial or parallel (i.e. decomposed) settings.
It has been tested on Ubuntu 18.04.2 LTS (Bionic Beaver), but should
work on any linux system meeting the dependency requirements.

## Build Instructions ##
### Build Dependencies ###
You will need to install at least the following dependencies:

* build-essential
* cmake
* VTK (versions 7+)
* python + matplotlib (version 2.7)
* MPI
* mencoder

### Build ExtractInterface ###
Once the dependencies are installed, execute the following from the top 
of the source tree:
```
$ mkdir build && cd build
$ cmake ..
$ make -j6 (replace 6 with number of cores you want to use)
```
This will create the executable `ExtractAtInterface`. You can either copy it
to where it will be used, or add its absolute path to your PATH env variable

## Example Usage ##
Copy the executable `ExtractAtInterface` and the script `animate.sh` 
to your OpenFOAM case directory. This directory should contain outputs 
from a parallel run of interFoam and should be decomposed, i.e. `reconstructPar` 
has **not** been called. This is the case if the time directories are located in
`processor0`,`processor1` and so on. Then, execute something like the following:
```
$ touch case.foam
$ mpirun -np 6 ExtractAtInterface case.foam <start> <stride> <end> <interface_field> <contour_val> <data_on_contour>
$ bash animate.sh
```
The output will be an avi file `output.avi`, whose name can be changed by editing
line 12 in `animate.sh` before executing the script. For a concrete example, consider
```
$ mpirun -np 6 ExtractAtInterface case.foam 5 5 400 alpha.water 0.5 p_rgh
```
In this case, there were 80 time directories including 0, and they were 
written every 5 seconds for a total of 400 s simulation time. Each frame 
of the avi file is a plot of the y coordinate of the interface, defined as 
alpha.water=0.5, and another plot of p_rgh at the interface, starting at t = 5s.
See below for a sample frame: 
![Sample Frame](example_frame.png)



