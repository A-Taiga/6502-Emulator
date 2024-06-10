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
}

void page_group (const char* text, debug::Data& data, int offset = 0, float xPos = 0.0, float yPox = 0.0)
{
    static ImVec2 min;
    static ImVec2 max;
    static ImDrawList* drawList = ImGui::GetWindowDrawList();
    static ImDrawListSplitter splitter;
    auto t = ImGui::GetCursorScreenPos().y - ImGui::CalcTextSize("Zero Page").y;
    splitter.Split(drawList, 3);
    splitter.SetCurrentChannel(drawList, 2);
    ImGui::SetCursorPos({ImGui::GetCursorScreenPos().x + xPos, t + yPox});
    ImGui::Text("%s", text);
    ImGui::SetCursorPos({ImGui::GetCursorScreenPos().x + xPos, t + yPox + ImGui::CalcTextSize(text).y + ImGui::CalcTextSize(text).y / 2});
    splitter.SetCurrentChannel(drawList, 1);
    drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32({0,0,0,255}));
    ImGui::BeginGroup();
    ImGui::Text("%s", std::format("{:<5}","").c_str());
    for (std::uint16_t i = 0; i < 16; i++)
    {
        ImGui::SameLine();
        ImGui::Text("%02X", i);
    }
    ImGui::NewLine();
    for (int row = 0; row < 16; row++)
    {
        ImGui::Text("%04X:", (row*16) + offset);
        for (std::size_t i = 0; i < 16; i++)
        {
            ImGui::SameLine();
            ImGui::Text("%02X", data.memory[((row*16)+i) + offset]);
        }
    }
    ImGui::EndGroup();
    splitter.SetCurrentChannel(drawList, 0);
    min = {ImGui::GetItemRectMin().x - ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMin().y - ImGui::CalcTextSize(" ").y};
    max = {ImGui::GetItemRectMax().x + ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMax().y + ImGui::CalcTextSize(" ").y};
    drawList->AddRect(min, max, ImGui::GetColorU32({255,255,255,255}));
    splitter.Merge(drawList);
};

void debug::test_demo (Window& window,[[maybe_unused]]debug::Data& data)
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    static ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding.x = ImGui::CalcTextSize("   ").x;
    style.WindowPadding.y = ImGui::CalcTextSize("   ").y;
    ImGui::SetNextWindowSize({(float)window.get_w(), (float)window.get_h()});
    ImGui::Begin("main window", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowFontScale(1.2);
    
    page_group("Zero Page", data);
    page_group("Page 1", data, STK_END+1, ImGui::GetItemRectMax().x,  -ImGui::GetItemRectMax().y + ImGui::CalcTextSize("").y - 6.5);
    ImGui::NewLine();
    ImGui::Text("X  : $%02X", data.cpu.X);
    ImGui::Text("PC : $%04X", data.cpu.PC);

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




