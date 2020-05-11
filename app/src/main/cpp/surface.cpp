#include "template.h"

// -----------------------------------------------------------
// True-color surface class implementation
// -----------------------------------------------------------

Surface::Surface( char* file )
{
	LoadImage( file );
}

void Surface::LoadImage( char* file )
{
	vector<uchar> pixels;
	uint w = 0, h = 0;
	if (LoadPNGFile( file, w, h, pixels ))
	{
		width = w, height = h;
		buffer = new Pixel[w * h];
		uchar* s = pixels.data();
		for (uint i = 0; i < w * h; i++) buffer[i] = (s[i * 4 + 0] << 16) + (s[i * 4 + 1] << 8) + s[i * 4 + 2];
	}
}

Surface::~Surface()
{
	if ((flags & OWNER) == 0) return; // only delete if the buffer was not passed to us
	delete buffer;
}

void Surface::Clear( Pixel color )
{
	int s = width * height;
	for (int i = 0; i < s; i++) buffer[i] = color;
}

void Surface::Centre( const char* s, int y1, Pixel color )
{
	int x = (width - (int)strlen( s ) * 6) / 2;
	Print( s, x, y1, color );
}

void Surface::Print( const char* s, int x1, int y1, Pixel color )
{
	if (!fontInitialized)
	{
		InitCharset();
		fontInitialized = true;
	}
	Pixel* t = buffer + x1 + y1 * width;
	for (int i = 0; i < (int)(strlen( s )); i++, t += 6)
	{
		long pos = 0;
		if ((s[i] >= 'A') && (s[i] <= 'Z')) pos = transl[(unsigned short)(s[i] - ('A' - 'a'))];
		else pos = transl[(unsigned short)s[i]];
		Pixel* a = t;
		char* c = (char*)font[pos];
		for (int v = 0; v < 5; v++, c++, a += width)
			for (int h = 0; h < 5; h++) if (*c++ == 'o') *(a + h) = color, * (a + h + width) = 0;
	}
}

void Surface::Resize( Surface* orig )
{
	Pixel* src = orig->GetBuffer(), * dst = buffer;
	int u, v, owidth = orig->GetWidth(), oheight = orig->GetHeight();
	int dx = (owidth << 10) / width, dy = (oheight << 10) / height;
	for (v = 0; v < height; v++)
	{
		for (u = 0; u < width; u++)
		{
			int su = u * dx, sv = v * dy;
			Pixel* s = src + (su >> 10) + (sv >> 10) * owidth;
			int ufrac = su & 1023, vfrac = sv & 1023;
			int w4 = (ufrac * vfrac) >> 12;
			int w3 = ((1023 - ufrac) * vfrac) >> 12;
			int w2 = (ufrac * (1023 - vfrac)) >> 12;
			int w1 = ((1023 - ufrac) * (1023 - vfrac)) >> 12;
			int x2 = ((su + dx) > ( (owidth - 1) << 10 )) ? 0 : 1;
			int y2 = ((sv + dy) > ((oheight - 1) << 10)) ? 0 : 1;
			Pixel p1 = *s, p2 = *(s + x2), p3 = *(s + owidth * y2), p4 = *(s + owidth * y2 + x2);
			unsigned int r = (((p1 & REDMASK) * w1 + (p2 & REDMASK) * w2 + (p3 & REDMASK) * w3 + (p4 & REDMASK) * w4) >> 8) & REDMASK;
			unsigned int g = (((p1 & GREENMASK) * w1 + (p2 & GREENMASK) * w2 + (p3 & GREENMASK) * w3 + (p4 & GREENMASK) * w4) >> 8) & GREENMASK;
			unsigned int b = (((p1 & BLUEMASK) * w1 + (p2 & BLUEMASK) * w2 + (p3 & BLUEMASK) * w3 + (p4 & BLUEMASK) * w4) >> 8) & BLUEMASK;
			*(dst + u + v * width) = (Pixel)(r + g + b);
		}
	}
}


