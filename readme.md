# Realtime GPU Raytracer and "Slowtime" CPU Raytracer
This is a raytracing rendering application. It currently has CPU based and GPU based realtime (via DXR) raytracing. The core CPU rendering code builds for Windows, Mac and Linux. The GPU realtime raytracing is currently only supported under Windows 10. This project initially grew from working through the great "Raytracing In One Weekend" series by Peter Shirley.
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/final.png "CPU traced image")
<br>

## Features
* RGB based
* CPU: multi-core, SIMD accelerated
* GPU: DXR accelerated raytracing
* Area lighting
* Raytraced direct, indirect lighting (single bounce)
* Raytraced ambient occlusion
* Anti-aliasing (via camera jitter + temporal accumulation)
<br>

![alt text](https://github.com/KhoiFish/raytracer/blob/master/SavedImages/realtime.png "GPU traced image")
<br>

## References & Credits

This was not created in isolation. A portion of this software was built using code and documentation from around the internet for reference. Feel free to use the code in this package and kindly give credit to the following individuals and/or organizations, as well. Thank you!

### Reference Literature

The following are really good references for someone getting into raytracing. If any of the following people are reading this, thank you for sharing your knowledge and giving inspiration to this project.

**Peter Shirley**
<br>
*His great book "Ray Tracing In A Weekend" started this whole shin-dig.*
  
**Matt Pharr, Wenzel Jakob**
<br>
*Their magnificent and massive "Physically based rendering" book is a must read.*

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


