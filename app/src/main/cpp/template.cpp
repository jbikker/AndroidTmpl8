#include "template.h"

// typedefs
typedef unsigned char uchar;
typedef unsigned int uint;

// namespace
using namespace std;

// game object
static Game game;

// forward declarations
bool LoadPNGFile( const char* fileName, uint& w, uint& h, vector<uchar>& image );

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

GLuint LoadTexture( const char* fileName, uint** data )
{
	vector<uchar> image;
	uint w, h;
	GLuint retVal = 0;
	if (LoadPNGFile( fileName, w, h, image ))
	{
		uint* d = (uint*)image.data();
		for (uint i = 0; i < w * h; i++)
			d[i] = ((d[i] & 255) << 16) + (255 << 24) + ((d[i] >> 16) & 255) + (d[i] & 0xff00);
		retVal = CreateTexture( d, w, h );
		if (data)
		{
			uint* buffer = new uint[w * h];
			memcpy( buffer, image.data(), w * h * sizeof( uint ) );
			*data = buffer;
		}
	}
	return retVal;
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
	const char vsText[] =
		"#version 330\nlayout(location=0)in vec3 pos;\nlayout(location=1)in vec2 uv;\n"
		"out vec2 t;\nvoid main(){t=uv;gl_Position=vec4(pos,1);}";
	char fsText[4096] = "#version 330\nuniform sampler2D C;\nin vec2 t;\nvec2 r=vec2(320,192);\n";
#else
	char vsText[] =
		"attribute vec4 pos;\nattribute vec2 uv;\nvarying vec2 t; \n"
		"void main(){gl_Position=pos;t=uv;}";
	char fsText[4096] =
		"precision mediump float;\nuniform sampler2D C;\nvarying vec2 t;\n"
		"const vec2 r=vec2(320,192);\n";
#endif
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
	strcat( fsText, fsBody );
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

int decodePNG( vector<uchar>& out_image, uint& image_width, uint& image_height, const uchar* in_png,
			   size_t in_size, bool convert_to_rgba32 = true )
{
	static const uint LENBASE[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35,
		43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	static const uint LENEXTRA[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
		4, 4, 4, 4, 5, 5, 5, 5, 0 };
	static const uint DISTBASE[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193,
		12289, 16385, 24577 };
	static const uint DISTEXTRA[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
		9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
	static const uint CLCL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1,
		15 }; //code length code lengths
	struct Zlib
	{
		static uint readBitFromStream( size_t& bitp, const uchar* bits )
		{
			uint result = (bits[bitp >> 3] >> (bitp & 0x7)) & 1;
			bitp++;
			return result;
		}
		static uint readBitsFromStream( size_t& bitp, const uchar* bits, size_t nbits )
		{
			uint result = 0;
			for (size_t i = 0; i < nbits; i++) result += (readBitFromStream( bitp, bits )) << i;
			return result;
		}

		struct HuffmanTree
		{
			int makeFromLengths( const vector<uint>& bitlen, uint maxbitlen )
			{
				uint numcodes = (uint)(bitlen.size()), treepos = 0, nodefilled = 0;
				std::vector<uint> tree1d( numcodes ), blcount( maxbitlen + 1, 0 ), nextcode(
						maxbitlen + 1, 0 );
				for (uint bits = 0; bits < numcodes; bits++) blcount[bitlen[bits]]++;
				for (uint bits = 1; bits <= maxbitlen; bits++)
					nextcode[bits] = (nextcode[bits - 1] + blcount[bits - 1]) << 1;
				for (uint n = 0; n < numcodes; n++)
					if (bitlen[n] != 0)
						tree1d[n] = nextcode[bitlen[n]]++;
				tree2d.clear();
				tree2d.resize( numcodes * 2, 32767 );
				for (uint n = 0; n < numcodes; n++)
					for (uint i = 0; i < bitlen[n]; i++)
					{
						uint bit = (tree1d[n] >> (bitlen[n] - i - 1)) & 1;
						if (treepos > numcodes - 2) return 55;
						if (tree2d[2 * treepos + bit] == 32767)
						{
							if (i + 1 == bitlen[n])
							{
								tree2d[2 * treepos + bit] = n;
								treepos = 0;
							}
							else
							{
								tree2d[2 * treepos + bit] = ++nodefilled + numcodes;
								treepos = nodefilled;
							}
						}
						else treepos = tree2d[2 * treepos + bit] - numcodes;
					}
				return 0;
			}
			int decode( bool& decoded, uint& result, size_t& treepos, uint bit ) const
			{
				uint numcodes = (uint)tree2d.size() / 2;
				if (treepos >= numcodes) return 11;
				result = tree2d[2 * treepos + bit];
				decoded = (result < numcodes);
				treepos = decoded ? 0 : result - numcodes;
				return 0;
			}
			std::vector<uint> tree2d;
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
					if (bp >> 3 >= in.size())
					{
						error = 52;
						return;
					}
					BFINAL = readBitFromStream( bp, &in[inpos] );
					uint BTYPE = readBitFromStream( bp, &in[inpos] );
					BTYPE += 2 * readBitFromStream( bp, &in[inpos] );
					if (BTYPE == 3)
					{
						error = 20;
						return;
					}
					else if (BTYPE == 0)
						inflateNoCompression( out, &in[inpos], bp, pos, in.size() );
					else inflateHuffmanBlock( out, &in[inpos], bp, pos, in.size(), BTYPE );
				}
				if (!error) out.resize( pos );
			}
			void generateFixedTrees( HuffmanTree& tree, HuffmanTree& treeD )
			{
				std::vector<uint> bitlen( 288, 8 ), bitlenD( 32, 5 );;
				for (size_t i = 144; i <= 255; i++) bitlen[i] = 9;
				for (size_t i = 256; i <= 279; i++) bitlen[i] = 7;
				tree.makeFromLengths( bitlen, 15 );
				treeD.makeFromLengths( bitlenD, 15 );
			}
			HuffmanTree codetree, codetreeD, codelengthcodetree;
			uint huffmanDecodeSymbol( const uchar* in, size_t& bp, const HuffmanTree& codetree,
									  size_t inlength )
			{
				bool decoded;
				uint ct;
				for (size_t treepos = 0;; )
				{
					if ((bp & 0x07) == 0 && (bp >> 3) > inlength)
					{
						error = 10;
						return 0;
					}
					error = codetree.decode( decoded, ct, treepos, readBitFromStream( bp, in ) );
					if (error) return 0;
					if (decoded) return ct;
				}
			}
			void getTreeInflateDynamic( HuffmanTree& tree, HuffmanTree& treeD, const uchar* in,
										size_t& bp, size_t inlength )
			{
				std::vector<uint> bitlen( 288, 0 ), bitlenD( 32, 0 );
				if (bp >> 3 >= inlength - 2)
				{
					error = 49;
					return;
				}
				size_t HLIT = readBitsFromStream( bp, in, 5 ) + 257;
				size_t HDIST = readBitsFromStream( bp, in, 5 ) + 1;
				size_t HCLEN = readBitsFromStream( bp, in, 4 ) + 4;
				std::vector<uint> codelengthcode( 19 );
				for (size_t i = 0; i < 19; i++)
					codelengthcode[CLCL[i]] = (i < HCLEN) ? readBitsFromStream( bp, in, 3 ) : 0;
				error = codelengthcodetree.makeFromLengths( codelengthcode, 7 );
				if (error) return;
				size_t i = 0, replength;
				while (i < HLIT + HDIST)
				{
					uint code = huffmanDecodeSymbol( in, bp, codelengthcodetree, inlength );
					if (error) return;
					if (code <= 15)
					{
						if (i < HLIT) bitlen[i++] = code;
						else
							bitlenD[i++ - HLIT] = code;
					}
					else if (code == 16)
					{
						if (bp >> 3 >= inlength)
						{
							error = 50;
							return;
						}
						replength = 3 + readBitsFromStream( bp, in, 2 );
						uint value;
						if ((i - 1) < HLIT) value = bitlen[i - 1];
						else value = bitlenD[i - HLIT - 1];
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST)
							{
								error = 13;
								return;
							}
							if (i < HLIT) bitlen[i++] = value; else bitlenD[i++ - HLIT] = value;
						}
					}
					else if (code == 17)
					{
						if (bp >> 3 >= inlength)
						{
							error = 50;
							return;
						}
						replength = 3 + readBitsFromStream( bp, in, 3 );
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST)
							{
								error = 14;
								return;
							}
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else if (code == 18)
					{
						if (bp >> 3 >= inlength)
						{
							error = 50;
							return;
						}
						replength = 11 + readBitsFromStream( bp, in, 7 );
						for (size_t n = 0; n < replength; n++)
						{
							if (i >= HLIT + HDIST)
							{
								error = 15;
								return;
							}
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else
					{
						error = 16;
						return;
					}
				}
				if (bitlen[256] == 0)
				{
					error = 64;
					return;
				}
				error = tree.makeFromLengths( bitlen, 15 );
				if (error) return;
				error = treeD.makeFromLengths( bitlenD, 15 );
				if (error) return;
			}
			void inflateHuffmanBlock( vector<uchar>& out, const uchar* in, size_t& bp, size_t& pos,
									  size_t inlength, uint btype )
			{
				if (btype == 1) { generateFixedTrees( codetree, codetreeD ); }
				else if (btype == 2)
				{
					getTreeInflateDynamic( codetree, codetreeD, in, bp, inlength );
					if (error) return;
				}
				for (;; )
				{
					uint code = huffmanDecodeSymbol( in, bp, codetree, inlength );
					if (error) return;
					if (code == 256) return;
					else if (code <= 255)
					{
						if (pos >= out.size()) out.resize( (pos + 1) * 2 );
						out[pos++] = (uchar)(code);
					}
					else if (code >= 257 && code <= 285) //length code
					{
						size_t length = LENBASE[code - 257], numextrabits = LENEXTRA[code - 257];
						if ((bp >> 3) >= inlength)
						{
							error = 51;
							return;
						}
						length += readBitsFromStream( bp, in, numextrabits );
						uint codeD = huffmanDecodeSymbol( in, bp, codetreeD, inlength );
						if (error) return;
						if (codeD > 29)
						{
							error = 18;
							return;
						}
						uint dist = DISTBASE[codeD], numextrabitsD = DISTEXTRA[codeD];
						if ((bp >> 3) >= inlength)
						{
							error = 51;
							return;
						}
						dist += readBitsFromStream( bp, in, numextrabitsD );
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
			void inflateNoCompression( vector<uchar>& out, const uchar* in, size_t& bp, size_t& pos,
									   size_t inlength )
			{
				while ((bp & 0x7) != 0) bp++;
				size_t p = bp / 8;
				if (p >= inlength - 4)
				{
					error = 52;
					return;
				}
				uint LEN = in[p] + 256 * in[p + 1], NLEN = in[p + 2] + 256 * in[p + 3];
				p += 4;
				if (LEN + NLEN != 65535)
				{
					error = 21;
					return;
				}
				if (pos + LEN >= out.size()) out.resize( pos + LEN );
				if (p + LEN > inlength)
				{
					error = 23;
					return;
				}
				for (uint n = 0; n < LEN; n++) out[pos++] = in[p++];
				bp = p * 8;
			}
		};

		int decompress( vector<uchar>& out, const vector<uchar>& in )
		{
			Inflator inflator;
			if (in.size() < 2) { return 53; }
			if ((in[0] * 256 + in[1]) % 31 != 0) { return 24; }
			uint CM = in[0] & 15, CINFO = (in[0] >> 4) & 15, FDICT = (in[1] >> 5) & 1;
			if (CM != 8 || CINFO > 7) { return 25; }
			if (FDICT != 0) { return 26; }
			inflator.inflate( out, in, 2 );
			return inflator.error;
		}
	};
	struct PNG
	{
		struct Info
		{
			uint width, height, colorType, bitDepth, compressionMethod, filterMethod, interlaceMethod, key_r, key_g, key_b;
			bool key_defined;
			std::vector<uchar> palette;
		} info;
		int error;
		void decode( vector<uchar>& out, const uchar* in, size_t size, bool convert_to_rgba32 )
		{
			error = 0;
			if (size == 0 || in == 0)
			{
				error = 48;
				return;
			}
			readPngHeader( &in[0], size );
			if (error) return;
			size_t pos = 33;
			std::vector<uchar> idat;
			bool IEND = false, known_type = true;
			info.key_defined = false;
			while (!IEND)
			{
				if (pos + 8 >= size)
				{
					error = 30;
					return;
				}
				size_t chunkLength = read32bitInt( &in[pos] );
				pos += 4;
				if (chunkLength > 2147483647)
				{
					error = 63;
					return;
				}
				if (pos + chunkLength >= size)
				{
					error = 35;
					return;
				}
				if (in[pos + 0] == 'I' && in[pos + 1] == 'D' && in[pos + 2] == 'A' &&
					in[pos + 3] == 'T')
				{
					idat.insert( idat.end(), &in[pos + 4], &in[pos + 4 + chunkLength] );
					pos += (4 + chunkLength);
				}
				else if (in[pos + 0] == 'I' && in[pos + 1] == 'E' && in[pos + 2] == 'N' &&
						in[pos + 3] == 'D')
				{
					pos += 4;
					IEND = true;
				}
				else if (in[pos + 0] == 'P' && in[pos + 1] == 'L' && in[pos + 2] == 'T' &&
						in[pos + 3] == 'E')
				{
					pos += 4;
					info.palette.resize( 4 * (chunkLength / 3) );
					if (info.palette.size() > (4 * 256))
					{
						error = 38;
						return;
					}
					for (size_t i = 0; i < info.palette.size(); i += 4)
					{
						for (size_t j = 0; j < 3; j++) info.palette[i + j] = in[pos++];
						info.palette[i + 3] = 255;
					}
				}
				else if (in[pos + 0] == 't' && in[pos + 1] == 'R' && in[pos + 2] == 'N' &&
						in[pos + 3] == 'S')
				{
					pos += 4;
					if (info.colorType == 3)
					{
						if (4 * chunkLength > info.palette.size())
						{
							error = 39;
							return;
						}
						for (size_t i = 0; i < chunkLength; i++)
							info.palette[4 * i + 3] = in[pos++];
					}
					else if (info.colorType == 0)
					{
						if (chunkLength != 2)
						{
							error = 40;
							return;
						}
						info.key_defined = 1;
						info.key_r = info.key_g = info.key_b = 256 * in[pos] + in[pos + 1];
						pos += 2;
					}
					else if (info.colorType == 2)
					{
						if (chunkLength != 6)
						{
							error = 41;
							return;
						}
						info.key_defined = 1;
						info.key_r = 256 * in[pos] + in[pos + 1];
						pos += 2;
						info.key_g = 256 * in[pos] + in[pos + 1];
						pos += 2;
						info.key_b = 256 * in[pos] + in[pos + 1];
						pos += 2;
					}
					else
					{
						error = 42;
						return;
					}
				}
				else
				{
					if (!(in[pos + 0] & 32))
					{
						error = 69;
						return;
					}
					pos += (chunkLength + 4);
					known_type = false;
				}
				pos += 4;
			}
			uint bpp = getBpp( info );
			std::vector<uchar> scanlines(
					((info.width * (info.height * bpp + 7)) / 8) + info.height );
			Zlib zlib;
			error = zlib.decompress( scanlines, idat );
			if (error) return;
			size_t bytewidth = (bpp + 7) / 8, outlength = (info.height * info.width * bpp + 7) / 8;
			out.resize( outlength );
			uchar* out_ = outlength ? &out[0] : 0;
			if (info.interlaceMethod == 0)
			{
				size_t linestart = 0, linelength = (info.width * bpp + 7) / 8;
				if (bpp >= 8)
					for (uint y = 0; y < info.height; y++)
					{
						uint filterType = scanlines[linestart];
						const uchar* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width *
							bytewidth];
						unFilterScanline( &out_[linestart - y], &scanlines[linestart + 1], prevline,
										  bytewidth, filterType, linelength );
						if (error) return;
						linestart += (1 + linelength);
					}
				else
				{
					std::vector<uchar> templine( (info.width * bpp + 7) >> 3 );
					for (size_t y = 0, obp = 0; y < info.height; y++)
					{
						uint filterType = scanlines[linestart];
						const uchar* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width *
							bytewidth];
						unFilterScanline( &templine[0], &scanlines[linestart + 1], prevline,
										  bytewidth, filterType, linelength );
						if (error) return;
						for (size_t bp = 0; bp < info.width * bpp; )
							setBitOfReversedStream( obp, out_, readBitFromReversedStream( bp,
								&templine[0] ) );
						linestart += (1 + linelength);
					}
				}
			}
			else
			{
				size_t passw[7] = { (info.width + 7) / 8, (info.width + 3) / 8,
					(info.width + 3) / 4, (info.width + 1) / 4,
					(info.width + 1) / 2, (info.width + 0) / 2,
					(info.width + 0) / 1 };
				size_t passh[7] = { (info.height + 7) / 8, (info.height + 7) / 8,
					(info.height + 3) / 8, (info.height + 3) / 4,
					(info.height + 1) / 4, (info.height + 1) / 2,
					(info.height + 0) / 2 };
				size_t passstart[7] = { 0 };
				size_t pattern[28] = { 0, 4, 0, 2, 0, 1, 0, 0, 0, 4, 0, 2, 0, 1, 8, 8, 4, 4, 2, 2,
					1, 8, 8, 8, 4, 4, 2, 2 };
				for (int i = 0; i < 6; i++)
					passstart[i + 1] = passstart[i] + passh[i] * ((passw[i] ? 1 : 0) +
																  (passw[i] * bpp + 7) / 8);
				std::vector<uchar> scanlineo( (info.width * bpp + 7) / 8 ), scanlinen(
						(info.width * bpp + 7) / 8 );
				for (int i = 0; i < 7; i++)
					adam7Pass( &out_[0], &scanlinen[0], &scanlineo[0], &scanlines[passstart[i]],
							   info.width, pattern[i], pattern[i + 7], pattern[i + 14],
							   pattern[i + 21], passw[i], passh[i], bpp );
			}
			if (convert_to_rgba32 && (info.colorType != 6 || info.bitDepth != 8))
			{
				std::vector<uchar> data = out;
				error = convert( out, &data[0], info, info.width, info.height );
			}
		}
		void readPngHeader( const uchar* in, size_t inlength )
		{
			if (inlength < 29)
			{
				error = 27;
				return;
			}
			if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 || in[4] != 13 ||
				in[5] != 10 || in[6] != 26 || in[7] != 10)
			{
				error = 28;
				return;
			} //no PNG signature
			if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R')
			{
				error = 29;
				return;
			}
			info.width = read32bitInt( &in[16] );
			info.height = read32bitInt( &in[20] );
			info.bitDepth = in[24];
			info.colorType = in[25];
			info.compressionMethod = in[26];
			if (in[26] != 0)
			{
				error = 32;
				return;
			}
			info.filterMethod = in[27];
			if (in[27] != 0)
			{
				error = 33;
				return;
			}
			info.interlaceMethod = in[28];
			if (in[28] > 1)
			{
				error = 34;
				return;
			}
			error = checkColorValidity( info.colorType, info.bitDepth );
		}
		void unFilterScanline( uchar* recon, const uchar* scanline, const uchar* precon,
							   size_t bytewidth, uint filterType, size_t length )
		{
			switch (filterType)
			{
			case 0:
				for (size_t i = 0; i < length; i++) recon[i] = scanline[i];
				break;
			case 1:
				for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
				for (size_t i = bytewidth; i < length; i++)
					recon[i] = scanline[i] + recon[i - bytewidth];
				break;
			case 2:
				if (precon)
					for (size_t i = 0; i < length; i++)
						recon[i] = scanline[i] + precon[i];
				else for (size_t i = 0; i < length; i++) recon[i] = scanline[i];
				break;
			case 3:
				if (precon)
				{
					for (size_t i = 0; i < bytewidth; i++)
						recon[i] = scanline[i] + precon[i] / 2;
					for (size_t i = bytewidth; i < length; i++)
						recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
				}
				else
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
					for (size_t i = bytewidth; i < length; i++)
						recon[i] = scanline[i] + recon[i - bytewidth] / 2;
				}
				break;
			case 4:
				if (precon)
				{
					for (size_t i = 0; i < bytewidth; i++)
						recon[i] = scanline[i] + paethPredictor( 0, precon[i], 0 );
					for (size_t i = bytewidth; i < length; i++)
						recon[i] = scanline[i] +
						paethPredictor( recon[i - bytewidth], precon[i], precon[i -
																				bytewidth] );
				}
				else
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
					for (size_t i = bytewidth; i < length; i++)
						recon[i] = scanline[i] + paethPredictor( recon[i - bytewidth], 0, 0 );
				}
				break;
			default:
				error = 36;
				return;
			}
		}
		void
			adam7Pass( uchar* out, uchar* linen, uchar* lineo, const uchar* in, uint w, size_t passleft,
					   size_t passtop, size_t spacex, size_t spacey, size_t passw, size_t passh,
					   uint bpp )
		{
			if (passw == 0) return;
			size_t bytewidth = (bpp + 7) / 8, linelength = 1 + ((bpp * passw + 7) / 8);
			for (uint y = 0; y < passh; y++)
			{
				uchar filterType = in[y * linelength], * prevline = (y == 0) ? 0 : lineo;
				unFilterScanline( linen, &in[y * linelength + 1], prevline, bytewidth, filterType,
								  (w * bpp + 7) / 8 );
				if (error) return;
				if (bpp >= 8)
					for (size_t i = 0; i < passw; i++)
						for (size_t b = 0; b < bytewidth; b++)
							out[bytewidth * w * (passtop + spacey * y) +
							bytewidth * (passleft + spacex * i) + b] = linen[bytewidth * i + b];
				else
					for (size_t i = 0; i < passw; i++)
					{
						size_t obp = bpp * w * (passtop + spacey * y) +
							bpp * (passleft + spacex * i), bp = i * bpp;
						for (size_t b = 0; b < bpp; b++)
							setBitOfReversedStream( obp, out, readBitFromReversedStream( bp,
								&linen[0] ) );
					}
				uchar* temp = linen;
				linen = lineo;
				lineo = temp;
			}
		}
		static uint readBitFromReversedStream( size_t& bitp, const uchar* bits )
		{
			uint result = (bits[bitp >> 3] >> (7 - (bitp & 0x7))) & 1;
			bitp++;
			return result;
		}
		static uint readBitsFromReversedStream( size_t& bitp, const uchar* bits, uint nbits )
		{
			uint result = 0;
			for (size_t i = nbits - 1; i < nbits; i--)
				result += ((readBitFromReversedStream( bitp, bits )) << i);
			return result;
		}
		void setBitOfReversedStream( size_t& bitp, uchar* bits, uint bit )
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
			{
				if (!(bd == 8 || bd == 16))return 37; else return 0;
			}
			else if (colorType == 0)
			{
				if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16))
					return 37;
				else return 0;
			}
			else if (colorType == 3)
			{
				if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8))
					return 37;
				else return 0;
			}
			else return 31;
		}
		uint getBpp( const Info& info )
		{
			if (info.colorType == 2) return (3 * info.bitDepth);
			else if (info.colorType >= 4) return (info.colorType - 2) * info.bitDepth;
			else return info.bitDepth;
		}
		int convert( vector<uchar>& out, const uchar* in, Info& infoIn, uint w, uint h )
		{
			size_t numpixels = w * h, bp = 0;
			out.resize( numpixels * 4 );
			uchar* out_ = out.empty() ? 0 : &out[0];
			if (infoIn.bitDepth == 8 && infoIn.colorType == 0)
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[i];
					out_[4 * i + 3] = (infoIn.key_defined && in[i] == infoIn.key_r) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 2)
				for (size_t i = 0; i < numpixels; i++)
				{
					for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[3 * i + c];
					out_[4 * i + 3] = (infoIn.key_defined == 1 && in[3 * i + 0] == infoIn.key_r &&
									   in[3 * i + 1] == infoIn.key_g &&
									   in[3 * i + 2] == infoIn.key_b) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 3)
				for (size_t i = 0; i < numpixels; i++)
				{
					if (4U * in[i] >= infoIn.palette.size()) return 46;
					for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * in[i] + c];
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 4)
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i + 0];
					out_[4 * i + 3] = in[2 * i + 1];
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 6)
				for (size_t i = 0; i < numpixels; i++)
					for (size_t c = 0; c < 4; c++)
						out_[4 * i + c] = in[4 * i + c]; //RGB with alpha
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 0)
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i];
					out_[4 * i + 3] = (infoIn.key_defined &&
									   256U * in[i] + in[i + 1] == infoIn.key_r) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 2)
				for (size_t i = 0; i < numpixels; i++)
				{
					for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[6 * i + 2 * c];
					out_[4 * i + 3] = (infoIn.key_defined &&
									   256U * in[6 * i + 0] + in[6 * i + 1] == infoIn.key_r &&
									   256U * in[6 * i + 2] + in[6 * i + 3] == infoIn.key_g &&
									   256U * in[6 * i + 4] + in[6 * i + 5] == infoIn.key_b) ? 0
						: 255;
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 4)
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[4 * i];
					out_[4 * i + 3] = in[4 * i + 2];
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 6)
				for (size_t i = 0; i < numpixels; i++)
					for (size_t c = 0; c < 4; c++)
						out_[4 * i + c] = in[8 * i + 2 * c]; //RGB with alpha
			else if (infoIn.bitDepth < 8 && infoIn.colorType == 0)
				for (size_t i = 0; i < numpixels; i++)
				{
					uint value = (readBitsFromReversedStream( bp, in, infoIn.bitDepth ) * 255) /
						((1 << infoIn.bitDepth) - 1);
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = (uchar)(value);
					out_[4 * i + 3] = (infoIn.key_defined && value &&
									   ((1U << infoIn.bitDepth) - 1U) == infoIn.key_r &&
									   ((1U << infoIn.bitDepth) - 1U)) ? 0 : 255;
				}
			else if (infoIn.bitDepth < 8 && infoIn.colorType == 3)
				for (size_t i = 0; i < numpixels; i++)
				{
					uint value = readBitsFromReversedStream( bp, in, infoIn.bitDepth );
					if (4 * value >= infoIn.palette.size()) return 47;
					for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * value + c];
				}
			return 0;
		}
		uchar paethPredictor( short a, short b, short c )
		{
			short p = a + b - c, pa = p > a ? (p - a) : (a - p), pb =
				p > b ? (p - b) : (b - p), pc = p > c ? (p - c) : (c - p);
			return (uchar)((pa <= pb && pa <= pc) ? a : pb <= pc ? b : c);
		}
	};
	PNG decoder;
	decoder.decode( out_image, in_png, in_size, convert_to_rgba32 );
	image_width = decoder.info.width;
	image_height = decoder.info.height;
	return decoder.error;
}

