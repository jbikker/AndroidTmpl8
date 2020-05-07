#ifndef _GAME_H
#define _GAME_H

class Game
{
public:
	Game() = default;
	void Init();
	void Tick( const float deltaTime );
	void Shutdown();
	void SetPenPos( const int x, const int y ) { cursorx = x, cursory = y; }
	void PenDown() { pendown = true; }
	void PenUp() { pendown = false; }
	void SetScreenSize( const int w, const int h ) { scrwidth = w, scrheight = h; }
private:
	int cursorx = 0, cursory = 0;
	bool pendown = false;
	int scrwidth = 1, scrheight = 1;
	Surface screen;
	GLuint pixels = -1, shader = -1;
	SoLoud::Soloud loud;
	SoLoud::Wav sound;
	Surface* bluePrint = 0;
};

#endif // _GAME_H
