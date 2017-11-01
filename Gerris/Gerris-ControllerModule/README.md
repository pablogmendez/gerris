# Gerris - Controller Module

The following tutorial provides quick guidelines in how to install the Gerris Controller Module using the latest source code version. This module was not yet packaged so an old-school installation is required.

## Prerequisites

As first step, it is recommended to test your environment against the latest version of Gerris. In order to install it, you should download the source code and required dependencies as follows:
```bash
sudo apt-get install libglib2.0-dev libnetpbm10-dev m4 libproj-dev libgsl0-dev libnetcdf-dev libode-dev libfftw3-dev libhypre-dev libgtkglext1-dev libstartup-notification0-dev ffmpeg
darcs get http://gerris.dalembert.upmc.fr/darcs/gerris-stable
darcs get http://gerris.dalembert.upmc.fr/darcs/gts-stable
darcs get http://gerris.dalembert.upmc.fr/darcs/gfsview-stable
```

## Compiling Gerris

Before proceeding, you need to define the target installation folder. It is being done by providing a 'prefix' variable to the compile configuration scripts. Then, you can opt to use either:
- sh autogen.sh  #default installation folder and GCC options. We'll assume this option for the rest of the tutorial.
- sh autogen.sh --prefix=$HOME/local CFLAGS='-ggdb' #when a specific installation folder was choosed and/or a CFLAG needs to be used during compilation.

Also, the *autogen.sh* script needs to be used the first time in order to create *configure* and *Make* files.

Then, you must execute a sequence of autogen/make/make-install commands to compile the system:
```bash
cd gts-stable
sh autogen.sh && automake --add-missing
./configure
make && sudo make install
cd ../gerris-stable
sh autogen.sh && automake --add-missing
make && sudo make install
cd ../gfsview-stable
sh autogen.sh && automake --add-missing
make && sudo make install
sudo /sbin/ldconfig
```
Full instructions to install Gerris can be found at:
 - http://gfs.sourceforge.net/wiki/index.php/Installing_from_source

## Testing the installation

It is a good idea to test the Gerris installation now by using one of the online examples. This way you have a safe checkpoint to return in case anything fails. A comprehensive list of Gerris examples can be found at:
 - http://gerris.dalembert.upmc.fr/gerris/examples/examples/ 

## Compiling Controller Module

You should download the source code, compile and install it as in the original Gerris repository.

```bash
git clone https://github.com/pablodroca/Gerris-ControllerModule.git
cd Gerris-ControllerModule
cd gerris-stable
sh autogen.sh && automake --add-missing
make && sudo make install
```

Note:
In order to have debugging information for development purposes, gerris-stable could be compiled with different log levels. You can do it by defining the lower level of debugging during the *Make* file generation:
```bash
sh autogen.sh CFLAGS='-ggdb -g -Og -DG_DEBUG=\"debug\"' && automake --add-missing
make && sudo make install
```

## Testing the Controller

Before defining custom control experiences, it is a good idea to test the installation with the provided examples. So, jump into the Gerris-ControllerModule folder and execute the examples/cylinder_control simulation:
```bash
cd examples/cylinder_control
gerris2D -m cylinder_control.gfs
```

You should receive the simulation output with a basic PI controller which stabilizes the cylinder wake based in the online information from different sensors.
In order to understand the controller structure, consider the following summary:
 - **python/main.py:** Python subsystem's entry-point. Is the responsible to to launch synchronization tasks with Gerris and close the simulation properly.
 - **python/communication.py:** Handles the synchronization with Gerris by receiving sensors and answering with the user defined actuation values.
 - **python/samples.py:** Samples classes and structures. Defines the container structures to collect sensors (locations and forces) from Gerris and provides an easy to understand searching API
 - **python/user/controller.py:** Custom controlling script file. The user should define here the control strategy using the following interface:
```python
  def actuation(time, step, samples):
    act = 0
    #define the actuation value for the given time and step in the simulation
    return act
  def init(proc_index):
    #define custom actions to initialize the controller knowing the given number of processor in a MPI context
    pass
  def destroy(proc_index):
    #define custom actions to close the controller knowing the given number of processor in a MPI context
    pass
```
