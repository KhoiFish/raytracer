// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_win32.h>
#include <imgui/examples/imgui_impl_dx12.h>
#include <stdint.h>
#include <inttypes.h>

// ----------------------------------------------------------------------------------------------------------------------------

static int      gNumberOfLoadDots = 0;
static ImVec4   gTextHeadingColor(0.621f, 0.351f, 0.988f, 1.0f);

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGui()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("RuntimeData/Fonts/RobotoMono-Regular.ttf", 20.0f);
    io.Fonts->Build();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGui()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
    switch (TheAppState)
    {
        case AppState_Loading:
            RenderGuiLoadingScreen();
            break;

        case AppState_RenderScene:
            gNumberOfLoadDots = 0;
            RenderGuiOptionsWindow();
            break;
    }
    ImGui::EndFrame();

    // Setup context and render
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderGUI");
    {
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());
        renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
        renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RenderDevice::Get().GetImguiDescriptorHeapStack().GetDescriptorHeap());

        if (TheAppState == AppState_Loading)
        {
            renderContext.ClearColor(RenderDevice::Get().GetRenderTarget());
        }

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderContext.GetCommandList());
    }
    renderContext.Finish();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGuiOptionsWindow()
{
    ImGui::SetNextWindowPos(ImVec2(12, 18), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(615, 735), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
    if (!ImGui::Begin("Options Window", nullptr, windowFlags))
    {
        ImGui::End();
        return;
    }

    // ------------------------------------------------------------

    if (TheRaytracer != nullptr && TheRaytracer->IsTracing())
    {
        ImGui::Separator();
        ImGui::Separator();
        ImGui::TextColored(gTextHeadingColor, "Cpu Raytrace Info");
        ImGui::Separator();
        {
            Raytracer::Stats stats = TheRaytracer->GetStats();
            double percentage = double(double(stats.NumPixelSamples) / double(stats.TotalNumPixelSamples));
            int    percentInt = (int)(percentage * 100);
            int    numMinutes = stats.TotalTimeInSeconds / 60;
            int    numSeconds = stats.TotalTimeInSeconds % 60;

            ImGui::BulletText("Completion: %d%%", percentInt);
            ImGui::BulletText("Time: %dm:%ds", numMinutes, numSeconds);
            ImGui::BulletText("Resolution: %dx%d", Width, Height);
            ImGui::BulletText("Scatter Depth: %d", TheUserInputData.CpuMaxScatterDepth);
            ImGui::BulletText("Num Threads: %d", TheUserInputData.CpuNumThreads);
            ImGui::BulletText("Num Samples: %d", TheUserInputData.CpuNumSamplesPerRay);
            ImGui::BulletText("Done Samples: %d", stats.CompletedSampleCount);
            ImGui::BulletText("Rays Fired: %" PRId64 "", stats.TotalRaysFired);
            ImGui::BulletText("Num Pixels Sampled: %" PRId64 "", stats.NumPixelSamples);
            ImGui::BulletText("Pdf Query Retries: %d", stats.NumPdfQueryRetries);
        }

        ImGui::Text("");
        if (ImGui::Button("Stop Cpu Raytrace", ImVec2(180, 30)))
        {
            SetEnableCpuRaytrace(false);
        }
    }
    else
    {
        char stringBuf[128];
        bool cpuOptionsChanged = false;
        bool gpuOptionsChanged = false;

        // ------------------------------------------------------------

        ImGui::Separator();
        ImGui::Separator();
        ImGui::TextColored(gTextHeadingColor, "Input Legend");
        ImGui::Separator();
        ImGui::BulletText("Translate Camera:[WASDQE]  Pan Camera:[hold left mouse btn]");
        ImGui::BulletText("Cpu Raytrace:[SPACE]  Select Output:[1 - 9 keys]");

        // ------------------------------------------------------------

        ImGui::Separator();
        ImGui::Separator();
        ImGui::TextColored(gTextHeadingColor, "Global Options");
        ImGui::Separator();
        if (ImGui::ListBox("Scene Select", &TheUserInputData.SampleScene, SampleSceneNames, IM_ARRAYSIZE(SampleSceneNames), MaxScene))
        {
            LoadSceneRequested = true;
        }

        // ------------------------------------------------------------

        ImGui::Separator();
        ImGui::Separator();
        ImGui::TextColored(gTextHeadingColor, "Cpu Raytracer");
        ImGui::Separator();

        _itoa_s(TheUserInputData.CpuNumSamplesPerRay, stringBuf, 10);
        if (ImGui::InputText("Cpu Rays/Pixel", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
        {
            TheUserInputData.CpuNumSamplesPerRay = atoi(stringBuf);
            cpuOptionsChanged = true;
        }

        _itoa_s(TheUserInputData.CpuMaxScatterDepth, stringBuf, 10);
        if (ImGui::InputText("Cpu Ray Depth", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
        {
            TheUserInputData.CpuMaxScatterDepth = atoi(stringBuf);
            cpuOptionsChanged = true;
        }

        if (ImGui::SliderInt("Cpu Num Threads", &TheUserInputData.CpuNumThreads, 1, MaxNumCpuThreads))
        {
            cpuOptionsChanged = true;
        }

        ImGui::Text("");
        if (ImGui::Button("Begin Cpu Raytrace", ImVec2(180, 30)))
        {
            SetEnableCpuRaytrace(true);
        }

        // ------------------------------------------------------------

        ImGui::Separator();
        ImGui::Separator();
        ImGui::TextColored(gTextHeadingColor, "Gpu Raytracer");
        ImGui::Separator();


        if (ImGui::SliderInt("Accum. Passes/Frame", &TheUserInputData.GpuNumAccumPasses, 1, 10))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderInt("Rays Per Pixel", &TheUserInputData.GpuNumRaysPerPixel, 1, 10))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderInt("Ray Recursion Depth", &TheUserInputData.GpuRayRecursionDepth, 1, RAYTRACING_MAX_RAY_RECURSION_DEPTH))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Direct Light Scale", &TheUserInputData.GpuDirectLightMult, 0.0f, 10.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Indirect Light Scale", &TheUserInputData.GpuIndirectLightMult, 0.0f, 10.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("AO Radius", &TheUserInputData.GpuAORadius, 1.0f, 1000.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Denoise Alpha", &TheUserInputData.GpuDenoiseAlpha, 0.0f, 1.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Denoise Moments Alpha", &TheUserInputData.GpuDenoiseMomentsAlpha, 0.0f, 1.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Denoise Phi Color", &TheUserInputData.GpuDenoisePhiColor, 0.0f, 250.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::SliderFloat("Denoise Phi Normal", &TheUserInputData.GpuDenoisePhiNormal, 0.0f, 250.0f))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::Checkbox("Enable Camera Jitter", &TheUserInputData.GpuCameraJitter))
        {
            gpuOptionsChanged = true;
        }

        if (ImGui::Checkbox("Enable Tone Mapping", &TheUserInputData.GpuEnableToneMapping))
        {
            // This does not require flagging gpu as dirty
            //gpuOptionsChanged = true;
        }

        // ------------------------------------------------------------

        // If gpu options changed, trigger camera to update
        if (gpuOptionsChanged)
        {
            SetCameraDirty();
        }
    }

    ImGui::End();
}


// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGuiLoadingScreen()
{
    static char  buf[64];
    static float timeElapsed = 0;

    ImGui::SetNextWindowPosCenter();
    ImGui::SetNextWindowSize(ImVec2(200, 35));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration;
    if (!ImGui::Begin("Loading", nullptr, windowFlags))
    {
        ImGui::End();
        return;
    }

    timeElapsed += CurrentDeltaTime;
    if (timeElapsed > 0.20f)
    {
        gNumberOfLoadDots = (gNumberOfLoadDots + 1) % 10;
        timeElapsed = 0.0f;
    }

    sprintf_s(buf, "Loading");
    for (int i = 0; i < gNumberOfLoadDots; i++)
    {
        strcat_s(buf, ".");
    }

    ImGui::TextColored(gTextHeadingColor, buf);

    ImGui::End();
}

