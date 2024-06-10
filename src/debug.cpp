#include "debug.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <cstdio>
#include <cstring>
#include <format>
#include "bus.hpp"

void debug::init (Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForSDLRenderer(window.get_window(), window.get_renderer());
    ImGui_ImplSDLRenderer2_Init(window.get_renderer());
}

void page_group (const char* text, std::function<void()> callback, float xPos = 0.0, float yPox = 0.0)
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
    callback();
    ImGui::EndGroup();
    splitter.SetCurrentChannel(drawList, 0);
    min = {ImGui::GetItemRectMin().x - ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMin().y - ImGui::CalcTextSize(" ").y};
    max = {ImGui::GetItemRectMax().x + ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMax().y + ImGui::CalcTextSize(" ").y};
    drawList->AddRect(min, max, ImGui::GetColorU32({255,255,255,255}));
    splitter.Merge(drawList);
};
void debug::test_demo (Window& window, _6502::Bus data)
// void debug::test_demo (Window& window,debug::Data& data)
{
    static int current_page_index = 0;
    static char buffer[5] = {0};
    std::string SR = std::format("NV-BDIZC\n{:08b}", data.cpu.SR);
    std::string PC = std::format("PC : ${:04X} {:<5} {:08b} {:08b}", data.cpu.PC, data.cpu.PC, (data.cpu.PC & 0xFF00) >> 8, data.cpu.PC & 0x00FF);
    std::string AC = std::format("AC : ${:02X} {:<2}{:<5} {:08b}", data.cpu.AC, "", data.cpu.AC, data.cpu.AC);
    std::string SP = std::format("SP : ${:02X} {:<2}{:<5} {:08b}", data.cpu.SP, "", data.cpu.SP, data.cpu.SP);
    std::string X  = std::format("X  : ${:02X} {:<2}{:<5} {:08b}", data.cpu.X, "", data.cpu.X, data.cpu.X);
    std::string Y  = std::format("Y  : ${:02X} {:<2}{:<5} {:08b}", data.cpu.Y, "", data.cpu.Y, data.cpu.Y);
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    static ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding.x = ImGui::CalcTextSize("   ").x;
    style.WindowPadding.y = ImGui::CalcTextSize("   ").y;
    ImGui::SetNextWindowSize({(float)window.get_w(), (float)window.get_h()});
    ImGui::Begin("main window", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowFontScale(1.2);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {20, 20});
    ImGui::BeginTable("test", 3, ImGuiTableFlags_Borders  | ImGuiTableFlags_SizingFixedFit);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(1);
    if(ImGui::InputText("##page input", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
    {  
        auto page = std::strtol(buffer, nullptr, 16);
        if (page < 0xFFFF && page >= 0x0000)
            current_page_index = page / 256;
    }
    if (ImGui::Button("<<"))
    {
        current_page_index--;
        if (current_page_index < 0)
            current_page_index = 255;
    }
    ImGui::SameLine();
    if (ImGui::Button(">>"))
    {
        current_page_index++;
        if (current_page_index > 255)
            current_page_index = 0;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    page_group("Zero Page", [&](){
        ImGui::Text("%s", std::format("{:<6}","").c_str());
        for (std::uint16_t i = 0; i < 16; i++)
        {
            ImGui::SameLine();
            ImGui::Text("%02X", i);
        }
        ImGui::NewLine();
        for (int row = 0; row < 16; row++)
        {
            ImGui::Text("%04X: ", (row*16));
            for (std::size_t i = 0; i < 16; i++)
            {
                ImGui::SameLine();
                ImGui::Text("%02X", data.ram[((row*16)+i)]);
            }
        }
    });
    ImGui::TableSetColumnIndex(1);
    static std::string p = {};
    if (current_page_index == 0)
        p = "Zero Page";
    else if (current_page_index == 1)
        p = "Stack";
    else
        p = std::format("Page {:d}", current_page_index - 1);
    
    page_group (p.c_str(), [&]()
    {
        ImGui::Text("%s", std::format("{:<6}","").c_str());
        for (std::uint16_t i = 0; i < 16; i++)
        {
            ImGui::SameLine();
            ImGui::Text("%02X", i);
        }
        ImGui::NewLine();
        for (int row = 0; row < 16; row++)
        {
            ImGui::Text("%04X: ", (row*16) + (current_page_index * 256));
            for (std::size_t i = 0; i < 16; i++)
            {
                ImGui::SameLine();
                ImGui::Text("%02X", data.ram[((row*16)+i) + (current_page_index * 256)]);
            }
        }
    });
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    page_group("Registers", [&]()
    {
        ImGui::TextUnformatted(SR.c_str());
        ImGui::TextUnformatted(PC.c_str());
        ImGui::TextUnformatted(AC.c_str());
        ImGui::TextUnformatted(SP.c_str());
        ImGui::TextUnformatted(X.c_str());
        ImGui::TextUnformatted(Y.c_str());
    });
    ImGui::EndTable();
    ImGui::PopStyleVar();
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




