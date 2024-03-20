# Encosy-Engine

This is the game engine I have been working on as part of my studies. I've used it to explore data oriented design and Entity-Component-System architechtural pattern. It is made using C++23 and uses Vulkan as the graphics API. It uses the modules introduced in C++20. This currently makes development harder, as Intellisense doesn't really want to co-operate with modules yet.  
I'm currently working on implementing more Vulkan graphics features and a graphical demo as part of my last projects studies. As my thesis I'm researching how to add multithreading capabilities to the engine. Most of the multithreading capabilites for the thesis are in place. There may be some tweaks and performance updates coming. After my thesis is done I will add a special branch for the code used in thesis writing.  

YouTube-playlist showcasing the project:  
https://www.youtube.com/playlist?list=PL7UiajzfESH9aFZna6MW-wU_mE83rhjxj

## Getting started

### Requirements
Due to usage of C++23 features, the project requires new tools to even compile.

1. Windows 10/11 
2. Visual Studio 2022 (17.9.2 tested to work)
- Desktop Development with C++
- C++ Modules for v143 build tools
3. GPU with Resizable BAR enabled
- Project should still work without the feature as the allocated buffers are small. But the rendering engine is currently built with the assumption that the CPU can address the whole GPU memory via Resizable BAR.
4. Vulkan SDK 1.3.275 or newer. You have to install vma with the SDK installer!. 
5. CMake 3.28 or newer.


### Setting up the project
1. Clone the repository
2. Run following commands:
- git submodule init
- git submodule update
3. Build the project files using CMake
4. Select EncosyGame as startup project and build the binaries with Visual Studio


### Demo controls
Currently there are 4 demo scenes. You can switch between them using the ChoseScene-enum in Game/Core/main.cppm. The file also default to 1920x1080 window. Depending on your screen size that might need to be modified. The engine has also been hardcoded to use maximum window size of 2560x1440. This will be changed in the future.
RotationTest: By default spawns 8 000 entities and rotates them.   
StaticTest: By default spawns 8 000 static entities.   
CollisionDemo: Spawns spheres that follow invisible targets. All spheres have colliders.  
DynamicDemo: Same as CollisionDemo but if a sphere collides with a sphere with different texture there's a chance both of them are killed.  

WASD: Move around.  
Q/E: Move up/down.  
Right mouse button: Look around.  

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
Version: 1.3.275  

### Vulkan SDK 1.3
Download and install from https://vulkan.lunarg.com/  
- Choose Vulkan Memory Allocator from the install options  

### vma
Vulkan Memory Allocator  
MIT License
Comes with the Vulkan SDK installer

## Special thanks
The best tutorial to Vulkan I found: https://vkguide.dev/
Some helper classes and builders are copied from the guide, but I have adapted them to work better in my engine. Substantial portions have been marked.