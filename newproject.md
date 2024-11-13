Creating a new development project using 980f's microcontroller libraries

The pieces are in repo cortexm.

In your source directory clone:

`git clone https://github.com/980f/cortexm`

Review the branches and pick one before you go much further. 
You may wish to create a new branch named for your project to protect against changes made by others, or just fork it.

If you don't like git submodules then write yourself a script like the following:

`onclone.sh`:

#run on clone of this repo, not (yet) using git submodules as they have undesirable limitations on branch maintenance.

#this branch of the `safely` repo was created for a particular project, so that we could get rid of `ezcpp` repo dependence using `safely/cppext `instead since that was being used for the host which was talking to the mcu using this library. 
`git clone -b cortexm --single-branch https://github.com/980f/safely`
You may wish to branch this repo so that you can diddle with it and then ask to merge your changes into the master repo.

----
In `cortexm` you will find this file (presently named `newproject.md`) and `CMakeLists.txt.eg` the latter which you copy to your source directory, dropping the `.eg`. 
Inside of it name your project, identify your processor via including the cmake file for it from those in each vendor folder under `cortexm` such as `cortexm/stm32/f103.cmake` and follow other instructions therein.
I have started to put the cmake source file list in its own file named `_projname_.cmake` and include that in `CMakeLists.txt` as per the example file, but that is not yet necessary and may be a waste of time.

`newproject.ld`: copy to your source directory and rename for your project (match the name in `CMakeLists.txt`).
Edit it for the processor you are using, and any options particular to it such as whether to put the vector table in ram.

Your main must be in a file named `_projname_.cpp`, so that you can have multiple projects in one directory with just a little `CMakeLists.txt` work.

#include "core_cmInstr.h" //for MNE 
#pragma ide diagnostic ignored "EndlessLoop"   
int main() {    
    while(true){  
        MNE(WFE);  
    }  
}

___

The calling hierarchy of cmake files is:  
* your `CmakeLists.txt` which must set `CortexmVendor` and `VendorPartname` which are used for include directories and some chip specific files, this cmake file then includes
* * `cortexm/` _CortexmVendor_ / _VendorPartname_ `.cmake` vendor/device specific cmake file which includes
* * * core specific cmake file in `cortexm` such as `cortexm/cortex-cm3.cmake` which includes
* * * * `cortexm-all.cmake` which arranges to later call `mkIrqs` which creates
* * * * * `nvicTable.inc` which is referenced by `cortexm/nvic.cpp`
* * `cortexm/postamble.cmake` which has the actual build invoking steps, and references 
* * * *progname*`/.ld` which allows you to tweak your link.

To be clear: you only need to edit CmakeLists.txt and often *progname*`/.ld` 

---
Build tools:
I use various IDE's. Obviously ones that support Cmake.

I get my compiler from Arm and install it in a path that I then mention in the `BINROOT` variable in `cortexm/cortex-all.cmake`.

TODO: allow the project `CMakeLists.txt` to set the path to the compiler, change the present hardcoded value to a default, or simply require the user to set a filesystem link from the project file to the bin directory!


End of instructions.
