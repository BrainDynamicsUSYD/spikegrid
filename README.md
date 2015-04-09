# spikegrid
John and Adams spiking conductance based model with STDP.

#Dependencies
1. OpenCV - used for visualization and some output - this can be disabled in the makefile, but many features will be lost
2. Newish version of GCC - see here for how to access it at physics https://github.com/BrainDynamicsUSYD/neurofield/wiki/Getting-GCC-in-Physics

Following the instructions for setting up GCC should also pull in a version of openCV that I have compiled.

Note: compiling openCV will also require CMake

Other Note: doing `CC=Clang CXX=clang make` will compile the code with clang (if installed).  This can be useful as clang will warn on different things to GCC. I would like to reduce the very large number of warnings this generates

#Documentation
Some developer documentation can be auto-generated from the code using Doxygen with the following command: `make docs`
