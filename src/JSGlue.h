// https://github.com/CedricGuillemet/Imogen
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

void ImWebConsoleOutput(const char* szText)
{
    printf("%s", szText);
}

EM_JS(void, HideLoader, (), {
    document.getElementById("loader").style.display = "none";
});

// https://stackoverflow.com/questions/25354313/saving-a-uint8array-to-a-binary-file
EM_JS(void, DownloadImage, (const char *filename, int filenameLen, const char* text, int textLen), {
    var filenamejs = String.fromCharCode.apply(null, new Uint8Array(HEAPU8.subarray(filename, filename + filenameLen)));
    var downloadBlob, downloadURL;

    downloadBlob = function(data, fileName, mimeType) {
        var blob, url;
        blob = new Blob([data], {
            type: mimeType
        });
        url = window.URL.createObjectURL(blob);
        downloadURL(url, fileName);
        setTimeout(function() {
            return window.URL.revokeObjectURL(url);
        }, 1000);
        };

    downloadURL = function(data, fileName) {
        var a;
        a = document.createElement('a');
        a.href = data;
        a.download = fileName;
        document.body.appendChild(a);
        a.style = 'display: none';
        a.click();
        a.remove();
        };
    downloadBlob(new Uint8Array(HEAPU8.subarray(text, text + textLen)), filenamejs, 'application/octet-stream');
    });

EM_JS(void, UploadDialog, (), {
document.getElementById('FileInput').click();
});

uint8_t *currentUpload = 0;
int currentUploadLength = 0;
extern "C" {
    void EMSCRIPTEN_KEEPALIVE OpenFileAsync(const uint8_t *buf, int length, const uint8_t* filename, int filenameLength, int posx, int posy)
    {
        //std::cout << "OpenFileAsync called " << " , " << int(buf) << " , " << length << " - " << std::string((const char*)filename, filenameLength) << " - " << posx << " , " << posy << std::endl;
        /*char tmps[512];
        sprintf(tmps, "data %c %c %c %c", buf[0], buf[1], buf[2], buf[3]);
        std::cout << std::string(tmps) << std::endl;
        */
        currentUpload = (uint8_t *)malloc(length);
        memcpy(currentUpload, buf, length);
        currentUploadLength = length;
    }
}