void Surface::Line( float x1, float y1, float x2, float y2, Pixel c )
{
#define OUTCODE(x,y) (((x)<xmin)?1:(((x)>xmax)?2:0))+(((y)<ymin)?4:(((y)>ymax)?8:0))
	// clip (Cohen-Sutherland, https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm)
	const float xmin = 0, ymin = 0, xmax = width - 1.f, ymax = height - 1.f;
	int c0 = OUTCODE( x1, y1 ), c1 = OUTCODE( x2, y2 );
	bool accept = false;
	while (1)
	{
		if (!(c0 | c1)) { accept = true; break; }
		else if (c0 & c1) break; else
		{
			float x, y;
			const int co = c0 ? c0 : c1;
			if (co & 8) x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1), y = ymax;
			else if (co & 4) x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1), y = ymin;
			else if (co & 2) y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1), x = xmax;
			else if (co & 1) y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1), x = xmin;
			if (co == c0) x1 = x, y1 = y, c0 = OUTCODE( x1, y1 );
			else x2 = x, y2 = y, c1 = OUTCODE( x2, y2 );
		}
	}
	if (!accept) return;
	float b = x2 - x1;
	float h = y2 - y1;
	float l = fabsf( b );
	if (fabsf( h ) > l) l = fabsf( h );
	int il = (int)l;
	float dx = b / (float)l;
	float dy = h / (float)l;
	for (int i = 0; i <= il; i++)
	{
		*(buffer + (int)x1 + (int)y1 * width) = c;
		x1 += dx, y1 += dy;
	}
}

void Surface::HLine( int x, int y, int l, Pixel color )
{
	uint* a = buffer + x + y * width;
	for (int i = 0; i < l; i++) a[i] = color;
}

void Surface::VLine( int x, int y, int l, Pixel color )
{
	uint* a = buffer + x + y * width;
	for (int i = 0; i < l; i++, a += width) *a = color;
}

void Surface::Plot( int x, int y, Pixel c )
{
	if ((x >= 0) && (y >= 0) && (x < width) && (y < height))
		buffer[x + y * width] = c;
}

void Surface::Box( int x1, int y1, int x2, int y2, Pixel c )
{
	Line( (float)x1, (float)y1, (float)x2, (float)y1, c );
	Line( (float)x2, (float)y1, (float)x2, (float)y2, c );
	Line( (float)x1, (float)y2, (float)x2, (float)y2, c );
	Line( (float)x1, (float)y1, (float)x1, (float)y2, c );
}

void Surface::Bar( int x1, int y1, int x2, int y2, Pixel c )
{
	Pixel* a = x1 + y1 * width + buffer;
	for (int y = y1; y <= y2; y++)
	{
		for (int x = 0; x <= (x2 - x1); x++) a[x] = c;
		a += width;
	}
}

void Surface::CopyTo( Surface* dst, int x, int y )
{
	Pixel* d = dst->GetBuffer();
	Pixel* s = buffer;
	if (s == 0 || d == 0) return;
	int srcwidth = width;
	int srcheight = height;
	int dstwidth = dst->GetWidth();
	int dstheight = dst->GetHeight();
	if ((srcwidth + x) > dstwidth) srcwidth = dstwidth - x;
	if ((srcheight + y) > dstheight) srcheight = dstheight - y;
	if (x < 0) s -= x, srcwidth += x, x = 0;
	if (y < 0) s -= y * srcwidth, srcheight += y, y = 0;
	if (srcwidth <= 0 || srcheight <= 0) return;
	d += x + dstwidth * y;
	for (int l = 0; l < srcheight; l++, d += dstwidth, s += srcwidth ) memcpy( d, s, srcwidth * 4 );
}

void Surface::BlendCopyTo( Surface* dst, int x, int y )
{
	Pixel* d = dst->GetBuffer();
	Pixel* s = buffer;
	if (s == 0 || d == 0) return;
	int srcwidth = width;
	int srcheight = height;
	int dstwidth = dst->GetWidth();
	int dstheight = dst->GetHeight();
	if ((srcwidth + x) > dstwidth) srcwidth = dstwidth - x;
	if ((srcheight + y) > dstheight) srcheight = dstheight - y;
	if (x < 0) s -= x, srcwidth += x, x = 0;
	if (y < 0) s -= y * srcwidth, srcheight += y, y = 0;
	if ((srcwidth > 0) && (srcheight > 0))
	{
		d += x + dstwidth * y;
		for (int y = 0; y < srcheight; y++)
		{
			for (int x = 0; x < srcwidth; x++) d[x] = AddBlend( d[x], s[x] );
			d += dstwidth;
			s += srcwidth;
		}
	}
}

