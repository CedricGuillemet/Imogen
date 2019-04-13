int Log(const char *szFormat, ...);
char * strcpy (char * destination, const char * source);
int strcmp(char *str1, char *str2);
int strlen (const char * str);
float fabsf(float value);
float log2(float);

typedef struct Image_t
{
	void *decoder;
	int width, height;
	//int components;
	int mDataSize;
	unsigned char mNumMips;
	unsigned char mNumFaces;
	unsigned char mFormat;
	void *bits;
} Image;

int DirtyInput = (1 << 0);
int DirtyParameter = (1 << 1);
int DirtyMouse = (1 << 2);
int DirtyCamera = (1 << 3);
int DirtyTime = (1 << 4);
int DirtySampler = (1 << 5);


typedef struct Evaluation_t
{
	float inv_view_rot[16];
	float viewProjection[16];
	float viewInverse[16];
	float model[16];
	float modelViewProjection[16];
	float viewport[4];
	
	int targetIndex;
	int forcedDirty;
	int uiPass;
	int passNumber;
	float mouse[4];
	int keyModifier[4];
	int inputIndices[8];	
	
	int frame;
	int localFrame;
	int vertexSpace;
	int dirtyFlag;
	
    int mipmapNumber;
    int mipmapCount;
} Evaluation;

enum BlendOp
{
	ZERO,
	ONE, 
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR, 
	DST_COLOR, 
	ONE_MINUS_DST_COLOR, 
	SRC_ALPHA, 
	ONE_MINUS_SRC_ALPHA, 
	DST_ALPHA, 
	ONE_MINUS_DST_ALPHA, 
	CONSTANT_COLOR, 
	ONE_MINUS_CONSTANT_COLOR, 
	CONSTANT_ALPHA, 
	ONE_MINUS_CONSTANT_ALPHA, 
	SRC_ALPHA_SATURATE,
	BLEND_LAST
};

enum ImageFormat
{
	BGR8,
	RGB8,
	RGB16,
	RGB16F,
	RGB32F,
	RGBE,

	BGRA8,
	RGBA8,
	RGBA16,
	RGBA16F,
	RGBA32F,

	RGBM,

	ImageFormatCount
};

enum CubeMapFace
{
	CUBEMAP_POSX,
	CUBEMAP_NEGX,
	CUBEMAP_POSY,
	CUBEMAP_NEGY,
	CUBEMAP_POSZ,
	CUBEMAP_NEGZ,
};

int vertexSpace_UV = 0;
int vertexSpace_World = 1;

// call FreeImage when done
int ReadImage(void* context, char *filename, Image *image);
// writes an allocated image
int WriteImage(void* context, char *filename, Image *image, int format, int quality);
// call FreeImage when done
int GetEvaluationImage(void* context, int target, Image *image);
// 
int SetEvaluationImage(void* context, int target, Image *image);
int SetEvaluationImageCube(void* context, int target, Image *image, int cubeFace);
// call FreeImage when done
// set the bits pointer with an allocated memory
int AllocateImage(Image *image);
int FreeImage(Image *image);
int LoadSVG(const char *filename, Image *image, float dpi);

// Image resize
// Image thumbnail
int SetThumbnailImage(void *context, Image *image);

// force evaluation of a target with a specified size
// no guarantee that the resulting Image will have that size.
int Evaluate(void *context, int target, int width, int height, Image *image);

void SetBlendingMode(void *context, int target, int blendSrc, int blendDst);
void EnableDepthBuffer(void *context, int target, int enable);
void EnableFrameClear(void *context, int target, int enable);

void SetVertexSpace(void *context, int target, int vertexMode);

int GetEvaluationSize(void *context, int target, int *imageWidth, int *imageHeight);
int SetEvaluationSize(void *context, int target, int imageWidth, int imageHeight);
int SetEvaluationCubeSize(void *context, int target, int faceWidth, int mipmapCount);

int OverrideInput(void *context, int target, int inputIndex, int newInputTarget);
int CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias);

int Job(void *context, int(*jobFunction)(void*), void *ptr, unsigned int size);
int JobMain(void *context, int(*jobMainFunction)(void*), void *ptr, unsigned int size);
// processing values:
// 0 : no more processing, display node as normal
// 1 : processing with an animation for node display
// 2 : display node as normal despite it processing 
void SetProcessing(void *context, int target, int processing);

// compute shader memory allocation
int AllocateComputeBuffer(void *context, int target, int elementCount, int elementSize);

int LoadScene(const char *filename, void **scene);
int SetEvaluationScene(void *context, int target, void *scene);
int GetEvaluationScene(void *context, int target, void **scene);
char* GetEvaluationSceneName(void *context, int target);
int GetEvaluationRenderer(void *context, int target, void **renderer);
int InitRenderer(void *context, int target, int mode, void *scene);
int UpdateRenderer(void *context, int target);

int ReadGLTF(void *evaluationContext, char *filename, void **scene);

	
#define EVAL_OK 0
#define EVAL_ERR 1
#define EVAL_DIRTY 2
