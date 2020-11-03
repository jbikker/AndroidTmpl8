#ifndef _GAME_H
#define _GAME_H

class Game
{
public:
	Game() = default;
	void Init();
	void Tick( const float deltaTime );
	void Shutdown();
	void PenPos( const int x, const int y ) { cursorx = x, cursory = y; }
	void PenDown() { pendown = true; }
	void PenUp() { pendown = false; }
	void SaveState( void* buffer, size_t& bufferSize ) { /* nothing here yet */ }
	void RestoreState( void* buffer, size_t bufferSize ) { /* nothing here yet */ }
	void SetScreenSize( const int w, const int h ) { scrwidth = w, scrheight = h; }
public:
	Surface* screen;
	Soloud loud;
private:
	int cursorx = 0, cursory = 0;
	bool pendown = false;
	int scrwidth = 1, scrheight = 1;
	GLuint pixels = -1, shader = -1, post = -1;
	SoLoud::Wav sound;
	Surface* bluePrint = 0;
};

#endif // _GAME_H