void Surface::SetChar( int c, char* c1, char* c2, char* c3, char* c4, char* c5 )
{
	strcpy( font[c][0], c1 );
	strcpy( font[c][1], c2 );
	strcpy( font[c][2], c3 );
	strcpy( font[c][3], c4 );
	strcpy( font[c][4], c5 );
}

void Surface::InitCharset()
{
	SetChar( 0, ":ooo:", "o:::o", "ooooo", "o:::o", "o:::o" );
	SetChar( 1, "oooo:", "o:::o", "oooo:", "o:::o", "oooo:" );
	SetChar( 2, ":oooo", "o::::", "o::::", "o::::", ":oooo" );
	SetChar( 3, "oooo:", "o:::o", "o:::o", "o:::o", "oooo:" );
	SetChar( 4, "ooooo", "o::::", "oooo:", "o::::", "ooooo" );
	SetChar( 5, "ooooo", "o::::", "ooo::", "o::::", "o::::" );
	SetChar( 6, ":oooo", "o::::", "o:ooo", "o:::o", ":ooo:" );
	SetChar( 7, "o:::o", "o:::o", "ooooo", "o:::o", "o:::o" );
	SetChar( 8, "::o::", "::o::", "::o::", "::o::", "::o::" );
	SetChar( 9, ":::o:", ":::o:", ":::o:", ":::o:", "ooo::" );
	SetChar( 10, "o::o:", "o:o::", "oo:::", "o:o::", "o::o:" );
	SetChar( 11, "o::::", "o::::", "o::::", "o::::", "ooooo" );
	SetChar( 12, "oo:o:", "o:o:o", "o:o:o", "o:::o", "o:::o" );
	SetChar( 13, "o:::o", "oo::o", "o:o:o", "o::oo", "o:::o" );
	SetChar( 14, ":ooo:", "o:::o", "o:::o", "o:::o", ":ooo:" );
	SetChar( 15, "oooo:", "o:::o", "oooo:", "o::::", "o::::" );
	SetChar( 16, ":ooo:", "o:::o", "o:::o", "o::oo", ":oooo" );
	SetChar( 17, "oooo:", "o:::o", "oooo:", "o:o::", "o::o:" );
	SetChar( 18, ":oooo", "o::::", ":ooo:", "::::o", "oooo:" );
	SetChar( 19, "ooooo", "::o::", "::o::", "::o::", "::o::" );
	SetChar( 20, "o:::o", "o:::o", "o:::o", "o:::o", ":oooo" );
	SetChar( 21, "o:::o", "o:::o", ":o:o:", ":o:o:", "::o::" );
	SetChar( 22, "o:::o", "o:::o", "o:o:o", "o:o:o", ":o:o:" );
	SetChar( 23, "o:::o", ":o:o:", "::o::", ":o:o:", "o:::o" );
	SetChar( 24, "o:::o", "o:::o", ":oooo", "::::o", ":ooo:" );
	SetChar( 25, "ooooo", ":::o:", "::o::", ":o:::", "ooooo" );
	SetChar( 26, ":ooo:", "o::oo", "o:o:o", "oo::o", ":ooo:" );
	SetChar( 27, "::o::", ":oo::", "::o::", "::o::", ":ooo:" );
	SetChar( 28, ":ooo:", "o:::o", "::oo:", ":o:::", "ooooo" );
	SetChar( 29, "oooo:", "::::o", "::oo:", "::::o", "oooo:" );
	SetChar( 30, "o::::", "o::o:", "ooooo", ":::o:", ":::o:" );
	SetChar( 31, "ooooo", "o::::", "oooo:", "::::o", "oooo:" );
	SetChar( 32, ":oooo", "o::::", "oooo:", "o:::o", ":ooo:" );
	SetChar( 33, "ooooo", "::::o", ":::o:", "::o::", "::o::" );
	SetChar( 34, ":ooo:", "o:::o", ":ooo:", "o:::o", ":ooo:" );
	SetChar( 35, ":ooo:", "o:::o", ":oooo", "::::o", ":ooo:" );
	SetChar( 36, "::o::", "::o::", "::o::", ":::::", "::o::" );
	SetChar( 37, ":ooo:", "::::o", ":::o:", ":::::", "::o::" );
	SetChar( 38, ":::::", ":::::", "::o::", ":::::", "::o::" );
	SetChar( 39, ":::::", ":::::", ":ooo:", ":::::", ":ooo:" );
	SetChar( 40, ":::::", ":::::", ":::::", ":::o:", "::o::" );
	SetChar( 41, ":::::", ":::::", ":::::", ":::::", "::o::" );
	SetChar( 42, ":::::", ":::::", ":ooo:", ":::::", ":::::" );
	SetChar( 43, ":::o:", "::o::", "::o::", "::o::", ":::o:" );
	SetChar( 44, "::o::", ":::o:", ":::o:", ":::o:", "::o::" );
	SetChar( 45, ":::::", ":::::", ":::::", ":::::", ":::::" );
	SetChar( 46, "ooooo", "ooooo", "ooooo", "ooooo", "ooooo" );
	SetChar( 47, "::o::", "::o::", ":::::", ":::::", ":::::" ); // Tnx Ferry
	SetChar( 48, "o:o:o", ":ooo:", "ooooo", ":ooo:", "o:o:o" );
	SetChar( 49, "::::o", ":::o:", "::o::", ":o:::", "o::::" );
	char c[] = "abcdefghijklmnopqrstuvwxyz0123456789!?:=,.-() #'*/";
	int i;
	for (i = 0; i < 256; i++) transl[i] = 45;
	for (i = 0; i < 50; i++) transl[(unsigned char)c[i]] = i;
}

