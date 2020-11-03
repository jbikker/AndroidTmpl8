#include "template.h"

// typedefs
typedef unsigned char uchar;
typedef unsigned int uint;

// namespace
using namespace std;

// game objects
static Game game;
GLuint pixels = -1, shader = -1, basic = -1;

// error reporting
Surface* errorSurf = 0;
GLuint errorPixels = -1;
bool error = false;
void FatalError( const char* err )
{
	if (error) return; // already displaying an error
	// print the error to the error surface and display this from now on
	error = true;
	errorSurf->Clear( 0 );
	int offs = 0, y = 1;
	while (strlen( err + offs ) > 53)
	{
		char t[54];
		memset( t, 0, 54 );
		memcpy( t, err + offs, 53 );
		errorSurf->Print( t, 1, y, 0xffffffff );
		y += 8, offs += 53;
	}
	errorSurf->Print( err + offs, 1, y, 0xffffffff );
}

// functions

GLuint CreateTexture( uint* pixels, int w, int h )
{
	GLuint retVal = 0;
	glGenTextures( 1, &retVal );
	glBindTexture( GL_TEXTURE_2D, retVal );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	return retVal;
}

void SetMagFilter( GLuint id, uint flag )
{
	glBindTexture( GL_TEXTURE_2D, id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flag );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flag );
}

GLuint CreateVBO( const GLfloat* data, const uint size )
{
	GLuint id;
	glGenBuffers( 1, &id );
	glBindBuffer( GL_ARRAY_BUFFER, id );
	if (data) glBufferData( GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );
	return id;
}

void BindVBO( const uint idx, const uint N, const GLuint id )
{
	glEnableVertexAttribArray( idx );
	glBindBuffer( GL_ARRAY_BUFFER, id );
	glVertexAttribPointer( idx, N, GL_FLOAT, GL_FALSE, 0, (void*)0 );
}

void DrawQuad( float u1, float v1, float u2, float v2 )
{
	GLfloat verts[] = { u1 * 2 - 1, 1 - v1 * 2, 0, u2 * 2 - 1, 1 - v1 * 2, 0,
		u1 * 2 - 1, 1 - v2 * 2, 0, u2 * 2 - 1, 1 - v1 * 2, 0,
		u1 * 2 - 1, 1 - v2 * 2, 0, u2 * 2 - 1, 1 - v2 * 2, 0 };
	GLfloat uvdata[] = { u1, v1, u2, v1, u1, v2, u2, v1, u1, v2, u2, v2 };
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, verts );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, uvdata );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );
}

int compileStatus; // can be accessed as extern
GLuint CompileShader( const char* vtext, const char* ftext )
{
	compileStatus = 0;
	GLint compiled = 0;
	compileStatus = 0;
	GLuint vertex = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertex, 1, &vtext, 0 );
	glCompileShader( vertex );
	glGetShaderiv( vertex, GL_COMPILE_STATUS, &compiled );
	if (compiled != GL_TRUE) compileStatus = 1;
	GLuint pixel = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( pixel, 1, &ftext, 0 );
	glCompileShader( pixel );
	glGetShaderiv( pixel, GL_COMPILE_STATUS, &compiled );
	if (compiled != GL_TRUE)
	{
		GLchar errString[1025];
		GLsizei length = 0;
		glGetShaderInfoLog( pixel, 1024, &length, errString );
		FatalError( errString );
		compileStatus |= 2;
	}
	GLuint retVal = glCreateProgram();
	if (retVal == 0) compileStatus |= 4;
	glAttachShader( retVal, vertex );
	glAttachShader( retVal, pixel );
	glLinkProgram( retVal );
	glGetProgramiv( retVal, GL_LINK_STATUS, &compiled );
	if (compiled != GL_TRUE) compileStatus |= 8;
	return retVal;
}

