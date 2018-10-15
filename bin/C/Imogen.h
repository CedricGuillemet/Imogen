int Log(const char *szFormat, ...);

typedef struct Image_t
{
	void *bits;
	int width, height;
	//int components;
	int mDataSize;
	unsigned char mNumMips;
	unsigned char mNumFaces;
	unsigned char mFormat;
} Image;

typedef struct Evaluation_t
{
	int targetIndex;
	int forcedDirty;
	int uiPass;
	int padding;
	float mouse[4];
	int inputIndices[8];	
	float inv_proj[16];
	float inv_view_rot[9];
	float viewport[2];
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

// call FreeImage when done
int ReadImage(char *filename, Image *image);
// writes an allocated image
int WriteImage(char *filename, Image *image, int format, int quality);
// call FreeImage when done
int GetEvaluationImage(int target, Image *image);
// 
int SetEvaluationImage(int target, Image *image);
int SetEvaluationImageCube(int target, Image *image, int cubeFace);
// call FreeImage when done
// set the bits pointer with an allocated memory
int AllocateImage(Image *image);
int FreeImage(Image *image);

// Image resize
// Image thumbnail
int SetThumbnailImage(Image *image);

// force evaluation of a target with a specified size
// no guarantee that the resulting Image will have that size.
int Evaluate(int target, int width, int height, Image *image);

void SetBlendingMode(int target, int blendSrc, int blendDst);
int GetEvaluationSize(int target, int *imageWidth, int *imageHeight);
int SetEvaluationSize(int target, int imageWidth, int imageHeight);
int CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias);

int Job(int(*jobFunction)(void*), void *ptr, unsigned int size);
int JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size);

#define EVAL_OK 0
#define EVAL_ERR 1