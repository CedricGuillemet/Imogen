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
#include "Platform.h"
#include "Bitmap.h"
#include "Utils.h"

#include <string>
#include <functional>

#ifdef __EMSCRIPTEN__
void ImWebConsoleOutput(const char* szText)
{
    printf("%s", szText);
}

EM_JS(void, _HideLoader, (), {
    document.getElementById("loader").style.display = "none";
});
void HideLoader() { _HideLoader(); }

// https://stackoverflow.com/questions/25354313/saving-a-uint8array-to-a-binary-file
EM_JS(void, _DownloadImage, (const char *filename, int filenameLen, const char* data, int dataLen), {
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
    downloadBlob(new Uint8Array(HEAPU8.subarray(data, data + dataLen)), filenamejs, 'application/octet-stream');
    });

std::function<void(const std::string& filename)> _completeUploadCB;
EM_JS(void, _UploadDialog, (), {
document.getElementById('FileInput').click();
});
void UploadDialog(std::function<void(const std::string& filename)> completeUploadCB) 
{ 
    _completeUploadCB = completeUploadCB;
    _UploadDialog(); 
}

void DownloadImage(const char *filename, int filenameLen, const char* data, int dataLen)
{
    _DownloadImage(filename, filenameLen, data, dataLen);
}

int main_Async(int argc, char** argv);
extern "C"
{
    void EMSCRIPTEN_KEEPALIVE MountJSDirectoryDone()
    {
        main_Async(0, 0);
    }
}

void MountJSDirectory()
{
    // EM_ASM is a macro to call in-line JavaScript code.
    EM_ASM(
        // Make a directory other than '/'
        FS.mkdir('/offline');
        // Then mount with IDBFS type
        FS.mount(IDBFS, {}, '/offline');

        // Then sync
        Module.syncdone = 0;
        FS.syncfs(true, function(err) {
            if (err) {
                console.log("Error at mounting IDFS directory : " + err);
            }
            Module.syncdone = 1;
            ccall('MountJSDirectoryDone');
        });
    );
    emscripten_exit_with_live_runtime();
}

void SyncJSDirectory()
{
    EM_ASM(
        // Then sync
        if (Module.syncdone == 1) {
            Module.syncdone = 0;
            FS.syncfs(false, function(err) {
                if (err) {
                    console.log("Error at mounting IDFS directory : " + err);
                }
                Module.syncdone = 1;
            });
        }
    );
}

extern "C" 
{
    void EMSCRIPTEN_KEEPALIVE OpenFileAsync(const uint8_t *buf, int length, const uint8_t* filename, int filenameLength, int posx, int posy)
    {
        uint8_t* currentUpload = (uint8_t *)malloc(length);
        memcpy(currentUpload, buf, length);
        
        std::string filepath((const char*)filename, (size_t)filenameLength);

        Log("OpenFileAsync %s - % bytes\n", filepath.c_str(), length);
        Image image;
        if (Image::ReadMem(currentUpload, length, &image) == EVAL_OK)
        {
            Log("Image read OK from memory. Adding to cache.\n");
            gImageCache.AddImage(filepath, &image);
            _completeUploadCB(filepath);
        }
        else
        {
            Log("Unable to read image from memory.\n");
        }
        free(currentUpload);
    }
}
#else
void SyncJSDirectory()
{

}
#endif

