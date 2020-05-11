#include "template.h"

void Game::Init()
{
	// load a sound and play it
	std::vector<unsigned char> buffer;
	loadBinaryFile( buffer, "coin.wav" );
	sound.loadMem( buffer.data(), (int)buffer.size(), true, true );
	loud.play( sound );
	// load a png from the assets folder
	bluePrint = new Surface( "blueprint.png" );
}

void Game::Tick( const float deltaTime )
{
	// surface operation
	bluePrint->CopyTo( screen, 0, 0 );
	// cross hairs
	int cx = (cursorx * 320) / scrwidth;
	int cy = (cursory * 192) / scrheight;
	if (cx >= 0 && cx < 320) screen->VLine( cx, 0, 192, 0xff0000 );
	if (cy >= 0 && cy < 192) screen->HLine( 0, cy, 320, 0x00ff00 );
}

void Game::Shutdown()
{
}