Creating a new development project using 980f's microcontroller libraries

The pieces are in repo cortexm.

In your source directory clone:

git clone https://github.com/980f/cortexm

Review the branches and pick one before you go much further. 
You may wish to create a new branch named for your project to protect against changes made by others, or just fork it.

In cortexm you will find this file and:
CMakeLists.txt.eg:  copy to your source directory, dropping the .eg. 
Inside of it name your project, identify your processor and follow other instructions therein.
I have started to put the cmake source file list in its own file named projname.cmake and include that in CMakeLists.txt as per the example file, but that is not yet necessary and may be a waste of time.


newproject.ld: copy to your source directory and rename for your project (match the name in CMakeLists.txt).
Edit it for the processor you are using, and any options particular to it such as whether to put the vector table in ram.


You will need a main.cpp (you can name it whatever you want as long as you list it in your sources) something like:

#include "core_cmInstr.h" //for MNE

#pragma ide diagnostic ignored "EndlessLoop"
int main() {
  while(true){
    MNE(WFE);
  }
}

EOF.

I may insist on the main file being called projname.cpp, so that you can have multiple projects in one directory with just a little CMakeLists.txt work.


If you don't like git submodules then write yourself a script like the following:

onclone.sh:

#run on clone of this repo, not (yet) using git submodules as they have undesirable limitations on branch maintenance.

#this branch was created for the CG2 project, so that we could get rid of ezcpp repo dependence
git clone -b cortexm --single-branch https://github.com/980f/safely

#our microcontroller support libs
git clone https://github.com/980f/cortexm


Build tools:
I use various IDE's. Obviously ones that support Cmake.

I get my compiler from arm and install it in a path that I then mention in the BINROOT variable in cortexm/cortexm4.make.
TODO: allow the project CMakeLists.txt set the path to the compiler, change the present hardcoded value to a default, or simply require the user to set a filesystem link from the project file to the bin directory!


End of instructions.
