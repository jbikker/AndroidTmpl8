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
	Pixel* src = orig->buffer, * dst = buffer;
	int u, v, owidth = orig->width, oheight = orig->height;
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
	Pixel* d = dst->buffer;
	Pixel* s = buffer;
	if (s == 0 || d == 0) return;
	int srcwidth = width;
	int srcheight = height;
	int dstwidth = dst->width;
	int dstheight = dst->height;
	if ((srcwidth + x) > dstwidth) srcwidth = dstwidth - x;
	if ((srcheight + y) > dstheight) srcheight = dstheight - y;
	if (x < 0) s -= x, srcwidth += x, x = 0;
	if (y < 0) s -= y * srcwidth, srcheight += y, y = 0;
	if (srcwidth <= 0 || srcheight <= 0) return;
	d += x + dstwidth * y;
	for (int l = 0; l < srcheight; l++, d += dstwidth, s += srcwidth) memcpy( d, s, srcwidth * 4 );
}

void Surface::BlendCopyTo( Surface* dst, int x, int y )
{
	Pixel* d = dst->buffer;
	Pixel* s = buffer;
	if (s == 0 || d == 0) return;
	int srcwidth = width;
	int srcheight = height;
	int dstwidth = dst->width;
	int dstheight = dst->height;
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
			d += dstwidth, s += srcwidth;
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

int Surface::decodePNG( vector<uchar>& out_image, uint& imgw, uint& imgh, const uchar* in_png, size_t in_size, bool convert_to_rgba32 )
{
	static const uint CLCL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 }; //code length code lengths
	struct Zlib
	{
		static uint read1BFS( size_t& bitp, const uchar* bits )
		{
			uint result = (bits[bitp >> 3] >> (bitp & 0x7)) & 1;
			bitp++;
			return result;
		}
		static uint readBFS( size_t& bitp, const uchar* bits, size_t nbits )
		{
			uint result = 0;
			for (size_t i = 0; i < nbits; i++) result += (read1BFS( bitp, bits )) << i;
			return result;
		}
		struct HTree
		{
			int makeFromLengths( const vector<uint>& l, uint maxbl )
			{
				uint nc = (uint)(l.size()), p = 0, nf = 0;
				vector<uint> tree1d( nc ), c( maxbl + 1, 0 ), n( maxbl + 1, 0 );
				for (uint b = 0; b < nc; b++) c[l[b]]++;
				for (uint b = 1; b <= maxbl; b++) n[b] = (n[b - 1] + c[b - 1]) << 1;
				for (uint i = 0; i < nc; i++) if (l[i] != 0) tree1d[i] = n[l[i]]++;
				t.clear();
				t.resize( nc * 2, 32767 );
				for (uint n = 0; n < nc; n++) for (uint i = 0; i < l[n]; i++)
				{
					uint b = (tree1d[n] >> (l[n] - i - 1)) & 1;
					if (p > nc - 2) return 55; else if (t[2 * p + b] != 32767) p = t[2 * p + b] - nc;
					else if (i + 1 == l[n]) t[2 * p + b] = n, p = 0; else t[2 * p + b] = ++nf + nc, p = nf;
				}
				return 0;
			}
			int decode( bool& d, uint& r, size_t& p, uint bit ) const
			{
				uint nc = (uint)t.size() / 2;
				if (p >= nc) return 11; else r = t[2 * p + bit], d = (r < nc), p = d ? 0 : r - nc;
				return 0;
			}
			vector<uint> t;
		};
		struct Inflator
		{
			int error;
			void inflate( vector<uchar>& out, const vector<uchar>& in, size_t inpos = 0 )
			{
				size_t bp = 0, pos = 0;
				error = 0;
				uint BFINAL = 0;
				while (!BFINAL && !error)
				{
					if (bp >> 3 >= in.size()) { error = 52; return; }
					BFINAL = read1BFS( bp, &in[inpos] );
					uint BTYPE = read1BFS( bp, &in[inpos] ) + 2 * read1BFS( bp, &in[inpos] );
					if (BTYPE == 3) { error = 20; return; }
					else if (BTYPE == 0) inflateNoCompression( out, &in[inpos], bp, pos, in.size() );
					else inflateHuffmanBlock( out, &in[inpos], bp, pos, in.size(), BTYPE );
				}
				if (!error) out.resize( pos );
			}
			void generateFixedTrees( HTree& tree, HTree& treeD )
			{
				vector<uint> bitlen( 288, 8 ), bitlenD( 32, 5 );;
				for (size_t i = 144; i <= 255; i++) bitlen[i] = 9;
				for (size_t i = 256; i <= 279; i++) bitlen[i] = 7;
				tree.makeFromLengths( bitlen, 15 );
				treeD.makeFromLengths( bitlenD, 15 );
			}
			HTree codetree, codetreeD, codelengthcodetree;
			uint huffmanDecodeSymbol( const uchar* in, size_t& bp, const HTree& c, size_t l )
			{
				bool d;
				uint ct;
				for (size_t p = 0;; )
				{
					if ((bp & 0x07) == 0 && (bp >> 3) > l) { error = 10; return 0; }
					error = c.decode( d, ct, p, read1BFS( bp, in ) );
					if (error) return 0; else if (d) return ct;
				}
			}
			void getTID( HTree& tree, HTree& treeD, const uchar* in, size_t& bp, size_t l )
			{
				vector<uint> bitlen( 288, 0 ), bitlenD( 32, 0 );
				if (bp >> 3 >= l - 2) { error = 49; return; }
				size_t HLIT = readBFS( bp, in, 5 ) + 257, HDIST = readBFS( bp, in, 5 ) + 1;
				size_t HCLEN = readBFS( bp, in, 4 ) + 4;
				vector<uint> clc( 19 );
				for (size_t i = 0; i < 19; i++) clc[CLCL[i]] = (i < HCLEN) ? readBFS( bp, in, 3 ) : 0;
				if (error = codelengthcodetree.makeFromLengths( clc, 7 )) return;
				size_t i = 0, replength;
				while (i < HLIT + HDIST)
				{
					uint v, c = huffmanDecodeSymbol( in, bp, codelengthcodetree, l );
					if (error) return;
					if (c <= 15) { if (i < HLIT) bitlen[i++] = c; else bitlenD[i++ - HLIT] = c; }
					else if (c == 16)
					{
						if (bp >> 3 >= l) { error = 50; return; }
						replength = 3 + readBFS( bp, in, 2 );
						if ((i - 1) < HLIT) v = bitlen[i - 1]; else v = bitlenD[i - HLIT - 1];
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST) { error = 13; return; }
							if (i < HLIT) bitlen[i++] = v; else bitlenD[i++ - HLIT] = v;
						}
					}
					else if (c == 17)
					{
						if (bp >> 3 >= l) { error = 50; return; }
						replength = 3 + readBFS( bp, in, 3 );
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST) { error = 14; return; }
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else if (c == 18)
					{
						if (bp >> 3 >= l) { error = 50; return; }
						replength = 11 + readBFS( bp, in, 7 );
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST) { error = 15; return; }
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else { error = 16; return; }
				}
				if (bitlen[256] == 0) { error = 64; return; }
				if (error = tree.makeFromLengths( bitlen, 15 )) return;
				if (error = treeD.makeFromLengths( bitlenD, 15 )) return;
			}
			void inflateHuffmanBlock( vector<uchar>& out, const uchar* in, size_t& bp, size_t& pos, size_t l, uint t )
			{
				static const uint DISTBASE[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
					257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
				static const uint DISTEXTRA[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
					9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
				static const uint LENBASE[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35,
					43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
				static const uint LENEXTRA[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
				if (t == 1) { generateFixedTrees( codetree, codetreeD ); }
				else if (t == 2) { getTID( codetree, codetreeD, in, bp, l ); if (error) return; }
				for (;; )
				{
					uint c = huffmanDecodeSymbol( in, bp, codetree, l );
					if (error != 0 || c == 256) return; else if (c <= 255)
					{
						if (pos >= out.size()) out.resize( (pos + 1) * 2 );
						out[pos++] = (uchar)(c);
					}
					else if (c >= 257 && c <= 285) //length code
					{
						size_t length = LENBASE[c - 257], numextrabits = LENEXTRA[c - 257];
						if ((bp >> 3) >= l) { error = 51; return; }
						length += readBFS( bp, in, numextrabits );
						uint codeD = huffmanDecodeSymbol( in, bp, codetreeD, l );
						if (error) return; else if (codeD > 29) { error = 18; return; }
						uint dist = DISTBASE[codeD], numextrabitsD = DISTEXTRA[codeD];
						if ((bp >> 3) >= l) { error = 51; return; }
						dist += readBFS( bp, in, numextrabitsD );
						size_t start = pos, back = start - dist;
						if (pos + length >= out.size()) out.resize( (pos + length) * 2 );
						for (size_t i = 0; i < length; i++)
						{
							out[pos++] = out[back++];
							if (back >= start) back = start - dist;
						}
					}
				}
			}
			void inflateNoCompression( vector<uchar>& out, const uchar* in, size_t& bp, size_t& pos, size_t l )
			{
				while ((bp & 0x7) != 0) bp++;
				size_t p = bp / 8;
				if (p >= l - 4) { error = 52; return; }
				uint LEN = in[p] + 256 * in[p + 1], NLEN = in[p + 2] + 256 * in[p + 3];
				p += 4;
				if (LEN + NLEN != 65535) { error = 21; return; }
				else if (pos + LEN >= out.size()) out.resize( pos + LEN );
				if (p + LEN > l) { error = 23; return; }
				else for (uint n = 0; n < LEN; n++) out[pos++] = in[p++];
				bp = p * 8;
			}
		};
		int decompress( vector<uchar>& out, const vector<uchar>& in )
		{
			Inflator inflator;
			if (in.size() < 2) { return 53; }
			else if ((in[0] * 256 + in[1]) % 31 != 0) { return 24; }
			uint CM = in[0] & 15, C = (in[0] >> 4) & 15, F = (in[1] >> 5) & 1;
			if (CM != 8 || C > 7) { return 25; }
			else if (F != 0) { return 26; }
			inflator.inflate( out, in, 2 );
			return inflator.error;
		}
	};
	struct PNG
	{
		struct Info { uint w, h, ct, bd, cm, fm, im, kr, kg, kb; bool d; vector<uchar> p; } info;
		int error;
		void decode( vector<uchar>& out, const uchar* in, size_t size, bool convert_to_rgba32 )
		{
			error = 0;
			if (size == 0 || in == 0) { error = 48; return; }
			readPngHeader( &in[0], size );
			if (error) return;
			size_t pos = 33;
			vector<uchar> idat;
			bool IEND = false, known_type = true;
			info.d = false;
			while (!IEND)
			{
				if (pos + 8 >= size) { error = 30; return; }
				size_t chunkLength = read32bitInt( &in[pos] );
				pos += 4;
				if (chunkLength > 2147483647) { error = 63; return; }
				if (pos + chunkLength >= size) { error = 35; return; }
				if (in[pos + 0] == 'I' && in[pos + 1] == 'D' && in[pos + 2] == 'A' && in[pos + 3] == 'T')
				{
					idat.insert( idat.end(), &in[pos + 4], &in[pos + 4 + chunkLength] );
					pos += (4 + chunkLength);
				}
				else if (in[pos + 0] == 'I' && in[pos + 1] == 'E' && in[pos + 2] == 'N' && in[pos + 3] == 'D')
					pos += 4, IEND = true;
				else if (in[pos + 0] == 'P' && in[pos + 1] == 'L' && in[pos + 2] == 'T' && in[pos + 3] == 'E')
				{
					pos += 4;
					info.p.resize( 4 * (chunkLength / 3) );
					if (info.p.size() > (4 * 256)) { error = 38; return; }
					for (size_t i = 0; i < info.p.size(); i += 4)
					{
						for (size_t j = 0; j < 3; j++) info.p[i + j] = in[pos++];
						info.p[i + 3] = 255;
					}
				}
				else if (in[pos + 0] == 't' && in[pos + 1] == 'R' && in[pos + 2] == 'N' && in[pos + 3] == 'S')
				{
					pos += 4;
					if (info.ct == 3)
					{
						if (4 * chunkLength > info.p.size()) { error = 39; return; }
						for (size_t i = 0; i < chunkLength; i++) info.p[4 * i + 3] = in[pos++];
					}
					else if (info.ct == 0)
					{
						if (chunkLength != 2) { error = 40; return; }
						info.d = 1, info.kr = info.kg = info.kb = 256 * in[pos] + in[pos + 1];
						pos += 2;
					}
					else if (info.ct == 2)
					{
						if (chunkLength != 6) { error = 41; return; }
						info.d = 1;
						info.kr = 256 * in[pos] + in[pos + 1], pos += 2;
						info.kg = 256 * in[pos] + in[pos + 1], pos += 2;
						info.kb = 256 * in[pos] + in[pos + 1], pos += 2;
					}
					else { error = 42; return; }
				}
				else
				{
					if (!(in[pos + 0] & 32)) { error = 69; return; }
					pos += (chunkLength + 4), known_type = false;
				}
				pos += 4;
			}
			uint bpp = getBpp( info );
			vector<uchar> sc( ((info.w * (info.h * bpp + 7)) / 8) + info.h );
			Zlib zlib;
			error = zlib.decompress( sc, idat );
			if (error) return;
			size_t bw = (bpp + 7) / 8, outlength = (info.h * info.w * bpp + 7) / 8;
			out.resize( outlength );
			uchar* o = outlength ? &out[0] : 0;
			if (info.im == 0)
			{
				size_t s = 0, l = (info.w * bpp + 7) / 8;
				if (bpp >= 8) for (uint y = 0; y < info.h; y++)
				{
					uint t = sc[s];
					const uchar* pl = (y == 0) ? 0 : &o[(y - 1) * info.w * bw];
					unFilterScanline( &o[s - y], &sc[s + 1], pl, bw, t, l );
					if (error) return; else s += (1 + l);
				}
				else
				{
					vector<uchar> templine( (info.w * bpp + 7) >> 3 );
					for (size_t y = 0, obp = 0; y < info.h; y++)
					{
						uint f = sc[s];
						const uchar* pl = (y == 0) ? 0 : &o[(y - 1) * info.w * bw];
						unFilterScanline( &templine[0], &sc[s + 1], pl, bw, f, l );
						if (error) return;
						for (size_t bp = 0; bp < info.w * bpp; ) setBORS( obp, o, readBFRS( bp, &templine[0] ) );
						s += (1 + l);
					}
				}
			}
			else
			{
				size_t w[7] = { (info.w + 7) / 8, (info.w + 3) / 8,
					(info.w + 3) / 4, (info.w + 1) / 4, (info.w + 1) / 2, (info.w + 0) / 2, (info.w + 0) / 1 };
				size_t h[7] = { (info.h + 7) / 8, (info.h + 7) / 8,
					(info.h + 3) / 8, (info.h + 3) / 4, (info.h + 1) / 4, (info.h + 1) / 2, (info.h + 0) / 2 };
				size_t s[7] = { 0 };
				size_t p[28] = { 0, 4, 0, 2, 0, 1, 0, 0, 0, 4, 0, 2, 0, 1, 8, 8, 4, 4, 2, 2, 1, 8, 8, 8, 4, 4, 2, 2 };
				for (int i = 0; i < 6; i++) s[i + 1] = s[i] + h[i] * ((w[i] ? 1 : 0) + (w[i] * bpp + 7) / 8);
				vector<uchar> so( (info.w * bpp + 7) / 8 ), sl( (info.w * bpp + 7) / 8 );
				for (int i = 0; i < 7; i++)
					A7( &o[0], &sl[0], &so[0], &sc[s[i]], info.w, p[i], p[i + 7], p[i + 14], p[i + 21], w[i], h[i], bpp );
			}
			if (convert_to_rgba32 && (info.ct != 6 || info.bd != 8))
			{
				vector<uchar> data = out;
				error = convert( out, &data[0], info, info.w, info.h );
			}
		}
		void readPngHeader( const uchar* in, size_t inlength )
		{
			if (inlength < 29) { error = 27; return; }
			if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10)
			{
				error = 28; return;
			} // no PNG signature
			if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R') { error = 29; return; }
			info.w = read32bitInt( &in[16] ), info.h = read32bitInt( &in[20] );
			info.bd = in[24], info.ct = in[25], info.cm = in[26], info.fm = in[27], info.im = in[28];
			if (in[26] != 0) { error = 32; return; }
			else if (in[27] != 0) { error = 33; return; }
			if (in[28] > 1) { error = 34; return; }
			error = checkColorValidity( info.ct, info.bd );
		}
		void unFilterScanline( uchar* r, const uchar* s, const uchar* p, size_t bw, uint ft, size_t l )
		{
			switch (ft)
			{
			case 0:
				for (size_t i = 0; i < l; i++) r[i] = s[i]; break;
			case 1:
				for (size_t i = 0; i < bw; i++) r[i] = s[i];
				for (size_t i = bw; i < l; i++) r[i] = s[i] + r[i - bw]; break;
			case 2:
				if (p) for (size_t i = 0; i < l; i++) r[i] = s[i] + p[i];
				else for (size_t i = 0; i < l; i++) r[i] = s[i]; break;
			case 3:
				if (p)
				{
					for (size_t i = 0; i < bw; i++) r[i] = s[i] + p[i] / 2;
					for (size_t i = bw; i < l; i++) r[i] = s[i] + ((r[i - bw] + p[i]) / 2);
				}
				else
				{
					for (size_t i = 0; i < bw; i++) r[i] = s[i];
					for (size_t i = bw; i < l; i++) r[i] = s[i] + r[i - bw] / 2;
				}
				break;
			case 4:
				if (p)
				{
					for (size_t i = 0; i < bw; i++) r[i] = s[i] + PP( 0, p[i], 0 );
					for (size_t i = bw; i < l; i++) r[i] = s[i] + PP( r[i - bw], p[i], p[i - bw] );
				}
				else
				{
					for (size_t i = 0; i < bw; i++) r[i] = s[i];
					for (size_t i = bw; i < l; i++) r[i] = s[i] + PP( r[i - bw], 0, 0 );
				}
				break;
			default:
				error = 36; return;
			}
		}
		void A7( uchar* out, uchar* l, uchar* o, const uchar* in, uint w, size_t p,
			size_t q, size_t s, size_t t, size_t passw, size_t passh, uint bpp )
		{
			if (passw == 0) return;
			size_t bw = (bpp + 7) / 8, ll = 1 + ((bpp * passw + 7) / 8);
			for (uint y = 0; y < passh; y++)
			{
				uchar ft = in[y * ll], * pl = (y == 0) ? 0 : o;
				unFilterScanline( l, &in[y * ll + 1], pl, bw, ft, (w * bpp + 7) / 8 );
				if (error) return;
				if (bpp >= 8) for (size_t i = 0; i < passw; i++) for (size_t b = 0; b < bw; b++)
					out[bw * w * (q + t * y) + bw * (p + s * i) + b] = l[bw * i + b];
				else for (size_t i = 0; i < passw; i++)
				{
					size_t obp = bpp * w * (q + t * y) + bpp * (p + s * i), bp = i * bpp;
					for (size_t b = 0; b < bpp; b++) setBORS( obp, out, readBFRS( bp, &l[0] ) );
				}
				uchar* t = l;
				l = o, o = t;
			}
		}
		static uint readBFRS( size_t& bitp, const uchar* bits )
		{
			uint result = (bits[bitp >> 3] >> (7 - (bitp & 0x7))) & 1;
			bitp++;
			return result;
		}
		static uint readBFRSn( size_t& bitp, const uchar* bits, uint nbits )
		{
			uint result = 0;
			for (size_t i = nbits - 1; i < nbits; i--) result += ((readBFRS( bitp, bits )) << i);
			return result;
		}
		void setBORS( size_t& bitp, uchar* bits, uint bit )
		{
			bits[bitp >> 3] |= (bit << (7 - (bitp & 0x7)));
			bitp++;
		}
		uint read32bitInt( const uchar* buffer )
		{
			return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
		}
		int checkColorValidity( uint colorType, uint bd )
		{
			if ((colorType == 2 || colorType == 4 || colorType == 6))
			{ if (!(bd == 8 || bd == 16))return 37; else return 0; }
			else if (colorType == 0)
			{ if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; else return 0; }
			else if (colorType == 3)
			{ if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8)) return 37; else return 0; }
			else return 31;
		}
		uint getBpp( const Info& info )
		{
			if (info.ct == 2) return (3 * info.bd);
			else if (info.ct >= 4) return (info.ct - 2) * info.bd; else return info.bd;
		}
		int convert( vector<uchar>& out, const uchar* in, Info& i, uint w, uint h )
		{
			size_t n = w * h, bp = 0;
			out.resize( n * 4 );
			uchar* o = out.empty() ? 0 : &out[0];
			if (i.bd == 8 && i.ct == 0) for (size_t j = 0; j < n; j++)
				o[4 * j + 0] = o[4 * j + 1] = o[4 * j + 2] = in[j],
				o[4 * j + 3] = (i.d && in[j] == i.kr) ? 0 : 255;
			else if (i.bd == 8 && i.ct == 2) for (size_t j = 0; j < n; j++)
			{
				for (size_t c = 0; c < 3; c++) o[4 * j + c] = in[3 * j + c];
				o[4 * j + 3] = (i.d == 1 && in[3 * j + 0] == i.kr &&
					in[3 * j + 1] == i.kg && in[3 * j + 2] == i.kb) ? 0 : 255;
			}
			else if (i.bd == 8 && i.ct == 3) for (size_t j = 0; j < n; j++)
			{
				if (4U * in[j] >= i.p.size()) return 46;
				for (size_t c = 0; c < 4; c++) o[4 * j + c] = i.p[4 * in[j] + c];
			}
			else if (i.bd == 8 && i.ct == 4) for (size_t i = 0; i < n; i++)
				o[4 * i + 0] = o[4 * i + 1] = o[4 * i + 2] = in[2 * i + 0],
				o[4 * i + 3] = in[2 * i + 1];
			else if (i.bd == 8 && i.ct == 6) for (size_t i = 0; i < n; i++)
				for (size_t c = 0; c < 4; c++) o[4 * i + c] = in[4 * i + c]; // RGB with alpha
			else if (i.bd == 16 && i.ct == 0) for (size_t j = 0; j < n; j++)
				o[4 * j + 0] = o[4 * j + 1] = o[4 * j + 2] = in[2 * j],
				o[4 * j + 3] = (i.d && 256U * in[j] + in[j + 1] == i.kr) ? 0 : 255;
			else if (i.bd == 16 && i.ct == 2) for (size_t j = 0; j < n; j++)
			{
				for (size_t c = 0; c < 3; c++) o[4 * j + c] = in[6 * j + 2 * c];
				o[4 * j + 3] = (i.d && 256U * in[6 * j + 0] + in[6 * j + 1] == i.kr &&
					256U * in[6 * j + 2] + in[6 * j + 3] == i.kg &&
					256U * in[6 * j + 4] + in[6 * j + 5] == i.kb) ? 0 : 255;
			}
			else if (i.bd == 16 && i.ct == 4) for (size_t i = 0; i < n; i++)
				o[4 * i + 0] = o[4 * i + 1] = o[4 * i + 2] = in[4 * i],
				o[4 * i + 3] = in[4 * i + 2];
			else if (i.bd == 16 && i.ct == 6) for (size_t i = 0; i < n; i++)
				for (size_t c = 0; c < 4; c++) o[4 * i + c] = in[8 * i + 2 * c]; // RGB with alpha
			else if (i.bd < 8 && i.ct == 0) for (size_t j = 0; j < n; j++)
			{
				uint v = (readBFRSn( bp, in, i.bd ) * 255) / ((1 << i.bd) - 1);
				o[4 * j + 0] = o[4 * j + 1] = o[4 * j + 2] = (uchar)(v);
				o[4 * j + 3] = (i.d && v && ((1U << i.bd) - 1U) == i.kr &&
					((1U << i.bd) - 1U)) ? 0 : 255;
			}
			else if (i.bd < 8 && i.ct == 3) for (size_t j = 0; j < n; j++)
			{
				uint v = readBFRSn( bp, in, i.bd );
				if (4 * v >= i.p.size()) return 47;
				for (size_t c = 0; c < 4; c++) o[4 * j + c] = i.p[4 * v + c];
			}
			return 0;
		}
		uchar PP( short a, short b, short c )
		{
			short p = a + b - c, pa = p > a ? (p - a) : (a - p), pb =
				p > b ? (p - b) : (b - p), pc = p > c ? (p - c) : (c - p);
			return (uchar)((pa <= pb && pa <= pc) ? a : pb <= pc ? b : c);
		}
	};
	PNG d;
	d.decode( out_image, in_png, in_size, convert_to_rgba32 );
	imgw = d.info.w, imgh = d.info.h;
	return d.error;
}

