# CPU and GPU Raytracer

This is a raytracing rendering application. It currently has CPU based and Realtime (via DXR) based raytracing. The core CPU rendering code builds for Windows, Mac and Linux. The realtime raytracing is currently only supported under Windows 10.

This project initially grew from working through the great "Raytracing in a Weekend" series by Peter Shirley.

## Features
* RGB based
* CPU: multi-core, SIMD accelerated
* GPU: DXR accelerated raytracing
* Area lighting
* Raytraced direct, indirect lighting (single bounce)
* Raytraced ambient occlusion
* Anti-aliasing (via camera jitter + temporal accumulation)

<br>
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/final.png "CPU traced image")
<br>
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/realtime.png "GPU traced image")

## Good reads

The following people were great influences and provided the learning material and knowledge through their works, books, etc. If any of you are reading this, thank you for sharing your knowledge and for inspiring this project.

**Peter Shirley**
<br>
*His great book "Ray Tracing In A Weekend" started this whole shin-dig.*
  
**Matt Pharr, Wenzel Jakob**
<br>
*Their magnificent and massive "Physically based rendering" book is a must read.*

**Eric Haines, Tomas Akenine-Möller**
<br>
*I always keep a copy of "Real-time Rendering" book on my desk. The new "Raytracing Gems" is also a good read on emerging realtime raytracing tech.*

## References & Credits

This was not created in isolation. A portion of this software was built around code from around the internet. I've shamelessly borrowed and modified tutorial and sample code from great developers from Microsoft, Nvidia and former collegues of mine.

Most of the realtime core engine code was copied, stripped of features I didn't need and modified to match the style and spirit of this project. Feel free to use the code in this package and kindly credit the following individuals and/or organizations as well:

**Ray Tracing In One Weekend / Peter Shirley**
<br>
[https://github.com/petershirley/raytracinginoneweekend]

**Chris Wyman**
<br>
[http://cwyman.org/code/dxrTutors/dxr_tutors.md.html]

**Microsoft**
<br>
[https://github.com/microsoft/DirectX-Graphics-Samples]

**Stb**
<br>
[https://github.com/nothings/stb]

**Vcl / Agner Fog**
<br>
[https://www.agner.org/optimize/vectorclass.pdf]

**Jeremiah**
<br>
[https://www.3dgep.com/learning-directx12-1/]

**IMGUI**
<br>
[https://github.com/ocornut/imgui]


