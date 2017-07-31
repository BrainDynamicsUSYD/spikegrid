# spikegrid

#Dependencies
1. OpenCV - used for visualization and some output - this can be disabled in the makefile, but many features will be lost.
    OpenCV 2.X.X is best and is the version that most linux distros seem to be packaging.  OpenCV 3 can also be used, but some of the paths for the include files are different
2. Newish version of GCC - needs to support c++11 for c++ and gnu11 for C. See here for how to access it at physics https://github.com/BrainDynamicsUSYD/neurofield/wiki/Getting-GCC-in-Physics
3. inotify-tools - for `make watch` (build as soon as a file is changed)

Following the instructions for setting up GCC should also pull in a version of openCV that I have compiled.

Note: compiling openCV will also require CMake - but most systems will come with it preinstalled

Other Note: doing `CC=Clang CXX=clang make` will compile the code with clang (if installed).  This can be useful as clang will warn on different things to GCC. I would like to reduce the very large number of warnings this generates.  The clang build is secondary though and the primary compiler is still gcc.


#Documentation
Some developer documentation can be auto-generated from the code using Doxygen with the following command: `make docs`

other documentation is available in the wiki here: https://github.com/BrainDynamicsUSYD/spikegrid/wiki

#Long term code cleanup tasks:

Things that need to be done:

1. In some places, we might need to use `size_t` for things which are indexes in arrays.  There are at least a few places where we have assumed that `unsigned int` or `int` are sufficiently large.  For large grids with STDP / random connections, this may not actually be true.
2. Many of the parameters are not used and/or not currently working - there should be a real project to make sure we have a test case for each option and that it can be easily tested.