GLuint PostprocShader()
{
#ifdef _WIN64
	const char vsText[] = "#version 330\nlayout(location=0)in vec3 pos;\nlayout(location=1)in "
		"vec2 uv;\nout vec2 t;\nvoid main(){t=uv;gl_Position=vec4(pos,1);}";
	char fsText[4096] = "#version 330\nuniform sampler2D C;\nin vec2 t;\nvec2 r=vec2(320,192);\n";
#else
	char vsText[] = "attribute vec4 pos;\nattribute vec2 uv;\nvarying vec2 t; \n"
		"void main(){gl_Position=pos;t=uv;}";
	char fsText[4096] = "precision mediump float;\nuniform sampler2D C;\nvarying vec2 t;\n"
		"const vec2 r=vec2(320,192);\n";
#endif
#ifdef _WIN64
	// full version; expensive on mobile
	char fsBody[] = // by Timothy Lottes, https://www.shadertoy.com/view/ls2SRD
		"float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}vec3 ToL(vec"
		"3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}float srgb1(float c){retur"
		"n(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}vec3 srgb(vec3 c){return vec3(srgb1(c.r"
		"),srgb1(c.g),srgb1(c.b));}vec3 F(vec2 p,vec2 o){p=(p*r+o)/r;if(max(abs(p.x-0.5),abs(p.y-0."
		"5))>1.)return vec3(0);return ToL(texture2D(C,p,-16.).rgb);}vec2 D(vec2 p){p=p*r;return -(("
		"p-floor(p))-vec2(0.5));}float G(float p,float s){return exp2(s*pow(abs(p),3.));}vec3 H5(ve"
		"c2 p,float o){vec3 a=F(p,vec2(-2.,o)),b=F(p,vec2(-1.,o)),c=F(p,vec2(0.,o));vec3 d=F(p,vec2"
		"(1.,o)),e=F(p,vec2(2.,o));float l=D(p).x,wa=G(l-2.,-4.),wb=G(l-1.,-4.),wc=G(l,-4.),wd=G(l+"
		"1.,-4.),we=G(l+2.,-4.);return(a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}vec3 H7(vec2 p,f"
		"loat o){vec3 a=F(p,vec2(-3.,o)),b=F(p,vec2(-2.,o)),c=F(p,vec2(-1.,o)),d=F(p,vec2(0.,o));ve"
		"c3 e=F(p,vec2(1.,o)),f=F(p,vec2(2.,o)),g=F(p,vec2(3.,o));float l=D(p).x,wa=G(l-3.,-4.),wb="
		"G(l-2.,-4.);float wc=G(l-1.,-4.),wd=G(l,-4.),we=G(l+1.,-4.),wf=G(l+2.,-4.),wg=G(l+3.,-4.);"
		"return(a*wa+b*wb+c*wc+d*wd+e*we+f*wf+g*wg)/(wa+wb+wc+wd+we+wf+wg);}float S(vec2 p,float o)"
		"{return G(D(p).y+o,-24.);}vec3 Tri(vec2 p){return (H5(p,-2.)*S(p,-2.)+H7(p,-1.)*S(p,-1.)+H"
		"7(p,0.)*S(p,0.)+H7(p,1.)*S(p,1.)+H5(p,2.)*S(p,2.))*1.25;}vec3 M(vec2 p){p.x+=p.y*3.;vec3 m"
		"=vec3(0.65);p.x=fract(p.x/6.);if(p.x<.333)m.r=1.35;else if(p.x<0.666)m.g=1.35;else m.b=1.3"
		"5;return m;}void main(){gl_FragColor=vec4(srgb(Tri(t)*M(t*vec2(1024,640))).bgr,1);}";
#else
	// my cheap approximation
	char fsBody[] =
		"vec3 M(vec2 p){p.x+=p.y*3.;vec3 m=vec3(0.8);p.x=fract(p.x/6.);if(p.x<.333)m.r=1.2;else if("
		"p.x<0.666)m.g=1.2;else m.b=1.2;return m;}void main(){vec2 p=t*vec2(320,192);float o=abs(p."
		"y-floor(p.y)-0.5);float s=min(1.,1./(3.*o+0.3));vec2 d=vec2(s/640.,0.);vec4 pl=texture2D(C"
		",t-d),pr=texture2D(C,t+d),p0=texture2D(C,t);gl_FragColor=vec4((pl*0.25+p0*0.5+pr*0.25).bgr"
		"*s*M(t*vec2(1024,640)),1);}";
#endif
	strcat( fsText, fsBody );
	return CompileShader( vsText, fsText );
}

