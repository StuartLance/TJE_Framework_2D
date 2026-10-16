/*  NES-style controls: arrows/left stick + A (Z key) + B (X key) + back (ESC / START).
	Every stage reads input through here so keyboard and gamepad behave the same.
*/
#ifndef CONTROLS_H
#define CONTROLS_H

#include "framework.h"

namespace Controls
{
	Vector2 direction();	//normalized movement direction (0,0 if idle)
	bool upPressed();		//edge-triggered, for menus
	bool downPressed();
	bool buttonA();			//held
	bool buttonAPressed();	//edge-triggered
	bool buttonBPressed();
	bool backPressed();
}

#endif
