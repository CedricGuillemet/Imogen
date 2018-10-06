int Log(const char *szFormat, ...);

typedef struct Image_t
{
	int width, height;
	int components;
	void *bits;
} Image;

typedef struct Evaluation_t
{
	int targetIndex;
	int forcedDirty;
	int uiPass;
	int padding;
	float mouse[4];
	int inputIndices[8];	
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

// call FreeImage when done
int ReadImage(char *filename, Image *image);
// writes an allocated image
int WriteImage(char *filename, Image *image, int format, int quality);
// call FreeImage when done
int GetEvaluationImage(int target, Image *image);
// 
int SetEvaluationImage(int target, Image *image);
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

#define EVAL_OK 0
#define EVAL_ERR 1