bool LoadPNGFile( const char* fileName, uint& w, uint& h, vector<uchar>& image )
{
	vector<uchar> buffer;
	loadBinaryFile( buffer, fileName );
	return decodePNG( image, w, h, &buffer[0], (uint)buffer.size() ) == 0;
}

#ifdef _WIN64

// internal vars
static bool ldown = false;
GLFWwindow* window = 0;
int nativeWidth = 1024, nativeHeight = 640;

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
			game.SetPenPos( p.x, p.y );
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

struct saved_state
{
	int32_t x;
	int32_t y;
};
struct engine
{
	struct android_app* app;
	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width, height;
	struct saved_state state;
};

static void engine_init_display( struct engine* engine )
{
#if 1
	// from endless tunnel
	const EGLint attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // request OpenGL ES 2.0
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 16, EGL_NONE
	};
#else
	// from Microsoft's sample
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
		EGL_NONE
	};
#endif
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
#if 1
	// from endless tunnel
	EGLint attribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }; // OpenGL 2.0
	context = eglCreateContext( display, config, NULL, attribList );
#else
	// from Microsoft's sample
	context = eglCreateContext( display, config, NULL, NULL );
#endif
	eglMakeCurrent( display, surface, surface, context );
	eglQuerySurface( display, surface, EGL_WIDTH, &w );
	eglQuerySurface( display, surface, EGL_HEIGHT, &h );
	glViewport( 0, 0, w, h ); // let's assume this never changes
	game.SetScreenSize( w, h );
	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w, engine->height = h;
}

