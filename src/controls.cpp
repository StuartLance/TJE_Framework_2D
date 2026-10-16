#include "controls.h"
#include "input.h"

static char padDirection()
{
	return Input::gamepads[0].direction | (char)Input::gamepads[0].hat;
}

Vector2 Controls::direction()
{
	Vector2 dir;
	char pad = padDirection();
	if (Input::isKeyPressed(SDL_SCANCODE_LEFT) || (pad & PAD_LEFT)) dir.x -= 1;
	if (Input::isKeyPressed(SDL_SCANCODE_RIGHT) || (pad & PAD_RIGHT)) dir.x += 1;
	if (Input::isKeyPressed(SDL_SCANCODE_UP) || (pad & PAD_UP)) dir.y -= 1;
	if (Input::isKeyPressed(SDL_SCANCODE_DOWN) || (pad & PAD_DOWN)) dir.y += 1;
	if (dir.x != 0 || dir.y != 0)
		dir.normalize();
	return dir;
}

bool Controls::upPressed()
{
	return Input::wasKeyPressed(SDL_SCANCODE_UP) || Input::gamepads[0].didDirectionChanged(PAD_UP);
}

bool Controls::downPressed()
{
	return Input::wasKeyPressed(SDL_SCANCODE_DOWN) || Input::gamepads[0].didDirectionChanged(PAD_DOWN);
}

bool Controls::buttonA()
{
	return Input::isKeyPressed(SDL_SCANCODE_Z) || Input::gamepads[0].isButtonPressed(A_BUTTON);
}

bool Controls::buttonAPressed()
{
	return Input::wasKeyPressed(SDL_SCANCODE_Z) || Input::wasKeyPressed(SDL_SCANCODE_RETURN) || Input::gamepads[0].wasButtonPressed(A_BUTTON);
}

bool Controls::buttonBPressed()
{
	return Input::wasKeyPressed(SDL_SCANCODE_X) || Input::gamepads[0].wasButtonPressed(B_BUTTON);
}

bool Controls::backPressed()
{
	return Input::wasKeyPressed(SDL_SCANCODE_ESCAPE) || Input::gamepads[0].wasButtonPressed(START_BUTTON);
}