GLuint LoadTexture( const char* fileName, uint** data = 0 )
{
	Surface t( fileName );
	if (data)
	{
		*data = new uint[t.width * t.height];
		memcpy( *data, t.buffer, t.width * t.height * 4 );
	}
	return CreateTexture( t.buffer, t.width, t.height );
}

GLuint BasicShader()
{
#ifdef _WIN64
	const char vsText[] =
		"#version 330\nlayout(location=0)in vec3 pos;\nlayout(location=1)in vec2 uv;\n"
		"out vec2 t;\nvoid main(){t=uv;gl_Position=vec4(pos,1);}";
	char fsText[4096] =
		"#version 330\nuniform sampler2D C;\nin vec2 t;\n"
		"void main(){gl_FragColor=texture2D(C,t);}";
#else
	char vsText[] =
		"attribute vec4 pos;\nattribute vec2 uv;\nvarying vec2 t; \n"
		"void main(){gl_Position=pos;t=uv;}";
	char fsText[4096] =
		"precision mediump float;\nuniform sampler2D C;\nvarying vec2 t;\n"
		"void main(){gl_FragColor=texture2D(C,t);}";
#endif
	return CompileShader( vsText, fsText );
}

GLuint LoadShader()
{
#ifdef _WIN64
	const char vsText[] =
		"#version 330 \n"
		"layout(location=0)in vec3 pos; \n"
		"layout(location=1)in vec2 uv; \n"
		"out vec2 uvout; \n"
		"void main() \n"
		"{ \n"
		" uvout=uv; \n"
		" gl_Position=vec4(pos,1); \n"
		"} \n";
	const char fsText[] =
		"#version 330 \n"
		"uniform sampler2D C; \n"
		"uniform float a; \n"
		"in vec2 uvout; \n"
		"void main() \n"
		"{ \n"
		" vec4 k=texture2D(C,uvout); \n"
		" gl_FragColor=vec4(k.bgr,k.a*a); \n"
		"} \n";
#else
	const char vsText[] =
		"attribute vec4 pos; \n"
		"attribute vec2 uv; \n"
		"varying vec2 uvout; \n"
		"void main() \n"
		"{ \n"
		" gl_Position = pos; \n"
		" uvout = uv; \n"
		"} \n";
	const char fsText[] =
		"precision mediump float; \n"
		"uniform sampler2D C; \n"
		"uniform float a; \n"
		"varying vec2 uvout; \n"
		"void main() \n"
		"{ \n"
		" vec4 c = texture2D( C, uvout ); \n"
		" gl_FragColor = vec4( c.z, c.y, c.x, a * c.w ); \n"
		"} \n";
#endif
	return CompileShader( vsText, fsText );
}

// files

void loadBinaryFile( vector<uchar>& buffer, const string& filename )
{
#ifdef _WIN64
	FILE* f = fopen( filename.c_str(), "rb" );
#else
	FILE* f = android_fopen( filename.c_str(), "rb" );
#endif
	buffer.clear();
	char t[1028];
	while (!feof( f ))
	{
		uint bytes = (uint)fread( t, 1, 1024, f );
		for (uint i = 0; i < bytes; i++) buffer.push_back( t[i] );
	}
	fclose( f );
}

int nativeWidth = 1024, nativeHeight = 640;

void TemplateInit()
{
	// opengl state
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	// game screen
	game.screen = new Surface( 320, 192 );
	errorSurf = new Surface( 320, 192 );
	pixels = CreateTexture( game.screen->buffer, 320, 192 );
	errorPixels = CreateTexture( errorSurf->buffer, 320, 192 );
	shader = PostprocShader();
	basic = BasicShader();
	// initialize SoLoud
	game.loud.init();
}

void PostTick()
{
	if (error)
	{
		// display the error surface
		glUseProgram( basic );
		glDisable( GL_BLEND );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, errorPixels );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 320, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, errorSurf->buffer );

	}
	else
	{
		// render pixel buffer
		glUseProgram( shader );
		glDisable( GL_BLEND );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, pixels );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 320, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, game.screen->buffer );
	}
	DrawQuad();
	glEnable( GL_BLEND );
}

