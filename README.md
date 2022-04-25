# VulkanCourseApp

Application built during the [Vulkan Course](https://www.udemy.com/course/learn-the-vulkan-api-with-cpp/)
on [Udemy](https://www.udemy.com/).

## Prerequisites

1. Install the [**Vulkan SDK**](https://vulkan.lunarg.com/sdk/home) by [LunarG](https://www.lunarg.com/) on your system
   (whatever _path_ you choose is perfectly ok)
2. Download and extract* [GLFW](https://www.glfw.org/download.html) (Open**GL** **F**rame**W**ork)<br />
  _Please ensure to extract the **64bit** version to the "**Libraries**" path and rename the main folder to **GLFW**_<br />
  _Please ensure to extract the **32bit** version to the "**Libraries**" path and rename the main folder to **GLFW32**_
3. Download and extract* [GLM](https://github.com/g-truc/glm/releases) (Open**GL** **M**athematics)<br />
  _Please ensure to extract it to the "**Libraries**" _path_ and rename the main folder to **GLM**_
4. *Note:* this step is OPTIONAL (if you don't want to use the **Boost** libraries you shall provide alternative methods
   and/or comment the few lines using the Boost libraries).
   Download and extract the [Boost C++](https://www.boost.org/) libraries under the "**Libraries**" _path_ and rename the main folder to **BOOST_ROOT**.

\* That folder must be inside the main project folder, and it shall be called "**Libraries**".<br />
&nbsp;&nbsp;&nbsp;It will be searched with this relative _path_ (in VS project properties): "`$(SolutionDir)/Libraries/`"

### Using symbolic links (instead of _renaming_ the extracted library folders)

You can use [symbolic links](https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/mklink)
instead of renaming the main folders of the downloaded libraries.<br />
The important thing is that the _symbolic link names_ remain the same (**GLFW**, **GLFW32**, **GLM**) directly under
the "**Libraries**" folder.<br />
Of course each of them must point to the root folder of the corrresponding extracted library
(e.g. "GLM" => "glm_0.9.9.8").<br />
Example about creating a symbolic link with [**`mklink`**](https://ss64.com/nt/mklink.html) (be sure to open a shell
with [**Administrator** privileges](https://allthings.how/how-to-open-windows-terminal-as-admin-on-windows-11/)):
```
[...]\Libraries> mklink /D GLM glm_0.9.9.8
```
_output: `symbolic link created for GLM <<===>> glm_0.9.9.8`_<br />

## Environment Variables

You should verify or setup your system to have defined the following environment variables
(you can help yourself by installing [Rapid Environment Editor](https://www.rapidee.com/)):

- **VULKAN_SDK** - This one should already exist (it should be created by the **Vulkan SDK** installer).<br />
  So you can just check that it's present under your **system** _environment variables_
  (default: `VULKAN_SDK` => `C:\VulkanSDK\1.2.198.1`)<br />
  N.B.: please note that the **path** _doesn't end_ with a backslash ("\\")
