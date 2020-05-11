#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include <errno.h>
#include <string>
#include <vector>
#include <malloc.h>
#include <iostream>
#include <fstream>

#ifdef _WIN64

#include "windows.h"
#include "direct.h"
#include "glad/glad.h"
#include "glfw/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw/glfw3native.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define MALLOC64(x) _aligned_malloc(x,64)
#define FREE64(x) _aligned_free(x)

#else

#include <jni.h>
#include <unistd.h>
#include <sys/resource.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <android/sensor.h>
#include <android/log.h>
#include "android_native_app_glue.h"

void android_fopen_set_asset_manager( AAssetManager* manager );

#define MALLOC64(x) aligned_alloc(x,64)
#define FREE64(x) aligned_free(x)

#endif

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned int Pixel;

FILE* android_fopen( const char* fname, const char* mode );
GLuint LoadShader();
GLuint PostprocShader();
void DrawQuad( float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1 );
GLuint CreateTexture( uint* pixels, int w, int h );
GLuint LoadTexture( const char* fileName, uint** data = 0 );
void SetMagFilter( GLuint id, uint flag );
void loadBinaryFile( std::vector<unsigned char>& buffer, const std::string& filename );
bool LoadPNGFile( const char* fileName, uint& w, uint& h, vector<uchar>& image );

#include "surface.h"
#include "soloud.h"
#include "soloud_wav.h"

using namespace SoLoud;

#include "game.h"

#endif // _TEMPLATE_H
