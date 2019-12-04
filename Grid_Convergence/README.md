Grid Convergence
------------------------------
This tool is used to perform grid convergence studies for 2-phase
flow simulations conducted with OpenFOAM's interFoam solver. Users
supply 3 runs on successively refined grids, along with the refinement
ratio between each grid and the field of interest. The field at the
interface is extracted on each grid and the observed order of accurcay
as well as Roach's grid convergence index (GCI) are reported. An overplot is
also generated to visually inspect agreement between successive runs.


## Build Instructions ##
### Build Dependencies ###
You will need to install at least the following dependencies:

* build-essential
* cmake
* VTK (versions 7+)
* python + matplotlib (version 2.7)

### Build GridConvergence ###
Once the dependencies are installed, clone the repository with
```
$ git clone --recurse-submodules git@github.com:snatesh/OpenFoamUtils.git
```
and execute the following:
```
$ cd OpenFoamUtils/Grid_Convergence
$ mkdir build && cd build
$ cmake .. 
$ make -j6 (replace 6 with number of cores you want to use)
```
This will create the executable `GridConvergence`. You can either copy it
to where it will be used, or add its absolute path to your PATH env variable. If cmake 
has problems finding libraries, try manually specifying paths to cmake via the curses-gui 
interface. This is done by calling 
```
$ ccmake .
```
from the `build` directory, and navigating through the options. Additional
compiler flags can also be set through this interface or the command line.

## Example Usage ##
Copy the executable `GridConvergence` to the parent directory of the coarse, medium and fine runs. In this example, the directories contains decomposed outputs from a parallel (40 cores) run of interFoam, i.e. `reconstructPar` has **not** been called. The time directories are located in `processor0`,`processor1`, ..., `processor39`. 
We execute something like the following:
```
$ touch coarse/case.foam
$ touch medium/case.foam
$ toach fine/case.foam
$ ./GridConvergence input.json
$ bash animate.sh
```
```json
{
  "Coarse Case": {
    "Name": "coarse/case.foam",
    "Type": "decomposed"
   },
  "Medium Case": {
    "Name": "medium/case.foam",
    "Type": "decomposed",
    "Refinement Ratio": 2
   },
  "Fine Case": {
    "Name": "fine/case.foam",
    "Type": "decomposed",
    "Refinement Ratio": 4
   },  
  "Times": {
    "start": 5,
    "stride": 5,
    "end": 400 
  },  
  "Contour": {
    "array": "alpha.water",
    "value": 0.5,
    "data": {
      "name": "p_rgh"
    }   
  },  
  "Plot": {
    "title": "convergence test",
    "width": 400,
    "height": 500
  }
}
```
