// ImHotKey v1.0
// https://github.com/CedricGuillemet/ImHotKey
//
// The MIT License(MIT)
// 
// Copyright(c) 2019 Cedric Guillemet
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once


inline void ImMouseState()
{
    float sz = 50;
    float th = 3.f;
    float butsz = 20.f;
    unsigned int col = 0xFFAAAAAA;
    unsigned int colDown = 0xFF2020FF;

    auto& io = ImGui::GetIO();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowContentSize(ImVec2(sz, sz * 1.5f));
    ImGui::BeginTooltip();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImVec2 p1 = pos;
    ImVec2 p2 = ImVec2(pos.x + butsz, pos.y + butsz);
    ImVec2 p3 = ImVec2(pos.x + butsz + 10.f, pos.y);
    ImVec2 p4 = ImVec2(pos.x + butsz + 10.f + butsz, pos.y + butsz);
    ImVec2 p5 = ImVec2(pos.x + sz, pos.y + sz * 1.5f);

    // wheel up
    ImVec2 p6 = ImVec2(pos.x + butsz, pos.y);
    ImVec2 p7 = ImVec2(pos.x + butsz + 10.f, pos.y + butsz / 2.f);

    // wheel down
    ImVec2 p8 = ImVec2(pos.x + butsz, pos.y + butsz / 2.f);
    ImVec2 p9 = ImVec2(pos.x + butsz + 10.f, pos.y + butsz);

    draw_list->AddRectFilled(p1, p5, 0xFF666666, 10.0f, ImDrawCornerFlags_All);
    draw_list->AddRect(p1, p5, col, 10.0f, ImDrawCornerFlags_All, th);

    // left
    draw_list->AddRect(p1, p2, col, 10.0f, ImDrawCornerFlags_TopLeft, th);
    if (io.MouseDown[0])
    {
        draw_list->AddRectFilled(p1, p2, colDown, 10.0f, ImDrawCornerFlags_TopLeft);
    }

    // right
    draw_list->AddRect(p3, p4, col, 10.0f, ImDrawCornerFlags_TopRight, th);
    if (io.MouseDown[1])
    {
        draw_list->AddRectFilled(p3, p4, colDown, 10.0f, ImDrawCornerFlags_TopRight);
    }

    // wheel
    draw_list->AddRect(p6, p7, col, 0.0f, 0, th);
    if (io.MouseWheel < -0.001f)
    {
        draw_list->AddRectFilled(p6, p7, colDown, 0.0f, 0);
    }

    draw_list->AddRect(p8, p9, col, 0.0f, 0, th);
    if (io.MouseWheel > 0.001f)
    {
        draw_list->AddRectFilled(p8, p9, colDown, 0.0f, 0);
    }

    // modifier keys
    bool keyDown[] = { io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper };
    static const char * keyDownLibs[] = { "CTRL", "SHIFT", "ALT", "SUPER" };
    ImVec2 textPos = ImVec2(pos.x + 5.f, pos.y + butsz * 1.2f);
    for (int i = 0, av = 0; i < 4; i++)
    {
        if (keyDown[i])
        {
            draw_list->AddText(textPos, 0xFFFFFFFF, keyDownLibs[i]);
            av++;
            textPos.y += 16.f;
        }
    }
    ImGui::EndTooltip();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