#ifdef _WIN64

// internal vars
static bool ldown = false;
GLFWwindow* window = 0;

// window handle access
HWND GetWindowHandle() { return glfwGetWin32Window( window ); }

// callback
void ReshapeWindowCallback( GLFWwindow* window, int w, int h )
{
	// don't resize if nothing changed or the window was minimized
	if ((nativeWidth == w && nativeHeight == h) || w == 0 || h == 0) return;
	nativeWidth = w, nativeHeight = h;
	game.SetScreenSize( w, h );
	glViewport( 0, 0, w, h );
}

// application entry point
int WinMain( HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show )
{
	// GLFW initialization
	if (!glfwInit()) return -1;
	window = glfwCreateWindow( nativeWidth, nativeHeight, "Tmpl8win", NULL, NULL );
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 );
	gladLoadGLES2Loader( (GLADloadproc)glfwGetProcAddress );
	glfwSetFramebufferSizeCallback( window, ReshapeWindowCallback );
	// go to assets folder
	_chdir( "../app/src/main/assets" );
	// application initialization
	TemplateInit();
	game.SetScreenSize( nativeWidth, nativeHeight );
	game.Init();
	// application loop
	while (!glfwWindowShouldClose( window ))
	{
		// get the mouse
		POINT p;
		GetCursorPos( &p );
		ScreenToClient( GetForegroundWindow(), &p );
		if (GetForegroundWindow() == GetWindowHandle())
		{
			game.PenPos( p.x, p.y );
			if (GetAsyncKeyState( VK_LBUTTON ))
			{
				if (!ldown) game.PenDown();
				ldown = true;
			}
			else
			{
				if (ldown) game.PenUp();
				ldown = false;
			}
		}
		// tick
		game.Tick( 0 /* for now */ );
		PostTick();
		// present
		glfwSwapBuffers( window );
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}

#include "glad/src/glad.c"

#else

// android file access

static int android_read( void* cookie, char* buf, int size )
{
	return AAsset_read( (AAsset*)cookie, buf, size );
}

static int android_write( void* cookie, const char* buf, int size )
{
	return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek( void* cookie, fpos_t offset, int whence )
{
	return AAsset_seek( (AAsset*)cookie, offset, whence );
}

static int android_close( void* cookie )
{
	AAsset_close( (AAsset*)cookie );
	return 0;
}

static AAssetManager* android_asset_manager = 0;
void android_fopen_set_asset_manager( AAssetManager* manager )
{
	android_asset_manager = manager;
}

FILE* android_fopen( const char* fname, const char* mode )
{
	if (mode[0] == 'w') return NULL;
	AAsset* asset = AAssetManager_open( android_asset_manager, fname, 0 );
	if (!asset) return NULL;
	return funopen( asset, android_read, android_write, android_seek, android_close );
}

// engine

struct saved_state { int32_t x; int32_t y; };
struct engine
{
	struct android_app* app;
	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width, height;
	saved_state state;
};

static struct android_app* theApp = 0; // for global access in this file

static void engine_init_display( struct engine* engine )
{
	const EGLint attribs[] = { // from endless tunnel
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // request OpenGL ES 2.0
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 16, EGL_NONE
	};
	EGLint w, h, format, numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;
	EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	eglInitialize( display, 0, 0 );
	eglChooseConfig( display, attribs, &config, 1, &numConfigs );
	eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &format );
	ANativeWindow_setBuffersGeometry( engine->app->window, 0, 0, format );
	surface = eglCreateWindowSurface( display, config, engine->app->window, NULL );
	EGLint attribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }; // OpenGL 2.0
	context = eglCreateContext( display, config, NULL, attribList );
	eglMakeCurrent( display, surface, surface, context );
	eglQuerySurface( display, surface, EGL_WIDTH, &w );
	eglQuerySurface( display, surface, EGL_HEIGHT, &h );
	glViewport( 0, 0, w, h ); // let's assume this never changes
	game.SetScreenSize( w, h );
	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w, engine->height = h;
	nativeWidth = w, nativeHeight = h;
}

