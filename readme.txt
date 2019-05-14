// ----------------------------------------------------------------------------------------------------------------------------
// Welcome
// ----------------------------------------------------------------------------------------------------------------------------

This is a raytracing rendering application. It currently has CPU based and Realtime (via DXR) based raytracing.

The core CPU rendering code builds for Windows, Mac and Linux. The realtime raytracing is currently only supported under
Windows 10.

This project initially grew from working through the great "Raytracing in a Weekend" series by Peter Shirley. It
has grown beyond it. I have more planned, so please stay tuned.

// ----------------------------------------------------------------------------------------------------------------------------
// Credit Where Credit Is Due
// ----------------------------------------------------------------------------------------------------------------------------

As cliche as it sounds, this statement still holds true today: we stand upon the shoulders of giants. 

A great portion of this software was built around code from around the internet. I've shamelessly borrowed and modified 
tutorial and sample code from great developers from Microsoft, Stb and even former collegues of mine.

Most of the realtime core engine code was copied, stripped of features we didn't need and modified to match the style and
spirit of this project. I've included unadulterated copies of this software under the ThirdParty folder. Long story short,
if you see a similarity to something you've seen somewhere else, your hunches are correct: it's going to be code from
the gracious programmers aforementioned. So, feel free to use the code in this package and also kindly credit the following
individuals and/or organizations:

Microsoft
https://github.com/microsoft/DirectX-Graphics-Samples

Ray Tracing In A Weekend / Peter Shirley
https://github.com/petershirley/raytracinginoneweekend

Stb
https://github.com/nothings/stb

Vcl / Agner Fog
https://www.agner.org/optimize/vectorclass.pdf

Jeremiah
https://www.3dgep.com/learning-directx12-1/

IMGUI
https://github.com/ocornut/imgui

// ----------------------------------------------------------------------------------------------------------------------------
// Special Thanks
// ----------------------------------------------------------------------------------------------------------------------------

The following people were great influences and provided great learning material and knowledge through their 
works, books, posts and material. If any of you are reading this, thank you for sharing your knowledge and
for inspiring this project.

Peter Shirley
  His great book "Ray Tracing In A Weekend" started this whole shin-dig.
  
Matt Pharr, Wenzel Jakob
  Their magnificent and massive "Physically based rendering" book is a must read. I still have to finish my copy...

Eric Haines, Tomas Akenine-Möller
  I always keep a copy of "Real-time Rendering" book on my desk. The new "Raytracing Gems" is also a great read
  about the emerging realtime raytracing technology.
