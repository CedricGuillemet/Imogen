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
/*
How to use it?

// Get some hotkeys composed of:
// - hotkey name
// - hotkey comment/lib
// - hotkey scancodes. Computed by the editor. Store that value in your app.

static std::vector<ImHotKey::HotKey> hotkeys = { { "Layout", "Reorder nodes in a simpler layout", 0xFFFF261D}
        ,{"Save", "Save the current graph", 0xFFFF1F1D}
        ,{"Load", "Load an existing graph file", 0xFFFF181D}
        ,{"Play/Stop", "Play or stop the animation from the current graph", 0xFFFFFF3F}
        ,{"SetKey", "Make a new animation key with the current parameters values at the current time", 0xFFFFFF1F}
        };

// The editor is a modal window. bring it with something like that
 if (ImGui::Button("Edit Hotkeys"))
{
    ImGui::OpenPopup("HotKeys Editor");
}
ImHotKey::Edit(hotkeys.data(), hotkeys.size(), "HotKeys Editor");

// ImHotKey also provides a way to retrieve HotKey
int hotkey = ImHotKey::GetHotKey(hotkeys.data(), hotkeys.size());
if (hotkey != -1)
{
    // handle the hotkey index!
}

Awesome, you are done!
To help you integrate in your app, you can get a text (like "Ctrl + O") to integrate in your menu
static void GetHotKeyLib(unsigned int functionKeys, char* buffer, size_t bufferSize);

*/

#include "imgui.h"
#include "imgui_internal.h"

namespace ImHotKey
{
    struct HotKey
    {
        const char *functionName;
        const char *functionLib;
        unsigned int functionKeys;
    };

    struct Key
    {
        const char* lib = nullptr;
        unsigned int order;
        unsigned int scanCodePage1 = 0; // win32 scancode
        unsigned int scanCodePage7 = 0; // HID (SDL,...)
        float offset = 0;
        float width = 40;
    };

    static const Key Keys[6][18] = {
        { {"Esc", 4, 0x1, 0x29, 18}, {"F1", 5, 0x3B, 0x3A, 18}, {"F2", 6, 0x3C, 0x3B}, {"F3", 7, 0x3D, 0x3C}, {"F4", 8, 0x3E, 0x3D}, {"F5", 9, 0x3F, 0x3E, 24}, {"F6", 10, 0x40, 0x3F}, {"F7", 11, 0x41, 0x40}, {"F8", 12, 0x42, 0x41}, {"F9", 13, 0x43, 0x42, 24}, {"F10", 14, 0x44, 0x43}, {"F11", 15, 0x57, 0x44}, {"F12", 16, 0x58, 0x45}, {"PrSn", 17, 0x37, 0x46, 24}, {"ScLk", 18, 0x46}, {"Brk", 19, 126, 0x47} },
        { {"~", 20, 0x29, 0x35}, {"1", 21, 0x2, 0x1E}, {"2", 22, 0x3, 0x1F}, {"3", 23, 0x4, 0x20}, {"4", 24, 0x5, 0x21}, {"5", 25, 0x6, 0x22}, {"6", 26, 0x7, 0x23}, {"7", 27, 0x8, 0x24}, {"8", 28, 0x9, 0x25}, {"9", 29, 0xA, 0x26}, {"0", 30, 0xB, 0x27}, {"-", 31, 0xC, 0x2D}, {"+", 32, 0xD, 0x2E},{"Backspace", 33, 0xE, 0x2A, 0, 80}, {"Ins", 34, 0x52, 0x49, 24}, {"Hom", 35, 0x47, 0x4A}, {"PgU", 36, 0x49, 0x4B} },
        { {"Tab", 3, 0xF, 0x2B, 0, 60}, {"Q", 37, 0x10, 0x14}, {"W", 38, 0x11, 0x1A}, {"E", 39, 0x12, 0x08}, {"R", 40, 0x13, 0x15}, {"T", 41, 0x14, 0x17}, {"Y", 42, 0x15, 0x1C}, {"U", 43, 0x16, 0x18}, {"I", 44, 0x17, 0x0C}, {"O", 45, 0x18, 0x12}, {"P", 46, 0x19, 0x13}, {"[", 47, 0x1A, 0x2F}, {"]", 48, 0x1B, 0x30}, {"|", 49, 0x2B, 0x31, 0, 60}, {"Del", 50, 0x53, 0x4C, 24}, {"End", 51, 0x4F, 0x4D}, {"PgD", 52, 0x51, 0x4E} },
        { {"Caps Lock", 53, 0x3A, 0x39, 0, 80}, {"A", 54, 0x1E, 0x04}, {"S", 55, 0x1F, 0x16}, {"D", 56, 0x20, 0x07}, {"F", 57, 0x21, 0x09}, {"G", 58, 0x22, 0x0A}, {"H", 59, 0x23, 0x0B}, {"J", 60, 0x24, 0x0D}, {"K", 61, 0x25, 0x0E}, {"L", 62, 0x26, 0x0F}, {";", 63, 0x27, 0x33}, {"'", 64, 0x28, 0x34}, {"Ret", 65, 0x1C, 0X28, 0, 84} },
        { {"Shift", 2, 0x2A, 0xE1, 0, 104}, {"Z", 66, 0x2C, 0x1D}, {"X", 67, 0x2D, 0x1B}, {"C", 68, 0x2E, 0x06}, {"V", 69, 0x2F, 0x19}, {"B", 70, 0x30, 0x05}, {"N", 71, 0x31, 0x11}, {"M", 72, 0x32, 0x10}, {",", 73, 0x33, 0x36}, {".", 74, 0x34, 0x37}, {"/", 75, 0x35, 0x38}, {"Shift", 2, 0x2A, 0xE5, 0, 104}, {"Up", 76, 0x48, 0x52, 68} },
        { {"Ctrl", 0, 0x1D, 0xE0, 0, 60}, {"Alt", 1, 0x38, 0xE2, 68, 60}, {"Space", 77, 0x39, 0X2c, 0, 260}, {"Alt", 1, 0x38, 0xE6, 0, 60}, {"Ctrl", 0, 0x1D, 0xE4, 68, 60}, {"Left", 78, 0x4B, 0x50, 24}, {"Down", 79, 0x50, 0x51}, {"Right", 80, 0x4D, 0x52} }
    };

