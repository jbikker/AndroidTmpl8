#include "template.h"

void Game::Init()
{
	// opengl state
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	// cpu-side pixel buffer
	screen = Surface( 320, 192 );
	pixels = CreateTexture( screen.buffer, 320, 192 );
	shader = LoadShader();
	// load a sound and play it
	loud.init();
	std::vector<unsigned char> buffer;
	loadBinaryFile( buffer, "coin.wav" );
	sound.loadMem( buffer.data(), (int)buffer.size(), true, true );
	loud.play( sound );
	// load a png from the assets folder
	bluePrint = new Surface( "blueprint.png" );
}

void Game::Tick( const float deltaTime )
{
	// some OpenGL code
	glClearColor( 0.5f, 0.5f, 1, 1 );
	glClear( GL_COLOR_BUFFER_BIT );
	// surface operation
	bluePrint->CopyTo( &screen, 50, 50 );
	// cross hairs
	int cx = (cursorx * 320) / scrwidth;
	int cy = (cursory * 192) / scrheight;
	if (cx >= 0 && cx < 320) screen.VLine( cx, 0, 192, 0xff0000 );
	if (cy >= 0 && cy < 192) screen.HLine( 0, cy, 320, 0x00ff00 );
	// render pixel buffer
	glUseProgram( shader );
	glDisable( GL_BLEND );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, pixels );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 320, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				  screen.buffer );
	glUniform1i( glGetUniformLocation( shader, "C" ), 0 );
	glUniform1f( glGetUniformLocation( shader, "a" ), 1.0f );
	DrawQuad();
	glEnable( GL_BLEND );
}

void Game::Shutdown()
{
}