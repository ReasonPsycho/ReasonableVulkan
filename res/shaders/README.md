# Shaders

This folder contains the shaders used by the samples. Source files are available as GLSL and HLSL and also come with
precompiled SPIR-V files that are consumed by the samples. To recompile shaders you can use the `compileshaders.py`
scripts in the respective folders or any other means that can generate Vulkan SPIR-V from GLSL or HLSL. One such option
is [this extension for Visual Studio](https://github.com/SaschaWillems/SPIRV-VSExtension).