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

#endif

using namespace std;
typedef unsigned char uchar;
typedef unsigned int uint;

FILE* android_fopen( const char* fname, const char* mode );
GLuint LoadShader();
GLuint PostprocShader();
void DrawQuad( float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1 );
GLuint CreateTexture( uint* pixels, int w, int h );
GLuint LoadTexture( const char* fileName, uint** data = 0 );
void SetMagFilter( GLuint id, uint flag );
void loadBinaryFile( std::vector<unsigned char>& buffer, const std::string& filename );
bool LoadPNGFile( const char* fileName, uint& w, uint& h, vector<uchar>& image );

class Surface
{
public:
	Surface( const char* fileName )
	{
		vector<uchar> pixels;
		uint w = 0, h = 0;
		if (LoadPNGFile( fileName, w, h, pixels ))
		{
			width = w, height = h;
			buffer = new uint[w * h];
			uchar* s = pixels.data();
			for( uint i = 0; i < w * h; i++ ) buffer[i] = (s[i * 4 + 0] << 16) + (s[i * 4 + 1] << 8) + s[i * 4 + 2];
		}
	}
	Surface() = default;
	Surface( int w, int h ) : width( w ), height( h )
	{
		buffer = new uint[w * h];
	}
	void Clear( uint c = 0 )
	{
		if (c == 0) memset( buffer, 0, width * height * 4 );
		else
		{
			const uint s = width * height;
			for( uint i = 0; i < s; i++ ) buffer[i] = c;
		}
	}
	void CopyTo( Surface* dest, int x = 0, int y = 0 )
	{
		uint maxx = min( width, dest->width - x );
		uint maxy = min( height, dest->height - y );
		uint* d = dest->buffer + x + y * width, *s = buffer;
		for( uint y = 0; y < maxy; y++, d += dest->width, s += width ) memcpy( d, s, maxx * 4 );
	}
	void HLine( int x, int y, int l, uint c )
	{
		uint* a = buffer + x + y * width;
		for( int i = 0; i < l; i++ ) a[i] = c;
	}
	void VLine( int x, int y, int l, uint c )
	{
		uint* a = buffer + x + y * width;
		for( int i = 0; i < l; i++, a += width ) *a = c;
	}
	void Plot( int x, int y, uint c )
	{
		buffer[x + y * width] = c;
	}
	uint* buffer = 0;
	int width = 0, height = 0;
};

#include "soloud.h"
#include "soloud_wav.h"

#include "game.h"

#endif // _TEMPLATE_H
