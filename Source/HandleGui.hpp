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
        ImGui::SetNextWindowPos(ImVec2(770, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 460), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags window_flags = 0;
        if (!ImGui::Begin("Raytracer", nullptr, window_flags))
        {
            ImGui::End();
            break;
        }

        ImGui::Separator();
        ImGui::Text("--- STATUS ---");
        ImGui::Separator();
        ImGui::BulletText("Selected Buffer Id: %d", SelectedBufferIndex + 1);

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
                ImGui::BulletText("Scatter Depth: %d", sMaxScatterDepth);
                ImGui::BulletText("Num Threads: %d", sNumThreads);
                ImGui::BulletText("Num Samples: %d", sNumSamplesPerRay);
                ImGui::BulletText("Done Samples: %d", stats.CompletedSampleCount);
                ImGui::BulletText("Rays Fired: %" PRId64 "", stats.TotalRaysFired);
                ImGui::BulletText("Num Pixels Sampled: %" PRId64 "", stats.NumPixelSamples);
                ImGui::BulletText("Pdf Query Retries: %d", stats.NumPdfQueryRetries);
            }

            ImGui::Text("");
            if (ImGui::Button("Stop Trace", ImVec2(100, 50)))
            {
                SetEnableCpuRaytrace(false);
            }
        }
        else
        {
            char stringBuf[128];
            bool rayTracerDirty = false;

            ImGui::Separator();
            ImGui::Text("--- HELP ---");
            ImGui::Separator();
            ImGui::BulletText("Translate camera:    Press keys WASD strafe, QE up and down");
            ImGui::BulletText("Pan camera:          Left/Right mouse button (press and hold)");
            ImGui::BulletText("Start/stop trace:    Space bar/escape key");
            ImGui::BulletText("Select output:       1 - 9 keys");


            ImGui::Separator();
            ImGui::Text("--- GLOBAL OPTIONS ---");
            ImGui::Separator();
            if (ImGui::ListBox("Scene Select", &sSampleScene, SampleSceneNames, IM_ARRAYSIZE(SampleSceneNames), MaxScene))
            {
                LoadSceneRequested = true;
            }

            ImGui::Separator();
            ImGui::Text("--- CPU RAYTRACE OPTIONS ---");
            ImGui::Separator();
            _itoa_s(sNumSamplesPerRay, stringBuf, 10);
            if (ImGui::InputText("Num Samples", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
            {
                sNumSamplesPerRay = atoi(stringBuf);
                rayTracerDirty = true;
            }

            _itoa_s(sMaxScatterDepth, stringBuf, 10);
            if (ImGui::InputText("Scatter Depth", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
            {
                sMaxScatterDepth = atoi(stringBuf);
                rayTracerDirty = true;
            }

            if (ImGui::SliderInt("Num Threads", &sNumThreads, 1, 32))
            {
                rayTracerDirty = true;
            }

            // If any options changed, recreate the raytracer
            if (rayTracerDirty)
            {
                OnResizeRaytracer();
            }

            ImGui::Text("");
            if (ImGui::Button("Begin Trace", ImVec2(100, 50)))
            {
                SetEnableCpuRaytrace(true);
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
