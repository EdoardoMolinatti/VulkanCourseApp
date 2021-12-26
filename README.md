# VulkanCourseApp

Application built during the [Vulkan Course](https://www.udemy.com/course/learn-the-vulkan-api-with-cpp/) on [Udemy](https://www.udemy.com/).

## Prerequisites

- Install the [**Vulkan SDK**](https://vulkan.lunarg.com/sdk/home) by LunarG on your system
- Download and extract* [GLFW](https://www.glfw.org/download.html) (Open**GL** **F**rame**W**ork)<br />
  _Please ensure to extract the **64bit** version to the **CPP_LIBS** _path_ and rename the main folder to **GLFW**_<br />
  _Please ensure to extract the **32bit** version to the **CPP_LIBS** _path_ and rename the main folder to **GLFW32** (if needed)_
- Download and extract* [GLM](https://github.com/g-truc/glm/releases) (Open**GL** **M**athematics)<br />
  _Please ensure to extract it to the **CPP_LIBS** _path_ and rename the main folder to **GLM**_

\* That folder must be the path targeted by the **CPP_LIBS** _environment variable_ (e.g. "C:\\CppLibs")

### Alternative to _renaming_ extracted library folders

You can use [symbolic links](https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/mklink)
instead of renaming the main folders of the downloaded libraries.<br />
The important thing is that the _symbolic link names_ remain the same (**GLFW**, **GLFW32**, **GLM**) directly under
the **CPP_LIBS** folder, and of course they must point to the root folder of the corrresponding extracted library
(e.g. "GLM" => "glm_0.9.9.8").<br />
Example about creating a symbolic link with **`mklink`** (be sure to open a shell with
[**Administrator** privileges](https://allthings.how/how-to-open-windows-terminal-as-admin-on-windows-11/)):
```
C:\CppLibs> mklink /D GLM glm_0.9.9.8
```
_output: `symbolic link created for GLM <<===>> glm_0.9.9.8`_

## Environment Variables

You should setup on your system to have defined the following environment variables
(you can help yourself by installing [Rapid Environment Editor](https://www.rapidee.com/)):

- **VULKAN_SDK** - This one should already exist (it's been created by the **Vulkan SDK** installer),
  so just check that it's under your **system** _environment variables_<br />
  N.B.: you can also check that the **path** _doesn't end_ with a backslash ("\\")

- **CPP_LIBS** - This one should be created (under **system** or **user** _environemt variables_, your choice).<br />
  It must point to the path where you store generic libraries (e.g. "C:\\CppLibs").<br />
  N.B.: it's preferable that the **path** _doesn't end_ with a backslash ("\\")

- **BOOST_ROOT** - This one should be created (under **system** or **user** _environemt variables_, your choice).<br />
  It must point to the path where you extracted the Boost libraries (e.g. "C:\\CppLibs\\boost_1_78_0").<br />
  N.B.: it's preferable that the **path** _doesn't end_ with a backslash ("\\")
