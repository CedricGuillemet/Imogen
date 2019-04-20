// https://github.com/CedricGuillemet/imgInspect
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
/*
example 
Image pickerImage;
ImGui::ImageButton(pickerImage.textureID, ImVec2(pickerImage.mWidth, pickerImage.mHeight));
ImRect rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
ImVec2 mouseUVCoord = (io.MousePos - rc.Min) / rc.GetSize();
mouseUVCoord.y = 1.f - mouseUVCoord.y;
			
if (io.KeyShift && io.MouseDown[0] && mouseUVCoord.x >= 0.f && mouseUVCoord.y >= 0.f)
{
	int width = pickerImage.mWidth;
	int height = pickerImage.mHeight;

	imageInspect(width, height, pickerImage.GetBits(), mouseUVCoord, displayedTextureSize);
}
*/
inline void imageInspect(int width, int height, unsigned char* bits, ImVec2 mouseUVCoord, ImVec2 displayedTextureSize)
{
    ImGui::BeginTooltip();
    ImGui::BeginGroup();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton("AnotherInvisibleMan", ImVec2(120, 120));
    ImRect pickRc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    draw_list->AddRectFilled(pickRc.Min, pickRc.Max, 0xFF000000);
    static int zoomSize = 2;
    float quadWidth = 120.f / float(zoomSize * 2 + 1);
    ImVec2 quadSize(quadWidth, quadWidth);
    int basex = ImClamp(int(mouseUVCoord.x * width), zoomSize, width - zoomSize);
    int basey = ImClamp(int(mouseUVCoord.y * height) - zoomSize * 2 - 1, zoomSize, height - zoomSize);
    for (int y = -zoomSize; y <= zoomSize; y++)
    {
        for (int x = -zoomSize; x <= zoomSize; x++)
        {
            uint32_t texel = ((uint32_t*)bits)[(basey + zoomSize * 2 + 1 - y) * width + x + basex];
            ImVec2 pos = pickRc.Min + ImVec2(float(x + zoomSize), float(y + zoomSize)) * quadSize;
            draw_list->AddRectFilled(pos, pos + quadSize, texel);
        }
    }
    ImVec2 pos = pickRc.Min + ImVec2(float(zoomSize), float(zoomSize)) * quadSize;
    draw_list->AddRect(pos, pos + quadSize, 0xFF0000FF, 0.f, 15, 2.f);

    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    uint32_t texel = ((uint32_t*)bits)[basey * width + basex];
    ImVec4 color = ImColor(texel);
    ImVec4 colHSV;
    ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, colHSV.x, colHSV.y, colHSV.z);
    ImGui::Text("U %1.3f V %1.3f", mouseUVCoord.x, mouseUVCoord.y);
    ImGui::Text("Coord %d %d", int(mouseUVCoord.x * width), int(mouseUVCoord.y * height));
    ImGui::Separator();
    ImGui::Text("R 0x%02x  G 0x%02x  B 0x%02x", int(color.x * 255.f), int(color.y * 255.f), int(color.z * 255.f));
    ImGui::Text("R %1.3f G %1.3f B %1.3f", color.x, color.y, color.z);
    ImGui::Separator();
    ImGui::Text("H 0x%02x  S 0x%02x  V 0x%02x", int(colHSV.x * 255.f), int(colHSV.y * 255.f), int(colHSV.z * 255.f));
    ImGui::Text("H %1.3f S %1.3f V %1.3f", colHSV.x, colHSV.y, colHSV.z);
    ImGui::Separator();
    ImGui::Text("Alpha 0x%02x", int(color.w * 255.f));
    ImGui::Text("Alpha %1.3f", color.w);
    ImGui::Separator();
    ImGui::Text("Size %d, %d", int(displayedTextureSize.x), int(displayedTextureSize.y));
    ImGui::EndGroup();
    ImGui::EndTooltip();
}
