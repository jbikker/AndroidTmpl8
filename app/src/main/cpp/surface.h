#ifndef _SURFACE_H
#define _SURFACE_H

#define REDMASK	(0xff0000)
#define GREENMASK (0x00ff00)
#define BLUEMASK (0x0000ff)

typedef unsigned int Pixel; // unsigned int is assumed to be 32-bit, which seems a safe assumption.

inline Pixel AddBlend( Pixel color1, Pixel color2 )
{
	const unsigned int r = (color1 & REDMASK) + (color2 & REDMASK);
	const unsigned int g = (color1 & GREENMASK) + (color2 & GREENMASK);
	const unsigned int b = (color1 & BLUEMASK) + (color2 & BLUEMASK);
	const unsigned r1 = (r & REDMASK) | (REDMASK * (r >> 24));
	const unsigned g1 = (g & GREENMASK) | (GREENMASK * (g >> 16));
	const unsigned b1 = (b & BLUEMASK) | (BLUEMASK * (b >> 8));
	return (r1 + g1 + b1);
}

// subtractive blending
inline Pixel SubBlend( Pixel color1, Pixel color2 )
{
	int red = (color1 & REDMASK) - (color2 & REDMASK);
	int green = (color1 & GREENMASK) - (color2 & GREENMASK);
	int blue = (color1 & BLUEMASK) - (color2 & BLUEMASK);
	if (red < 0) red = 0;
	if (green < 0) green = 0;
	if (blue < 0) blue = 0;
	return (Pixel)(red + green + blue);
}

class Surface
{
	enum { OWNER = 1 };
public:
	// constructor / destructor
	Surface() = default;
	Surface( int w, int h, Pixel* b ) : width( w ), height( h ), buffer( b ) {}
	Surface( int w, int h ) : width( w ), height( h ), buffer( new Pixel[w * h] ), flags( OWNER ) {}
	Surface( char* file );
	~Surface();
	// member data access
	Pixel* GetBuffer() { return buffer; }
	void SetBuffer( Pixel* b ) { buffer = b; }
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	// Special operations
	void InitCharset();
	void SetChar( int c, char* c1, char* c2, char* c3, char* c4, char* c5 );
	void Centre( const char* s, int y1, Pixel color );
	void Print( const char* s, int x1, int y1, Pixel color );
	void Clear( Pixel color );
	void Line( float x1, float y1, float x2, float y2, Pixel color );
	void HLine( int x1, int y1, int l, Pixel color );
	void VLine( int x1, int y1, int l, Pixel color );
	void Plot( int x, int y, Pixel c );
	void LoadImage( char* file );
	void CopyTo( Surface* dst, int x, int y );
	void BlendCopyTo( Surface* dst, int x, int y );
	void ScaleColor( unsigned int scale );
	void Box( int x1, int y1, int x2, int y2, Pixel color );
	void Bar( int x1, int y1, int x2, int y2, Pixel color );
	void Resize( Surface* orig );
	// attributes
	Pixel* buffer = 0;	
	int width = 0, height = 0;
	int flags;
	// static attributes for the builtin font
	inline static char font[51][5][6];
	inline static bool fontInitialized = false;
	inline static int transl[256];
};

class Sprite
{
public:
	// Sprite flags
	enum
	{
		FLARE		= (1<< 0),
		OPFLARE		= (1<< 1),	
		FLASH		= (1<< 4),	
		DISABLED	= (1<< 6),	
		GMUL		= (1<< 7),
		BLACKFLARE	= (1<< 8),	
		BRIGHTEST   = (1<< 9),
		RFLARE		= (1<<12),
		GFLARE		= (1<<13),
		NOCLIP		= (1<<14)
	};
	
	// Structors
	Sprite( Surface* surface, unsigned int frames );
	~Sprite();
	// Methods
	void Draw( Surface* target, int x, int y );
	void DrawScaled( int x, int y, int w, int h, Surface* target );
	void SetFlags( uint f ) { flags = f; }
	void SetFrame( uint i ) { currentFrame = i; }
	unsigned int GetFlags() const { return flags; }
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	Pixel* GetBuffer() { return surface->GetBuffer(); }	
	unsigned int Frames() { return numFrames; }
	Surface* GetSurface() { return surface; }
private:
	// Methods
	void InitializeStartData();
	// Attributes
	int width = 0, height = 0, m_Pitch;
	unsigned int numFrames = 1;
	unsigned int currentFrame = 0;       
	unsigned int flags = 0;
	unsigned int** start = 0;
	Surface* surface = 0;
};

class Font
{
public:
	Font() = default;
	Font( char* file, char* chars );
	~Font();
	void Print( Surface* target, char* text, int x, int y, bool clip = false );
	void Centre( Surface* target, char* text, int y );
	int Width( char* text );
	int Height() { return surface->GetHeight(); }
	void YClip( int y1, int y2 ) { cy1 = y1; cy2 = y2; }
private:
	Surface* surface = 0;
	int* offset = 0, *width = 0, *trans = 0, height, cy1, cy2;
};

#endif