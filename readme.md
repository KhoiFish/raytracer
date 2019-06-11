# Realtime GPU Raytracer with Reference CPU Raytracer
This is (yet another) raytracing rendering application. It currently has CPU based and GPU based realtime (via DXR) raytracing.

The core CPU rendering code builds for Windows, Mac and Linux. The GPU realtime raytracing is currently only supported under Windows 10. 

This project initially grew from working through the great "Raytracing In One Weekend" series by Peter Shirley. This projects serves as a sample implementation of raytracing that ties the different compututation approaches of raytracing: CPU, GPU compute (CUDA) and GPU realtime (DXR). I hope this helps you in some way.
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/final.png "CPU traced image")
<br>

## Features
* RGB based
* CPU: multi-core, SIMD accelerated
* GPU: DXR accelerated
* Area lighting, soft shadows
* Multi-bounce GI
* Denoising (SVGF)
* Materials (lambert, metal, dielectric)
<br>

## Requirements
* Windows 10 with October 2018 Update (or later)
* DX12 Compatatible card, at least Pascal-based, Turing-based recommended
* Visual Studio 2019 (should compile in VS 2017 as well)
* Current Windows SDK 10 (10.0.18362.0 is the current latest)
<br>

## How To Build
There is a solution file,"RayTracer.sln" at the root of the project. Build "RayTracerWindows" for realtime raytracer. RayTracerConsole is a console application that runs CPU raytracing exclusively.

## Screenshots
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/realtime1.png "GPU traced image")
![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/realtime2.png "GPU traced image")
![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/realtime3.png "GPU traced image")

<br>

## References & Credits

This was not created in isolation. A portion of this software was built using code and documentation from around the internet for reference. Feel free to use the code in this package and kindly give credit to the following individuals and/or organizations, as well. Thank you!

### Reference Literature

The following are really good references for someone getting into raytracing. If any of the following people are reading this, thank you for sharing your knowledge and giving inspiration to this project.

**Peter Shirley**
<br>
*His great series of articles, "Ray Tracing In A Weekend" started this whole shin-dig.*
  
**Matt Pharr, Wenzel Jakob**
<br>
*The massive "Physically based rendering" book is a must read.*

**Eric Haines, Tomas Akenine-Möller**
<br>
*I always keep a copy of "Real-time Rendering" book on my desk. The new "Raytracing Gems" is also a good read on emerging realtime raytracing tech.*

### Reference Code
**Ray Tracing In One Weekend / Peter Shirley**
<br>
[https://github.com/petershirley/raytracinginoneweekend]

**Chris Wyman**
<br>
[http://cwyman.org/code/dxrTutors/dxr_tutors.md.html]

**Microsoft**
<br>
[https://github.com/microsoft/DirectX-Graphics-Samples]

**SVGF Denoiser**
<br>
[https://research.nvidia.com/publication/2017-07_Spatiotemporal-Variance-Guided-Filtering%3A]

**Stb**
<br>
[https://github.com/nothings/stb]

**Vcl / Agner Fog**
<br>
[https://www.agner.org/optimize/vectorclass.pdf]

**IMGUI**
<br>
[https://github.com/ocornut/imgui]