    static const Key& GetKeyForScanCode(unsigned int scancode)
    {
        for (unsigned int y = 0; y < 6; y++)
        {
            int x = 0;
            while (Keys[y][x].lib)
            {
#ifdef SDL_h_
                if (Keys[y][x].scanCodePage7 == scancode)
#elif WIN32
                if (Keys[y][x].scanCodePage1 == scancode)
#else
#error
#endif
                    return Keys[y][x];
                x++;
            }
        }
        return Keys[0][0];
    }

    static unsigned int GetOrderedScanCodes(unsigned char scanCodes[4], unsigned char order[4])
    {
        for (int pass = 0; pass < 2; pass++)
        {
            for (int o = 0; o < 3; o++)
            {
                if (order[o] > order[o + 1])
                {
                    ImSwap(order[o], order[o + 1]); ImSwap(scanCodes[o], scanCodes[o + 1]);
                }
            }
        }
        return (scanCodes[3] << 24) + (scanCodes[2] << 16) + (scanCodes[1] << 8) + scanCodes[0];
    }

    static void HotKeySPrintf(char* buffer, size_t bufferSize, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(buffer, bufferSize, fmt, args);
        va_end(args);
    }

    static void GetHotKeyLib(unsigned int functionKeys, char* buffer, size_t bufferSize, const char *functionLib = nullptr)
    {
        static const char* str[4] = { "%s", "%s + %s", "%s + %s +%s", "%s + %s + %s + %s" };
        static const char* strLib[4] = { "%s (%s)", "%s (%s + %s)", "%s (%s + %s +%s)", "%s (%s + %s + %s + %s)" };
        static const char* lib[4];
        int scanCodeCount = 0;
        for (int i = 0; i < 4; i++)
        {
            unsigned char scanCode = (unsigned char)(functionKeys >> i * 8);
            if (scanCode == 0xFF)
            {
                continue;
            }
            lib[scanCodeCount] = GetKeyForScanCode(scanCode).lib;
            scanCodeCount++;
        }
        if (!scanCodeCount)
        {
            buffer[0] = 0;
            return;
        }
        
        if (functionLib)
        {
            const char* fmt = strLib[scanCodeCount - 1];
            HotKeySPrintf(buffer, bufferSize, fmt, functionLib, lib[0], lib[1], lib[2], lib[3]);
        }
        else
        {
            const char* fmt = str[scanCodeCount - 1];
            HotKeySPrintf(buffer, bufferSize, fmt, lib[0], lib[1], lib[2], lib[3]);
        }
    }