void Surface::ScaleColor( unsigned int scale )
{
	int s = width * height;
	for (int i = 0; i < s; i++)
	{
		Pixel c = buffer[i];
		unsigned int rb = (((c & (REDMASK | BLUEMASK)) * scale) >> 5) & (REDMASK | BLUEMASK);
		unsigned int g = (((c & GREENMASK) * scale) >> 5) & GREENMASK;
		buffer[i] = rb + g;
	}
}

Sprite::Sprite( Surface* s, unsigned int frames ) :
	width( s->GetWidth() / frames ),
	height( s->GetHeight() ),
	numFrames( frames ),
	currentFrame( 0 ),
	flags( 0 ),
	start( new unsigned int* [frames] ),
	surface( s )
{
	InitializeStartData();
}

Sprite::~Sprite()
{
	delete surface;
	for (unsigned int i = 0; i < numFrames; i++) delete start[i];
	delete start;
}

void Sprite::Draw( Surface* target, int x, int y )
{
	if ((x < -width) || (x > ( target->GetWidth() + width ))) return;
	if ((y < -height) || (y > ( target->GetHeight() + height ))) return;
	int x1 = x, x2 = x + width;
	int y1 = y, y2 = y + height;
	Pixel* src = GetBuffer() + currentFrame * width;
	if (x1 < 0)
	{
		src += -x1;
		x1 = 0;
	}
	if (x2 > target->GetWidth()) x2 = target->GetWidth();
	if (y1 < 0)
	{
		src += -y1 * m_Pitch;
		y1 = 0;
	}
	if (y2 > target->GetHeight()) y2 = target->GetHeight();
	Pixel* dest = target->GetBuffer();
	int xs;
	const int dpitch = target->GetWidth();
	if ((x2 > x1) && (y2 > y1))
	{
		unsigned int addr = y1 * dpitch + x1;
		const int width = x2 - x1;
		const int height = y2 - y1;
		for (int y = 0; y < height; y++)
		{
			const int line = y + (y1 - y);
			const int lsx = start[currentFrame][line] + x;
			if (flags & FLARE)
			{
				xs = (lsx > x1) ? lsx - x1 : 0;
				for (int x = xs; x < width; x++)
				{
					const Pixel c1 = *(src + x);
					if (c1 & 0xffffff)
					{
						const Pixel c2 = *(dest + addr + x);
						*(dest + addr + x) = AddBlend( c1, c2 );
					}
				}
			}
			else
			{
				xs = (lsx > x1) ? lsx - x1 : 0;
				for (int x = xs; x < width; x++)
				{
					const Pixel c1 = *(src + x);
					if (c1 & 0xffffff) *(dest + addr + x) = c1;
				}
			}
			addr += dpitch;
			src += m_Pitch;
		}
	}
}

