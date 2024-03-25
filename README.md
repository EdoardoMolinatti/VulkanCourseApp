# VulkanCourseApp

Application built during the [Vulkan Course](https://www.udemy.com/course/learn-the-vulkan-api-with-cpp/)
on [Udemy](https://www.udemy.com/).

## Prerequisites

1. Install the [**Vulkan SDK**](https://vulkan.lunarg.com/sdk/home) by [LunarG](https://www.lunarg.com/) on your system
   (whatever _installation path_ you choose is perfectly ok)
2. Download and extract\* [GLFW](https://www.glfw.org/download.html) (Open**GL** **F**rame**W**ork)  
   _Please ensure to extract the **64bit** version to the "**Libraries**" path and rename the main folder to **GLFW**_  
   _Please ensure to extract the **32bit** version to the "**Libraries**" path and rename the main folder to **GLFW32**_
3. _OPTIONAL_  Download and extract\* the [Boost C++](https://www.boost.org/) libraries under the "**Libraries**" _path_ and
   rename the main folder to **BOOST_ROOT**  
   _(if you don't want to use the **Boost** libraries you shall provide alternative methods
   and/or comment the few lines of code that uses the Boost libraries in the `VulkanValidation` class)_

\* The target folder must be inside the main project folder, and it shall be called "**Libraries**".  
&nbsp;&nbsp;&nbsp;It will be searched with this relative _path_ (in VS project properties): "`$(SolutionDir)/Libraries/`"

> **NOTE:**  The project uses also the [GLM](https://github.com/g-truc/glm) library, but it takes it from the Vulkan SDK installation path (in "`$(VULKAN_SDK)/Include/glm`").

### Using symbolic links (instead of _renaming_ the extracted library folders)

You can use [symbolic links](https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/mklink)
instead of renaming the main folders of the downloaded libraries.  
The important thing is that the _symbolic link names_ remain the same (**GLFW**, **GLFW32**) directly under
the "**Libraries**" folder.  
Of course each of them must point to the root folder of the corrresponding extracted library
(e.g. "GLFW" => "glfw-3.3.8.bin.WIN64").  
Example about creating a symbolic link with [**`mklink`**](https://ss64.com/nt/mklink.html) (be sure to open a shell
with [**Administrator** privileges](https://allthings.how/how-to-open-windows-terminal-as-admin-on-windows-11/)):

```cmd
[...]\Libraries> mklink /D GLFW glfw-3.3.8.bin.WIN64
```

_output: `symbolic link created for GLFW <<===>> glfw-3.3.8.bin.WIN64`_  

## Environment Variables

You should verify to have defined the following environment variable
(you can help yourself by installing [Rapid Environment Editor](https://www.rapidee.com/)):

- **VULKAN_SDK** - This should already exist (it should be created by the **Vulkan SDK** installer).  
  So you can just check that it's present under your **system** _environment variables_
  (default: `VULKAN_SDK` => `C:\VulkanSDK\1.3.280.0`) and it points to the correct _installation path_ (the one you chose when installing the Vulkan SDK).

  > Please note that your VulkanSDK version might be more recent.  
  Also note that the `VULKAN_SDK` **path** _doesn't end_ with a backslash ("\\").
