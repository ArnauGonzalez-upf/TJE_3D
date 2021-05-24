/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class encapsulates the game, is in charge of creating the game, getting the user input, process the update and render.
*/

#ifndef GAME_H
#define GAME_H

#include "includes.h"
#include "camera.h"
#include "utils.h"
#include "scene.h"
#include "stage.h"

enum eRenderMode {
	DEFAULT,
	SHADOW
};

enum eSwordPositions : int {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum eCombatMode {
	ATTACK,
	DEFENSE
};

/*enum eCameraMode {
	STATIC_CAM,
	FOLLOWING,
	FOLLOWING_LATERAL,
	CENTERED
};*/

class Game
{
public:

	static Game* instance;
	//Stage* current;

	bool edit_mode;
	bool combat_mode;
	eCameraMode camera_mode;

	float combat_counter;
	bool attack_change;
	eSwordPositions attack;
	eSwordPositions defense;
	eCombatMode comb_m;

	//window
	SDL_Window* window;
	int window_width;
	int window_height;

	//some globals
	long frame;
    float time;
	float elapsed_time;
	int fps;
	bool must_exit;

	bool cosa;

	Scene* scene;
	eRenderMode render_mode = DEFAULT;

	FBO* fbo_shadow;

	//some vars
	Camera* camera; //our global camera
	bool mouse_locked; //tells if the mouse is locked (not seen)

	Game( int window_width, int window_height, SDL_Window* window );

	//main functions
	void render( void );
	void update( double dt );

	//events
	void onKeyDown( SDL_KeyboardEvent event );
	void onKeyUp(SDL_KeyboardEvent event);
	void onMouseButtonDown( SDL_MouseButtonEvent event );
	void onMouseButtonUp(SDL_MouseButtonEvent event);
	void onMouseWheel(SDL_MouseWheelEvent event);
	void onGamepadButtonDown(SDL_JoyButtonEvent event);
	void onGamepadButtonUp(SDL_JoyButtonEvent event);
	void onResize(int width, int height);

	void shadowMapping(EntityLight* light, Camera* camera);
	void renderMeshWithMaterialShadow(const Matrix44& model, Mesh* mesh, EntityLight* light);

	void AddObjectInFront();
};


#endif 