bool Surface::LoadPNGFile( const char* fileName, uint& w, uint& h, vector<uchar>& image )
{
	vector<uchar> buffer;
	loadBinaryFile( buffer, fileName );
	return decodePNG( image, w, h, &buffer[0], (uint)buffer.size() ) == 0;
}

Sprite::Sprite( Surface* s, unsigned int frames ) :
	width( s->width / frames ),
	height( s->height ),
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
	if ((x < -width) || (x > ( target->width + width ))) return;
	if ((y < -height) || (y > ( target->height + height ))) return;
	int x1 = x, x2 = x + width;
	int y1 = y, y2 = y + height;
	Pixel* src = GetBuffer() + currentFrame * width;
	if (x1 < 0)
	{
		src += -x1;
		x1 = 0;
	}
	if (x2 > target->width) x2 = target->width;
	if (y1 < 0)
	{
		src += -y1 * m_Pitch;
		y1 = 0;
	}
	if (y2 > target->height) y2 = target->height;
	Pixel* dest = target->buffer;
	int xs;
	const int dpitch = target->width;
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
		if (color & 0xffffff) target->buffer[x + x + ((y + y) * target->width)] = color;
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
	Pixel* b = surface->buffer;
	int w = surface->width;
	int h = surface->height;
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
	int x = (target->width - Width( text )) / 2;
	Print( target, text, x, y );
}

void Font::Print( Surface* target, char* text, int x, int y, bool clip )
{
	Pixel* b = target->buffer + x + y * target->width;
	Pixel* s = surface->buffer;
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
							if ((t[u]) && ((x + (int)cx + u) < target->width))
								d[u] = AddBlend( t[u], d[u] );
					}
					t += surface->width, d += target->width;
				}
			}
			else
			{
				for (v = 0; v < height; v++)
				{
					if (((y + v) >= cy1) && ((y + v) <= cy2))
						for (u = 0; u < width[c]; u++) if (t[u]) d[u] = AddBlend( t[u], d[u] );
					t += surface->width, d += target->width;
				}
			}
			cx += width[c] + 2;
			if ((int)(cx + x) >= target->width) break;
		}
	}
}