#include "debug.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "common.hpp"
#include <cstdio>
#include <format>



debug::Data::Data(_6502::CPU& processor, _6502::RAM& ram)
: cpu(processor)
, memory(ram)
{}

void debug::init (Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForSDLRenderer(window.get_window(), window.get_renderer());
    ImGui_ImplSDLRenderer2_Init(window.get_renderer());
    // fontSize.x = ImGui::CalcTextSize("F").x;
    // fontSize.y = ImGui::GetTextLineHeightWithSpacing();
    // windowSize = ImGui::GetIO().DisplaySize;
}


void debug::test_demo (Window& window,[[maybe_unused]]debug::Data& data)
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    static ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    // ImGui::SetNextWindowSize(ImVec2{static_cast<float>(window.get_w()) / 2, static_cast<float>(window.get_h())});
    ImGui::SetNextWindowPos({0,0});
    ImGui::Begin("test",nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("%s", std::format("{:<5}","").c_str());
    for (std::uint16_t i = 0; i < 16; i++)
    {
        ImGui::SameLine();
        ImGui::Text("%02X", i);
    }
    ImGui::End();
    ImGui::SetNextWindowPos({0, 25});
    if (ImGui::Begin("Zero Page", nullptr,  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
    {
 
        // ImGui::BeginChild("TEST", {ImGui::CalcTextSize("FF").x * 16 * 2,ImGui::CalcTextSize("FF").y * 22}, ImGuiWindowFlags_NoResize);
        {
            static ImGuiListClipper clipper;
            clipper.Begin(RAM_SIZE / 16);
            ImGui::SetWindowFontScale(1);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::Text("%04X:", row*16);
                    for (std::size_t i = 0; i < 16; i++)
                    {
                        ImGui::SameLine();
                        ImGui::Text("%02X", data.memory[(row*16)+i]);
                    }
                }
            }
        }
    }
    ImGui::End();

    debug::render (window);
    
}

void debug::render (Window& window)
{   
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(window.get_renderer());
    SDL_RenderSetScale(window.get_renderer(), ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(window.get_renderer(), 0,0,0,0);
    SDL_RenderClear(window.get_renderer());
}