static void engine_draw_frame( struct engine* engine )
{
	if (engine->display == NULL) return;
	game.Tick( 0 /* for now */ );
	PostTick();
	eglSwapBuffers( engine->display, engine->surface );
}

void engine_retrace()
{
	struct engine* engine = (struct engine*)theApp->userData;
	if (engine->display == NULL) return;
	struct android_poll_source* source;
	int ident, events;
	while ((ident = ALooper_pollAll( engine->animating ? 0 : -1, NULL, &events, (void**)&source )) >= 0) if (source != NULL) source->process( theApp, source );
	eglSwapBuffers( engine->display, engine->surface );
}

void engine_swapinterval( uint i )
{
	struct engine* engine = (struct engine*)theApp->userData;
	eglSwapInterval( engine->display, i );
}

static void engine_term_display( struct engine* engine )
{
	if (engine->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent( engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		if (engine->context != EGL_NO_CONTEXT) eglDestroyContext( engine->display, engine->context );
		if (engine->surface != EGL_NO_SURFACE) eglDestroySurface( engine->display, engine->surface );
		eglTerminate( engine->display );
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

static int32_t engine_handle_input( struct android_app* app, AInputEvent* event )
{
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType( event ) == AINPUT_EVENT_TYPE_MOTION)
	{
		engine->state.x = AMotionEvent_getX( event, 0 );
		engine->state.y = AMotionEvent_getY( event, 0 );
		game.PenPos( engine->state.x, engine->state.y );
		return 1;
	}
	return 0;
}

static JNIEnv* jniEnv = 0;
static android_app* androidApp;
char* dcimdir_int = new char[2048]; // to be queried as external
char* localdir = 0;

bool SetImmersiveMode( JNIEnv* env, android_app* iandroid_app )
{
	jclass ac = env->FindClass( "android/app/NativeActivity" );
	jclass wc = env->FindClass( "android/view/Window" );
	jclass vc = env->FindClass( "android/view/View" );
	jmethodID getWindow = env->GetMethodID( ac, "getWindow", "()Landroid/view/Window;" );
	jmethodID getDecorView = env->GetMethodID( wc, "getDecorView", "()Landroid/view/View;" );
	jmethodID setSysVis = env->GetMethodID( vc, "setSystemUiVisibility", "(I)V" );
	jmethodID getSystemUiVisibility = env->GetMethodID( vc, "getSystemUiVisibility", "()I" );
	jobject windowObj = env->CallObjectMethod( iandroid_app->activity->clazz, getWindow );
	jobject decorViewObj = env->CallObjectMethod( windowObj, getDecorView );
	// get flags
	int f1 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_LAYOUT_STABLE", "I" ) );
	int f2 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION", "I" ) );
	int f3 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN", "I" ) );
	int f4 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I" ) );
	int f5 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_FULLSCREEN", "I" ) );
	int f6 = env->GetStaticIntField( vc, env->GetStaticFieldID( vc, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I" ) );
	// get current immersiveness
	const int currentVisibility = env->CallIntMethod( decorViewObj, getSystemUiVisibility );
	bool is1 = (currentVisibility & f1) != 0, is2 = (currentVisibility & f2) != 0;
	bool is3 = (currentVisibility & f3) != 0, is4 = (currentVisibility & f4) != 0;
	bool is5 = (currentVisibility & f5) != 0, is6 = (currentVisibility & f6) != 0;
	bool isAlreadyImmersive = is1 && is2 && is3 && is4 && is5 && is6;
	bool success = true;
	if (!isAlreadyImmersive)
	{
		env->CallVoidMethod( decorViewObj, setSysVis, f1 | f2 | f3 | f4 | f5 | f6 );
		if (env->ExceptionCheck())
		{
			// Read exception msg
			jthrowable e = env->ExceptionOccurred();
			env->ExceptionClear(); // clears the exception; e seems to remain valid
			jclass clazz = env->GetObjectClass( e );
			jmethodID getMessage = env->GetMethodID( clazz, "getMessage", "()Ljava/lang/String;" );
			jstring message = (jstring)env->CallObjectMethod( e, getMessage );
			const char* mstr = env->GetStringUTFChars( message, NULL );
			const auto exception_msg = std::string{ mstr };
			env->ReleaseStringUTFChars( message, mstr );
			env->DeleteLocalRef( message );
			env->DeleteLocalRef( clazz );
			env->DeleteLocalRef( e );
			success = false;
		}
	}
	env->DeleteLocalRef( windowObj );
	env->DeleteLocalRef( decorViewObj );
	return success;
}

static JNIEnv* GetJniEnv()
{
	if (!jniEnv) androidApp->activity->vm->AttachCurrentThread( &jniEnv, NULL );
	return jniEnv;
}

static void engine_handle_cmd( struct android_app* app, int32_t cmd )
{
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE: // save current state
	    game.SaveState( engine->app->savedState, engine->app->savedStateSize );
		break;
	case APP_CMD_INIT_WINDOW: // do things when the window becomes visible
		if (engine->app->window != NULL)
		{
			engine_init_display( engine );
			TemplateInit();
			game.Init();
			engine_draw_frame( engine );
			engine->animating = 1;
		}
		break;
	case APP_CMD_TERM_WINDOW: // do things when the window is closed or hidden
		engine_term_display( engine );
		engine->animating = 0;
		break;
	case APP_CMD_GAINED_FOCUS: // do things when we gain focus
		androidApp->activity->vm->AttachCurrentThread( &jniEnv, NULL );
		SetImmersiveMode( GetJniEnv(), androidApp );
		engine->animating = 1;
		break;
	case APP_CMD_LOST_FOCUS:
		engine->animating = 0;
		break;
	case APP_CMD_PAUSE:
		engine->animating = 0;
		break;
	case APP_CMD_RESUME:
		androidApp->activity->vm->AttachCurrentThread( &jniEnv, NULL );
		engine->animating = 1;
		SetImmersiveMode( GetJniEnv(), androidApp );
		break;
	}
}

static void GetDCIMPath( JNIEnv* env, android_app* app, const char* param, char* dst )
{
	// from: https://stackoverflow.com/questions/29998820/dcim-directory-path-on-android-return-value
	jclass ec = env->FindClass( "android/os/Environment" );
	jmethodID getESD = env->GetStaticMethodID( ec,
		"getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;" );
	jfieldID fieldId = env->GetStaticFieldID( ec, param, "Ljava/lang/String;" );
	jstring jstrParam = (jstring)env->GetStaticObjectField( ec, fieldId );
	jobject extStorageFile = env->CallStaticObjectMethod( ec, getESD, jstrParam );
	jmethodID getPM = env->GetMethodID( env->FindClass( "java/io/File" ), "getPath", "()Ljava/lang/String;" );
	jstring extStoragePath = (jstring)env->CallObjectMethod( extStorageFile, getPM );
	const char* extStoragePathString = env->GetStringUTFChars( extStoragePath, NULL );
	strcpy( dst, extStoragePathString );
	env->ReleaseStringUTFChars( extStoragePath, extStoragePathString );
	app->activity->vm->DetachCurrentThread();
}

void android_main( struct android_app* state )
{
	theApp = state;
	android_fopen_set_asset_manager( state->activity->assetManager );
	androidApp = state;
	SetImmersiveMode( GetJniEnv(), state );
	GetDCIMPath( GetJniEnv(), state, "DIRECTORY_DCIM", dcimdir_int );
	localdir = new char[strlen( state->activity->internalDataPath ) + 1];
	strcpy( localdir, state->activity->internalDataPath );
	struct engine engine;
	memset( &engine, 0, sizeof( engine ) );
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;
	if (state->savedState != NULL) game.RestoreState( state->savedState, state->savedStateSize );
	engine.animating = 1;
	while (1)
	{
		int ident, events;
		struct android_poll_source* source;
		while ((ident = ALooper_pollAll( engine.animating ? 0 : -1, NULL, &events,
			(void**)&source )) >= 0)
		{
			if (source != NULL) source->process( state, source );
			if (state->destroyRequested != 0)
			{
				game.Shutdown();
				engine_term_display( &engine );
				return;
			}
		}
		if (engine.animating) engine_draw_frame( &engine );
	}
}

#endif