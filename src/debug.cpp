#include "debug.hpp"
#include "common.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include "bus.hpp"



namespace
{
}

void debug::init (Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForSDLRenderer(window.get_window(), window.get_renderer());
    ImGui_ImplSDLRenderer2_Init(window.get_renderer());
    ImGui::GetIO().FontGlobalScale = 1.4;

}


void page_group (const char* text, std::function<void(ImVec2&, ImVec2&)> callback, float xPos = 0.0, float yPox = 0.0)
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
    callback(min, max);
    ImGui::EndGroup();
    splitter.SetCurrentChannel(drawList, 0);
    min = {ImGui::GetItemRectMin().x - ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMin().y - ImGui::CalcTextSize(" ").y};
    max = {ImGui::GetItemRectMax().x + ImGui::CalcTextSize(" ").x, ImGui::GetItemRectMax().y + ImGui::CalcTextSize(" ").y};
    drawList->AddRect(min, max, ImGui::GetColorU32({255,255,255,255}));
    splitter.Merge(drawList);
};

void debug::test_demo (Window& window, _6502::Bus data)
{
    static int current_page_index = 2;
    static char buffer[5] = {0};
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    static ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding.x = ImGui::CalcTextSize("   ").x;
    style.WindowPadding.y = ImGui::CalcTextSize("   ").y;
    ImGui::SetNextWindowSize({(float)window.get_w(), (float)window.get_h()});
    ImGui::Begin("main window", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {30, 30});
    static bool openSettings = false;
    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu ("Menu"))
        {
            if(ImGui::MenuItem("Settings"))
                openSettings = true;
                
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (openSettings)
        ImGui::OpenPopup("Settings");
    else
        ImGui::CloseCurrentPopup();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize({200,200});
    if (ImGui::BeginPopupModal("Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::DragFloat("Font Size", &ImGui::GetIO().FontGlobalScale);
        
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
            openSettings = false;
        }
        ImGui::EndPopup();
    }
    ImGui::BeginTable("test", 3, ImGuiTableFlags_Borders  | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PreciseWidths);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(1);
    if(ImGui::InputText("##page input", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
    {  
        auto page = std::strtol(buffer, nullptr, 16);
        if (page < 0xFFFF && page >= 0x0000)
            current_page_index = page / 256;
    }
    ImGui::SameLine();
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
    page_group("Zero Page", [&](ImVec2&, ImVec2&){
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
    
    page_group (p.c_str(), [&](ImVec2&, ImVec2&)
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

    ImGui::TableSetColumnIndex(2);
    page_group("Program", [&](ImVec2&, ImVec2&)
    {
        ImGui::Text("%s", std::format("{:<6}","").c_str());
        for (std::uint16_t i = 0; i < 16; i++)
        {
            ImGui::SameLine();
            ImGui::Dummy(ImGui::CalcTextSize(std::format("{:02X}", i).c_str()));
        }
        ImGui::NewLine();
        ImGui::BeginChild("C", {}, ImGuiChildFlags_ResizeY);
        ImGuiListClipper clipper;
        clipper.Begin(1);
        while (clipper.Step())
        {
  
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                for (std::uint16_t i = ROM_BEGIN; i < ROM_END;)
                {

                    auto *ins = &data.cpu.opcodes[data.ram[i]];
                    if (data.cpu.PC == i) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                    ImGui::TextUnformatted(std::format("{:04X}: ", i).c_str());
                    if (data.cpu.PC == i) ImGui::PopStyleColor();
                    ImGui::SameLine();
                    if (ins->mode == &_6502::CPU::IMP)
                    {
                        if (data.cpu.PC == i) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                        ImGui::TextUnformatted(std::format ("{:02X} {:>9}\n", data.ram[i], ins->mnemonic).c_str());
                        if (data.cpu.PC == i) ImGui::PopStyleColor();
                        i+=1;
                    }
                    else if (ins->mode == &_6502::CPU::IMM
                           ||ins->mode == &_6502::CPU::ZPG 
                           ||ins->mode == &_6502::CPU::ZPX
                           ||ins->mode == &_6502::CPU::ZPY
                           ||ins->mode == &_6502::CPU::IZX
                           ||ins->mode == &_6502::CPU::IZY
                           ||ins->mode == &_6502::CPU::REL
                           )
                    {
                        if (data.cpu.PC == i) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                        ImGui::TextUnformatted(std::format ("{:02X} {:02X} {:>6}", data.ram[i], data.ram[i+1], ins->mnemonic).c_str());
                        if (data.cpu.PC == i) ImGui::PopStyleColor();
                        i+=2;
                    }
                    else if (ins->mode == &_6502::CPU::ABS
                           ||ins->mode == &_6502::CPU::ABX
                           ||ins->mode == &_6502::CPU::ABY
                           ||ins->mode == &_6502::CPU::IND
                           )
                    {
                        if (data.cpu.PC == i) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                        ImGui::TextUnformatted(std::format ("{:02X} {:02X} {:02X} {:}", data.ram[i], data.ram[i+1], data.ram[i+1], ins->mnemonic).c_str());
                        if (data.cpu.PC == i) ImGui::PopStyleColor();
                        i+=3;
                    }
                    else 
                    {
                        i++;
                    }
                }
            }
        }
        clipper.End();
        ImGui::EndChild();
    });
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    page_group("Registers", [&](ImVec2&, ImVec2&)
    {


        ImGui::TextUnformatted(std::format("NV-BDIZC\n{:08b}", data.cpu.SR).c_str());
        ImGui::TextUnformatted(std::format("PC : ${:04X} {:<5} {:08b} {:08b}", data.cpu.PC, data.cpu.PC, (data.cpu.PC & 0xFF00) >> 8, data.cpu.PC & 0x00FF).c_str());
        ImGui::TextUnformatted(std::format("AC : ${:02X} {:<2}{:<5} {:08b}", data.cpu.AC, "", data.cpu.AC, data.cpu.AC).c_str());
        ImGui::TextUnformatted(std::format("SP : ${:02X} {:<2}{:<5} {:08b}", data.cpu.SP, "", data.cpu.SP, data.cpu.SP).c_str());
        ImGui::TextUnformatted(std::format("X  : ${:02X} {:<2}{:<5} {:08b}", data.cpu.X, "", data.cpu.X, data.cpu.X).c_str());
        ImGui::TextUnformatted(std::format("Y  : ${:02X} {:<2}{:<5} {:08b}", data.cpu.Y, "", data.cpu.Y, data.cpu.Y).c_str());

        word nmiV = (data.ram[0xFFFB] << 8) | data.ram[0xFFFA];
        word resV = (data.ram[0xFFFD] << 8) | data.ram[0xFFFC];
        word irqV = (data.ram[0xFFFF] << 8) | data.ram[0xFFFE];

        ImGui::TextUnformatted(std::format("NMI: ${:04X} {:<5} {:08b} {:08b}", nmiV, nmiV, data.ram[0xFFFB], data.ram[0xFFFA]).c_str());
        ImGui::TextUnformatted(std::format("RES: ${:04X} {:<5} {:08b} {:08b}", resV, resV, data.ram[0xFFFD], data.ram[0xFFFC]).c_str());
        ImGui::TextUnformatted(std::format("IRQ: ${:04X} {:<5} {:08b} {:08b}", irqV, irqV, data.ram[0xFFFF], data.ram[0xFFFE]).c_str());


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
