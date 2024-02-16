# Encosy-Engine

This is the game engine I have been working on as part of my studies. I've used it to explore data oriented design and Entity-Component-System architechtural pattern.  
It is made using C++20 and uses Vulkan as the graphics API. It uses the C++20 modules. Which currently makes development harder, as Intellisense doesn't really want to co-operate with modules yet.  
I'm currently working on implementing more Vulkan graphics features and a graphical demo as part of my last projects studies. As my thesis I'm researching how to add multithreading capabilities to the engine.

## Getting started

### Requirements
1. Windows 10/11 
2. Visual Studio 2022
- Desktop Development with C++
- C++ Modules for v143 build tools
3. GPU with Resizable BAR enabled
- Project should still work without one as the allocated buffers are still small. But the rendering engine is built with the assumption that the CPU can address the whole GPU memory via Resizable BAR.


### Setting up the project
1. Clone the repository
2. Run following commands:
- git submodule init
- git submodule update
3. Build the project files using CMake. Always use "Build" as the name of the build folder and put it in the same folder as CMakeLists.txt
4. Select EncosyGame as startup project and build the binaries with Visual Studio


## Libraries used
Note: Most of the libraries are downloaded using git submodules. If a a library does not work try switching it to a release version specified below:

### assimp
https://github.com/assimp/assimp  
License: https://github.com/assimp/assimp/blob/master/LICENSE  
Version: 5.3.1

### fmt
https://github.com/fmtlib/fmt  
MIT License  
Version: 10.2.1

### glm
https://github.com/g-truc/glm  
MIT License  
Version: 1.0.0

### imgui
https://github.com/ocornut/imgui    
MIT License    
Version: 1.90.3

### SDL3
https://github.com/libsdl-org/SDL  
zlib License  
Version: Unspecified, SDL3 development version (main-branch)

### STB_image
https://github.com/nothings/stb  
MIT License / Public Domain  
Version: Correct header version included in the files.

### vk-bootstrap
https://github.com/charles-lunarg/vk-bootstrap  
MIT License  
Version: 1.3.277  

### Vulkan SDK 1.3
Download and install from https://vulkan.lunarg.com/  
- Choose Vulkan Memory Allocator from the install options  

### vma
Vulkan Memory Allocator  
Comes with the Vulkan SDK installer

## Special thanks
The best tutorial to Vulkan I found: https://vkguide.dev/
Many parts of the renderer are similar to this tutorial, but I have adapted them to work better in my engine.