#include "game.h"
#include "utils.h"
#include "input.h"
#include "image.h"
#include "world.h"

#include <cmath>
#include <algorithm>

Game* Game::instance = NULL;

static const char* SOUNDS[] = { "select", "confirm", "key", "door", "alert", "caught", "throw", "land", "exit", "win", "music" };

Game::Game(int window_width, int window_height, SDL_Window* window)
{
	this->window_width = window_width;
	this->window_height = window_height;
	this->window = window;
	this->renderer = NULL;
	instance = this;
	must_exit = false;

	fps = 0;
	frame = 0;
	time = 0.0f;
	elapsed_time = 0.0f;

	//every asset is loaded once here to avoid hiccups during the game
	font.load("data/bitmap-font-white.tga");
	font_black.load("data/bitmap-font-black.tga");
	minifont.load("data/mini-font-white-4x6.tga");
	tileset.load("data/tileset.png");
	player_sheet.load("data/player.png");
	guard_sheet.load("data/guard.png");

	enableAudio();
	for (const char* name : SOUNDS)
		synth.loadSample(std::string("data/sounds/") + name + ".wav");
	synth.playSample("data/sounds/music.wav", 0.25f, true);

	world = new World();
	stages[STAGE_INTRO] = new IntroStage();
	stages[STAGE_TUTORIAL] = new TutorialStage();
	stages[STAGE_PLAY] = new PlayStage();
	stages[STAGE_END] = new EndStage();
	current_stage = NULL;
	setStage(STAGE_INTRO);
}

void Game::setStage(eStageId id)
{
	current_stage = stages[id];
	current_stage->time_in_stage = 0.0f;
	current_stage->onEnter();
	fade_timer = 0.3f;
}

void Game::playSound(const std::string& name, float volume)
{
	synth.playSample("data/sounds/" + name + ".wav", volume, false);
}

//what to do when the image has to be draw
void Game::render(void)
{
	Image framebuffer(160, 120); //do not change framebuffer size

	current_stage->render(framebuffer);

	//fade in after every stage change
	if (fade_timer > 0)
		framebuffer.fillBlend(Color(0, 0, 0, (unsigned char)(255 * fade_timer / 0.3f)));

	//send image to screen
	showFramebuffer(&framebuffer);
}

void Game::update(double seconds_elapsed)
{
	//avoid huge steps (e.g. when dragging the window)
	float dt = (float)std::min(seconds_elapsed, 0.05);
	fade_timer -= dt;
	current_stage->time_in_stage += dt;
	current_stage->update(dt);
}

//Keyboard event handler (sync input)
void Game::onKeyDown( SDL_KeyboardEvent event )
{
}

void Game::onKeyUp(SDL_KeyboardEvent event)
{
}

void Game::onGamepadButtonDown(SDL_JoyButtonEvent event)
{

}

void Game::onGamepadButtonUp(SDL_JoyButtonEvent event)
{

}

void Game::onMouseMove(SDL_MouseMotionEvent event)
{
}

void Game::onMouseButtonDown( SDL_MouseButtonEvent event )
{
}

void Game::onMouseButtonUp(SDL_MouseButtonEvent event)
{
}

void Game::onMouseWheel(SDL_MouseWheelEvent event)
{
}

void Game::onResize(int width, int height)
{
    std::cout << "window resized: " << width << "," << height << std::endl;
	#ifdef USE_OPENGL
		glViewport( 0,0, width, height );
	#endif
	window_width = width;
	window_height = height;
}

//sends the image to the framebuffer of the GPU
void Game::showFramebuffer(Image* img)
{
	float startx = -1.0; float starty = -1.0;
	float width = 2.0; float height = 2.0;

	//center in window
	float real_aspect = window_width / (float)window_height;
	float desired_aspect = img->width / (float)img->height;
	float diff = desired_aspect / real_aspect;
	width *= diff;
	startx = -diff;

#ifdef USE_OPENGL
	static GLuint texture_id = -1;
	static GLuint shader_id = -1;
	if (!texture_id)
		glGenTextures(1, &texture_id);

	//upload as texture
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
	glDisable(GL_CULL_FACE); glDisable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(startx, starty + height);
	glTexCoord2f(1.0, 0.0); glVertex2f(startx + width, starty + height);
	glTexCoord2f(1.0, 1.0); glVertex2f(startx + width, starty);
	glTexCoord2f(0.0, 1.0); glVertex2f(startx, starty);
	glEnd();
#else
	static SDL_Texture* texture = NULL;
	static int tex_width = 0;
	static int tex_height = 0;
	if (!texture || tex_width != img->width || tex_height != img->height)
	{
		if(texture)
			SDL_DestroyTexture(texture);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET, img->width, img->height);
		tex_width = img->width;
		tex_height = img->height;
	}

	//SDL_RenderClear(renderer);
	SDL_Rect rect = { 0, 0, (int)img->width, (int)img->height };
	SDL_UpdateTexture(texture, &rect, img->pixels, img->width*4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

#endif
	/* this version resizes the image which is slower
	Image resized = *img;
	//resized.quantize(1); //change this line to have a more retro look
	resized.scale(window_width, window_height);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (1) //flip
	{
	glRasterPos2f(-1, 1);
	glPixelZoom(1, -1);
	}
	glDrawPixels( resized.width, resized.height, GL_RGBA, GL_UNSIGNED_BYTE, resized.pixels );
	*/
}

//AUDIO STUFF ********************

SDL_AudioSpec audio_spec;

void AudioCallback(void*  userdata,
	Uint8* stream,
	int    len)
{
	static double audio_time = 0;

	memset(stream, 0, len);//clear
	if (!Game::instance)
		return;

	Game::instance->onAudio((float*)stream, len / sizeof(float), audio_time, audio_spec);
	audio_time += len / (double)audio_spec.freq;
}

void Game::enableAudio()
{
	SDL_memset(&audio_spec, 0, sizeof(audio_spec)); /* or SDL_zero(want) */
	audio_spec.freq = 48000;
	audio_spec.format = AUDIO_F32;
	audio_spec.channels = 1;
	audio_spec.samples = 1024;
	audio_spec.callback = AudioCallback; /* you wrote this function elsewhere. */
	SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
	if (!audio_device) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		exit(-1);
	}
	SDL_PauseAudioDevice(audio_device, 0);
}

void Game::onAudio(float *buffer, unsigned int len, double time, SDL_AudioSpec& audio_spec)
{
	//fill the audio buffer using our custom retro synth
	synth.generateAudio(buffer, len, audio_spec);
}