    static void Edit(HotKey *hotkey, size_t hotkeyCount, const char *popupModal)
    {
        static int editingHotkey = -1;
        if (!hotkeyCount)
            return;
        static bool keyDown[512] = {};

        ImGui::SetNextWindowSize(ImVec2(1060, 400));
        if (!ImGui::BeginPopupModal(popupModal, NULL, ImGuiWindowFlags_NoResize))
            return;

        ImGui::BeginChildFrame(127, ImVec2(220, -1));
        for(size_t i = 0;i< hotkeyCount;i++)
        {
            char hotKeyLib[128];
            GetHotKeyLib(hotkey[i].functionKeys, hotKeyLib, sizeof(hotKeyLib), hotkey[i].functionName);
            if (ImGui::Selectable(hotKeyLib, editingHotkey == int(i)) || editingHotkey == -1)
            {
                editingHotkey = int(i);
                memset(keyDown, 0, sizeof(keyDown));
                for (int j = 0; j < 4; j++)
                {
                    int scan = (hotkey[editingHotkey].functionKeys >> (8 * j)) & 0xFF;
                    if (scan != 0xFF)
                    {
                        keyDown[scan] = true;
                    }
                }
            }
        }
        ImGui::EndChildFrame();
        ImGui::SameLine();
        ImGui::BeginGroup();

        for (int i = 0; i < 512; i++)
        {
            if (ImGui::IsKeyPressed(i, false))
            {
                int imKey;
#ifdef SDL_h_
                imKey = i;
#elif WIN32
                imKey = MapVirtualKeyA(i, MAPVK_VK_TO_VSC);
#else
                imKey = i;
#endif
                keyDown[imKey] = !keyDown[imKey];
            }
        }
        for (unsigned int y = 0; y < 6; y++)
        {
            int x = 0;
            ImGui::BeginGroup();
            while (Keys[y][x].lib)
            {
                const Key& key = Keys[y][x];
                const float ofs = key.offset + (x?4.f:0.f);

                const float width = key.width;
                if (x)
                {
                    ImGui::SameLine(0.f, ofs);
                }
                else
                {
                    if (ofs >= 1.f)
                    {
                        ImGui::Indent(ofs);
                    }
                }
#ifdef SDL_h_
                bool& butSwtch = keyDown[key.scanCodePage7];
#elif WIN32
                bool& butSwtch = keyDown[key.scanCodePage1];
#else
#error
#endif
                ImGui::PushStyleColor(ImGuiCol_Button, butSwtch ? 0xFF1040FF : 0x80000000);
                if (ImGui::Button(Keys[y][x].lib, ImVec2(width, 40)))
                {
                    butSwtch = !butSwtch;
                }
                ImGui::PopStyleColor();
                x++;
            }
            ImGui::EndGroup();
        }
        ImGui::InvisibleButton("space", ImVec2(10, 55));
        ImGui::BeginChildFrame(18, ImVec2(540, 40));
        ImGui::Text("%s :", hotkey[editingHotkey].functionName);
        ImGui::SameLine();
        ImGui::TextWrapped("%s", hotkey[editingHotkey].functionLib);
        ImGui::EndChildFrame();
        ImGui::SameLine();
        int keyDownCount = 0;
        for (auto d : keyDown)
        {
            keyDownCount += d ? 1 : 0;
        }
        if (ImGui::Button("Clear", ImVec2(80, 40)))
        {
            memset(keyDown, 0, sizeof(keyDown));
        }
        ImGui::SameLine();
        if (keyDownCount && keyDownCount < 5)
        {
            if (ImGui::Button("Set", ImVec2(80, 40)))
            {
                unsigned char scanCodes[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
                unsigned char order[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
                int scanCodeCount = 0;
                hotkey[editingHotkey].functionKeys = 0;
                for (int i = 1; i < sizeof(keyDown); i++)
                {
                    if (keyDown[i])
                    {
                        scanCodes[scanCodeCount] = (unsigned char)i;
                        order[scanCodeCount] = (unsigned char)GetKeyForScanCode(i).order;
                        scanCodeCount++;
                    }
                }

                hotkey[editingHotkey].functionKeys = GetOrderedScanCodes(scanCodes, order);
            }
            ImGui::SameLine(0.f, 20.f);
        }
        else
        {
            ImGui::SameLine(0.f, 100.f);
        }
        
        if (ImGui::Button("Done", ImVec2(80, 40))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndGroup();
        ImGui::EndPopup();
    }

    static int GetHotKey(HotKey *hotkey, size_t hotkeyCount)
    {
        static unsigned int lastHotKey = 0xFFFFFFFF;
        unsigned char scanCodes[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        unsigned char order[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        int scanCodeCount = 0;
        for (int i = 0; i < 512; i++)
        {
            if (ImGui::IsKeyDown(i))
            {
                int imKey;
#ifdef SDL_h_
                imKey = i;
#elif WIN32
                imKey = MapVirtualKeyA(i, MAPVK_VK_TO_VSC);
#else
                imKey = i;
#endif
                scanCodes[scanCodeCount] = (unsigned char)imKey;
                order[scanCodeCount] = (unsigned char)GetKeyForScanCode(imKey).order;
                scanCodeCount++;
                if (scanCodeCount == 4)
                    break;
            }
        }

        unsigned int newHotKey = GetOrderedScanCodes(scanCodes, order);

        if (scanCodeCount)
        {
            if (newHotKey != lastHotKey)
            {
                for (size_t i = 0; i < hotkeyCount; i++)
                {
                    if (hotkey[i].functionKeys == newHotKey)
                    {
                        lastHotKey = newHotKey;
                        return int(i);
                    }
                }
                lastHotKey = 0xFFFFFFFF;
            }
            return -1;
        }
        lastHotKey = 0xFFFFFFFF;
        return -1;
    }
};