void Sprite::DrawScaled( int x, int y, int w, int h, Surface* target )
{
	if ((w == 0) || (h == 0)) return;
	for (int x = 0; x < w; x++) for (int y = 0; y < h; y++)
	{
		int u = (int)((float)x * ((float)width / (float)w));
		int v = (int)((float)y * ((float)height / (float)h));
		Pixel color = GetBuffer()[u + v * m_Pitch];
		if (color & 0xffffff) target->GetBuffer()[x + x + ((y + y) * target->GetWidth())] = color;
	}
}

void Sprite::InitializeStartData()
{
	for (unsigned int f = 0; f < numFrames; ++f)
	{
		start[f] = new unsigned int[height];
		for (int y = 0; y < height; ++y)
		{
			start[f][y] = width;
			Pixel* addr = GetBuffer() + f * width + y * m_Pitch;
			for (int x = 0; x < width; ++x)
			{
				if (addr[x])
				{
					start[f][y] = x;
					break;
				}
			}
		}
	}
}

Font::Font( char* file, char* chars )
{
	surface = new Surface( file );
	Pixel* b = surface->GetBuffer();
	int w = surface->GetWidth();
	int h = surface->GetHeight();
	unsigned int charnr = 0, start = 0;
	trans = new int[256];
	memset( trans, 0, 1024 );
	unsigned int i;
	for (i = 0; i < strlen( chars ); i++) trans[(unsigned char)chars[i]] = i;
	offset = new int[strlen( chars )];
	width = new int[strlen( chars )];
	height = h;
	cy1 = 0, cy2 = 1024;
	int x, y;
	bool lastempty = true;
	for (x = 0; x < w; x++)
	{
		bool empty = true;
		for (y = 0; y < h; y++) if (*(b + x + y * w) & 0xffffff)
		{
			if (lastempty) start = x;
			empty = false;
		}
		if ((empty) && (!lastempty))
		{
			width[charnr] = x - start;
			offset[charnr] = start;
			if (++charnr == strlen( chars )) break;
		}
		lastempty = empty;
	}
}

Font::~Font()
{
	delete surface;
	delete trans;
	delete width;
	delete offset;
}

int Font::Width( char* text )
{
	int w = 0;
	unsigned int i;
	for (i = 0; i < strlen( text ); i++)
	{
		unsigned char c = (unsigned char)text[i];
		if (c == 32) w += 4; else w += width[trans[c]] + 2;
	}
	return w;
}

void Font::Centre( Surface* target, char* text, int y )
{
	int x = (target->GetWidth() - Width( text )) / 2;
	Print( target, text, x, y );
}

void Font::Print( Surface* target, char* text, int x, int y, bool clip )
{
	Pixel* b = target->GetBuffer() + x + y * target->GetWidth();
	Pixel* s = surface->GetBuffer();
	unsigned int i, cx;
	int u, v;
	if (((y + height) < cy1) || (y > cy2)) return;
	for (cx = 0, i = 0; i < strlen( text ); i++)
	{
		if (text[i] == ' ') cx += 4; else
		{
			int c = trans[(unsigned char)text[i]];
			Pixel* t = s + offset[c], * d = b + cx;
			if (clip)
			{
				for (v = 0; v < height; v++)
				{
					if (((y + v) >= cy1) && ((y + v) <= cy2))
					{
						for (u = 0; u < width[c]; u++)
							if ((t[u]) && ((x + (int)cx + u) < target->GetWidth()))
								d[u] = AddBlend( t[u], d[u] );
					}
					t += surface->GetWidth(), d += target->GetWidth();
				}
			}
			else
			{
				for (v = 0; v < height; v++)
				{
					if (((y + v) >= cy1) && ((y + v) <= cy2))
						for (u = 0; u < width[c]; u++) if (t[u]) d[u] = AddBlend( t[u], d[u] );
					t += surface->GetWidth(), d += target->GetWidth();
				}
			}
			cx += width[c] + 2;
			if ((int)(cx + x) >= target->GetWidth()) break;
		}
	}
}