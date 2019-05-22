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

void Renderer::SetupGui()
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGui()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    while(true)
    {
        ImGui::SetNextWindowPos(ImVec2(12, 18), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(576, 608), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
        if (!ImGui::Begin("Options Window", nullptr, window_flags))
        {
            ImGui::End();
            break;
        }

        // ------------------------------------------------------------

        ImGui::Separator();
        ImGui::Text("INFO");
        ImGui::Separator();
        ImGui::BulletText("Displaying Buffer: %s", GetSelectedBufferName());

        if (TheRaytracer != nullptr && TheRaytracer->IsTracing())
        {
            {
                Raytracer::Stats stats = TheRaytracer->GetStats();
                double percentage = double(double(stats.NumPixelSamples) / double(stats.TotalNumPixelSamples));
                int    percentInt = (int)(percentage * 100);
                int    numMinutes = stats.TotalTimeInSeconds / 60;
                int    numSeconds = stats.TotalTimeInSeconds % 60;

                ImGui::BulletText("Completion: %d%%", percentInt);
                ImGui::BulletText("Time: %dm:%ds", numMinutes, numSeconds);
                ImGui::BulletText("Resolution: %dx%d", Width, Height);
                ImGui::BulletText("Scatter Depth: %d", UserInput.CpuMaxScatterDepth);
                ImGui::BulletText("Num Threads: %d", UserInput.CpuNumThreads);
                ImGui::BulletText("Num Samples: %d", UserInput.CpuNumSamplesPerRay);
                ImGui::BulletText("Done Samples: %d", stats.CompletedSampleCount);
                ImGui::BulletText("Rays Fired: %" PRId64 "", stats.TotalRaysFired);
                ImGui::BulletText("Num Pixels Sampled: %" PRId64 "", stats.NumPixelSamples);
                ImGui::BulletText("Pdf Query Retries: %d", stats.NumPdfQueryRetries);
            }

            ImGui::Text("");
            if (ImGui::Button("Stop Cpu Raytrace", ImVec2(140, 30)))
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
            ImGui::Text("HELP");
            ImGui::Separator();
            ImGui::BulletText("Translate camera:     Press keys WASDQE");
            ImGui::BulletText("Pan camera:           Hold Left Mouse Button");
            ImGui::BulletText("Toggle Cpu Raytrace:  Space bar");
            ImGui::BulletText("Select output:        1 - 9 keys");

            // ------------------------------------------------------------

            ImGui::Separator();
            ImGui::Text("GLOBAL OPTIONS");
            ImGui::Separator();
            if (ImGui::ListBox("Scene Select", &UserInput.SampleScene, SampleSceneNames, IM_ARRAYSIZE(SampleSceneNames), MaxScene))
            {
                LoadSceneRequested = true;
            }

            // ------------------------------------------------------------

            ImGui::Separator();
            ImGui::Text("CPU RAYTRACE OPTIONS");
            ImGui::Separator();

            _itoa_s(UserInput.CpuNumSamplesPerRay, stringBuf, 10);
            if (ImGui::InputText("Cpu Num Rays Per Pixel", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
            {
                UserInput.CpuNumSamplesPerRay = atoi(stringBuf);
                cpuOptionsChanged = true;
            }

            _itoa_s(UserInput.CpuMaxScatterDepth, stringBuf, 10);
            if (ImGui::InputText("Cpu Recursion Depth", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
            {
                UserInput.CpuMaxScatterDepth = atoi(stringBuf);
                cpuOptionsChanged = true;
            }

            if (ImGui::SliderInt("Cpu Num Threads", &UserInput.CpuNumThreads, 1, MaxNumCpuThreads))
            {
                cpuOptionsChanged = true;
            }

            ImGui::Text("");
            if (ImGui::Button("Begin Cpu Raytrace", ImVec2(140, 30)))
            {
                SetEnableCpuRaytrace(true);
            }

            // ------------------------------------------------------------

            ImGui::Separator();
            ImGui::Text("GPU RAYTRACE OPTIONS");
            ImGui::Separator();

            if (ImGui::SliderInt("Gpu Rays Per Pixel", &UserInput.GpuNumRaysPerPixel, 1, 100))
            {
                cpuOptionsChanged = true;
            }

            if (ImGui::SliderFloat("Gpu Direct Light Scale", &UserInput.GpuDirectLightMult, 0.0f, 10.0f))
            {
                gpuOptionsChanged = true;
            }

            if (ImGui::SliderFloat("Gpu Indirect Light Scale", &UserInput.GpuIndirectLightMult, 0.0f, 10.0f))
            {
                gpuOptionsChanged = true;
            }

            if (ImGui::SliderFloat("Gpu AO Radius", &UserInput.GpuAORadius, 1.0f, 1000.0f))
            {
                gpuOptionsChanged = true;
            }

            if (ImGui::Checkbox("Enable Camera Jitter", &UserInput.GpuCameraJitter))
            {
                gpuOptionsChanged = true;
            }

            // ------------------------------------------------------------

            // If gpu options changed, trigger camera to update
            if (gpuOptionsChanged)
            {
                SetCameraDirty();
            }
        }

        ImGui::End();
        break;
    }
    ImGui::EndFrame();

    // Setup context and render
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderGUI");
    {
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());
        renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
        renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RenderDevice::Get().GetImguiDescriptorHeapStack().GetDescriptorHeap());

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderContext.GetCommandList());
    }
    renderContext.Finish();
}
