#include "app.h"
#include <imgui/imgui.h>

void ImApp::init() {

}

void ImApp::quit() {

}

void ImApp::tick() {
    ImGuiIO& io = ImGui::GetIO();
    static const char* content_window_name = "##niagara_perf_viewer_main_window";
    static const ImGuiWindowFlags content_window_flags =
        ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - ImGui::GetFrameHeight()));
    if (ImGui::Begin(content_window_name, nullptr, content_window_flags))
    {
        
        
        ImGui::End();
    }
}