static void engine_draw_frame( struct engine* engine )
{
	if (engine->display == NULL) return;
	game.Tick( 0 /* for now */ );
	eglSwapBuffers( engine->display, engine->surface );
}

static void engine_term_display( struct engine* engine )
{
	if (engine->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent( engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		if (engine->context != EGL_NO_CONTEXT)
			eglDestroyContext( engine->display, engine->context );
		if (engine->surface != EGL_NO_SURFACE)
			eglDestroySurface( engine->display, engine->surface );
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
		game.SetPenPos( engine->state.x, engine->state.y );
		return 1;
	}
	return 0;
}

static JNIEnv* jniEnv = 0;
static android_app* androidApp;

bool SetImmersiveMode( JNIEnv* env, android_app* iandroid_app )
{
	jclass activityClass = env->FindClass( "android/app/NativeActivity" );
	jclass windowClass = env->FindClass( "android/view/Window" );
	jclass viewClass = env->FindClass( "android/view/View" );
	jmethodID getWindow = env->GetMethodID( activityClass, "getWindow", "()Landroid/view/Window;" );
	jmethodID getDecorView = env->GetMethodID( windowClass, "getDecorView",
											   "()Landroid/view/View;" );
	jmethodID setSystemUiVisibility = env->GetMethodID( viewClass, "setSystemUiVisibility",
														"(I)V" );
	jmethodID getSystemUiVisibility = env->GetMethodID( viewClass, "getSystemUiVisibility", "()I" );
	jobject windowObj = env->CallObjectMethod( iandroid_app->activity->clazz, getWindow );
	jobject decorViewObj = env->CallObjectMethod( windowObj, getDecorView );
	// get flag ids
	jfieldID id_SYSTEM_UI_FLAG_LAYOUT_STABLE = env->GetStaticFieldID( viewClass,
																	  "SYSTEM_UI_FLAG_LAYOUT_STABLE",
																	  "I" );
	jfieldID id_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = env->GetStaticFieldID( viewClass,
																			   "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION",
																			   "I" );
	jfieldID id_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = env->GetStaticFieldID( viewClass,
																		  "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN",
																		  "I" );
	jfieldID id_SYSTEM_UI_FLAG_HIDE_NAVIGATION = env->GetStaticFieldID( viewClass,
																		"SYSTEM_UI_FLAG_HIDE_NAVIGATION",
																		"I" );
	jfieldID id_SYSTEM_UI_FLAG_FULLSCREEN = env->GetStaticFieldID( viewClass,
																   "SYSTEM_UI_FLAG_FULLSCREEN",
																   "I" );
	jfieldID id_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = env->GetStaticFieldID( viewClass,
																		 "SYSTEM_UI_FLAG_IMMERSIVE_STICKY",
																		 "I" );
	// get flags
	const int flag_SYSTEM_UI_FLAG_LAYOUT_STABLE = env->GetStaticIntField( viewClass,
																		  id_SYSTEM_UI_FLAG_LAYOUT_STABLE );
	const int flag_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = env->GetStaticIntField( viewClass,
																				   id_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION );
	const int flag_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = env->GetStaticIntField( viewClass,
																			  id_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN );
	const int flag_SYSTEM_UI_FLAG_HIDE_NAVIGATION = env->GetStaticIntField( viewClass,
																			id_SYSTEM_UI_FLAG_HIDE_NAVIGATION );
	const int flag_SYSTEM_UI_FLAG_FULLSCREEN = env->GetStaticIntField( viewClass,
																	   id_SYSTEM_UI_FLAG_FULLSCREEN );
	const int flag_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = env->GetStaticIntField( viewClass,
																			 id_SYSTEM_UI_FLAG_IMMERSIVE_STICKY );
	// get current immersiveness
	const int currentVisibility = env->CallIntMethod( decorViewObj, getSystemUiVisibility );
	const bool is_SYSTEM_UI_FLAG_LAYOUT_STABLE =
		(currentVisibility & flag_SYSTEM_UI_FLAG_LAYOUT_STABLE) != 0;
	const bool is_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION =
		(currentVisibility & flag_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION) != 0;
	const bool is_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN =
		(currentVisibility & flag_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN) != 0;
	const bool is_SYSTEM_UI_FLAG_HIDE_NAVIGATION =
		(currentVisibility & flag_SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0;
	const bool is_SYSTEM_UI_FLAG_FULLSCREEN =
		(currentVisibility & flag_SYSTEM_UI_FLAG_FULLSCREEN) != 0;
	const bool is_SYSTEM_UI_FLAG_IMMERSIVE_STICKY =
		(currentVisibility & flag_SYSTEM_UI_FLAG_IMMERSIVE_STICKY) != 0;
	const auto isAlreadyImmersive =
		is_SYSTEM_UI_FLAG_LAYOUT_STABLE &&
		is_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION &&
		is_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN &&
		is_SYSTEM_UI_FLAG_HIDE_NAVIGATION &&
		is_SYSTEM_UI_FLAG_FULLSCREEN &&
		is_SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
	auto success = true;
	if (!isAlreadyImmersive)
	{
		const int flag =
			flag_SYSTEM_UI_FLAG_LAYOUT_STABLE |
			flag_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
			flag_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
			flag_SYSTEM_UI_FLAG_HIDE_NAVIGATION |
			flag_SYSTEM_UI_FLAG_FULLSCREEN |
			flag_SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		env->CallVoidMethod( decorViewObj, setSystemUiVisibility, flag );
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
		engine->app->savedState = malloc( sizeof( struct saved_state ) );
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof( struct saved_state );
		break;
	case APP_CMD_INIT_WINDOW: // do things when the window becomes visible
		if (engine->app->window != NULL)
		{
			engine_init_display( engine );
			game.Init();
			engine_draw_frame( engine );
		}
		break;
	case APP_CMD_TERM_WINDOW: // do things when the window is closed or hidden
		engine_term_display( engine );
		break;
	case APP_CMD_GAINED_FOCUS: // do things when we gain focus
		break;
	case APP_CMD_RESUME:
		androidApp->activity->vm->AttachCurrentThread( &jniEnv, NULL );
		SetImmersiveMode( GetJniEnv(), androidApp );
		/* mgr->OnResume(); */
		break;
	case APP_CMD_LOST_FOCUS:
		engine->animating = 0;
		engine_draw_frame( engine );
		break;
	}
}

static void GetDCIMPath( JNIEnv* env, android_app* app, const char* param, char* dst )
{
	// from: https://stackoverflow.com/questions/29998820/dcim-directory-path-on-android-return-value
	jclass envClass = env->FindClass( "android/os/Environment" );
	jmethodID getExtStorageDirectoryMethod = env->GetStaticMethodID( envClass,
																	 "getExternalStoragePublicDirectory",
																	 "(Ljava/lang/String;)Ljava/io/File;" );
	jfieldID fieldId = env->GetStaticFieldID( envClass, param, "Ljava/lang/String;" );
	jstring jstrParam = (jstring)env->GetStaticObjectField( envClass, fieldId );
	jobject extStorageFile = env->CallStaticObjectMethod( envClass, getExtStorageDirectoryMethod,
														  jstrParam );
	jmethodID getPathMethod = env->GetMethodID( env->FindClass( "java/io/File" ), "getPath",
												"()Ljava/lang/String;" );
	jstring extStoragePath = (jstring)env->CallObjectMethod( extStorageFile, getPathMethod );
	const char* extStoragePathString = env->GetStringUTFChars( extStoragePath, NULL );
	strcpy( dst, extStoragePathString );
	env->ReleaseStringUTFChars( extStoragePath, extStoragePathString );
	app->activity->vm->DetachCurrentThread();
}

void android_main( struct android_app* state )
{
	android_fopen_set_asset_manager( state->activity->assetManager );
	androidApp = state;
	SetImmersiveMode( GetJniEnv(), state );
	struct engine engine;
	memset( &engine, 0, sizeof( engine ) );
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;
	if (state->savedState != NULL) engine.state = *(struct saved_state*)state->savedState;
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