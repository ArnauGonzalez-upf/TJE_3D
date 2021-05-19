#include "game.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"

#include <cmath>

//some globals
Mesh* mesh = NULL;
Texture* texture = NULL;
Shader* shader = NULL;
Animation* anim = NULL;
float angle = 0;
float mouse_speed = 100.0f;
FBO* fbo = NULL;

Game* Game::instance = NULL;

Game::Game(int window_width, int window_height, SDL_Window* window)
{
	this->window_width = window_width;
	this->window_height = window_height;
	this->window = window;
	instance = this;
	must_exit = false;

	fps = 0;
	frame = 0;
	time = 0.0f;
	elapsed_time = 0.0f;
	mouse_locked = false;

	//OpenGL flags
	glEnable( GL_CULL_FACE ); //render both sides of every triangle
	glEnable( GL_DEPTH_TEST ); //check the occlusions using the Z buffer

	//create our camera
	camera = new Camera();
	camera->lookAt(Vector3(0.f,100.f, 100.f),Vector3(0.f,0.f,0.f), Vector3(0.f,1.f,0.f)); //position the camera and point to 0,0,0
	camera->setPerspective(70.f,window_width/(float)window_height,0.1f,10000.f); //set the projection, we want to be perspective

	//hide the cursor
	SDL_ShowCursor(!mouse_locked); //hide or show the mouse

	scene = new Scene();

	//fbo_shadow = new FBO();
	//fbo_shadow->setDepthOnly(2048, 2048);

	cosa = false;

	new EditorStage();

	edit_mode = true;
	combat_mode = false;

	comb_m = DEFENSE;
	combat_counter = 4;
	attack_change = true;
}

//what to do when the image has to be draw
void Game::render(void)
{
	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default
	camera->enable();

	//cosa = false;
	if (!edit_mode)
	{
		if (!combat_mode) {
			if (cosa)
			{
				Vector3 eye = *(scene->player->model) * Vector3(0.0f, 3.0f, -3.0f);
				Vector3 center = *(scene->player->model) * Vector3(0.0f, 2.0f, -0.1f);
				Vector3 up = scene->player->model->rotateVector(Vector3(0.0f, 1.0f, 0.0f));
				camera->lookAt(eye, center, up);
			}
			else
			{
				Vector3 eye = *(scene->player->model) * Vector3(0.0f, 2.0f, 0.0f);
				Vector3 center = *(scene->player->model) * Vector3(0.0f, 1.99f, 0.1f);
				Vector3 up = scene->player->model->rotateVector(Vector3(0.0f, 1.0f, 0.0f));
				camera->lookAt(eye, center, up);
			}
		}
		else {
			Vector3 eye = *(scene->player->model) * Vector3(0.0f, 2.0f, 0.0f);
			Vector3 center = *(scene->player->model) * Vector3(0.0f, 1.99f, 0.1f);
			Vector3 up = scene->player->model->rotateVector(Vector3(0.0f, 1.0f, 0.0f));
			camera->lookAt(eye, center, up);
		}
	}

	//set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glDisable(GL_DEPTH_TEST);
	//scene->drawSky(camera);
	glEnable(GL_DEPTH_TEST);
	
	EntityLight* light = scene->lights[0];
	shadowMapping(light, camera);


	for (int i = 0; i < scene->entities.size(); ++i)
	{
		Entity* ent = scene->entities[i];
		ent->render(camera);
	}

	//Draw the floor grid
	//drawGrid();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

	if (combat_mode)
	{
		drawText(350, 200, std::to_string(attack), Vector3(1, 1, 1), 5);
		drawText(350, 400, std::to_string(defense), Vector3(1, 1, 1), 5);
	}

	//swap between front buffer and back buffer
	SDL_GL_SwapWindow(this->window);
}

void Game::shadowMapping(EntityLight* light, Camera* camera)
{
	//Bind to render inside a texture
	light->shadow_fbo->bind();
	glColorMask(false, false, false, false);
	glClear(GL_DEPTH_BUFFER_BIT);

	for (int i = 0; i < scene->entities.size(); ++i)
	{
		EntityMesh* ent = (EntityMesh*)scene->entities[i]; 

		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(*ent->model, ent->mesh->box);

		//if bounding box is inside the camera frustum then the object is probably visible
		if (light->cam->testBoxInFrustum(world_bounding.center, world_bounding.halfsize) && ent->name != "MUROS")
		{
			renderMeshWithMaterialShadow(*ent->model, ent->mesh, light);
		}
	}

	//disable it to render back to the screen
	light->shadow_fbo->unbind();
	glColorMask(true, true, true, true);
}

void Game::renderMeshWithMaterialShadow(const Matrix44& model, Mesh* mesh, EntityLight* light)
{
	//define locals to simplify coding
	Shader* shader = NULL;
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadowmap.fs");
	assert(glGetError() == GL_NO_ERROR);

	shader->enable();

	Matrix44 shadow_proj = light->cam->viewprojection_matrix;
	shader->setUniform("u_viewprojection", shadow_proj);
	shader->setUniform("u_model", model);

	mesh->render(GL_TRIANGLES);

	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS); //as default
}

void Game::update(double seconds_elapsed)
{
	if (Input::wasKeyPressed(SDL_SCANCODE_P)) { edit_mode = (edit_mode + 1) % 2; } //move faster with left shift
	if (Input::wasKeyPressed(SDL_SCANCODE_SPACE)) { cosa = (cosa + 1) % 2; } //move faster with left shift
	if (Input::wasKeyPressed(SDL_SCANCODE_TAB)) { combat_mode = (combat_mode + 1) % 2; edit_mode = false; } //move faster with left shift

	if (edit_mode)
	{
		float speed = seconds_elapsed * mouse_speed; //the speed is defined by the seconds_elapsed so it goes constant

		//example
		angle += (float)seconds_elapsed * 10.0f;

		//mouse input to rotate the cam
		if ((Input::mouse_state & SDL_BUTTON_LEFT) || mouse_locked) //is left button pressed?
		{
			camera->rotate(Input::mouse_delta.x * 0.005f, Vector3(0.0f, -1.0f, 0.0f));
			camera->rotate(Input::mouse_delta.y * 0.005f, camera->getLocalVector(Vector3(-1.0f, 0.0f, 0.0f)));
		}

		//async input to move the camera around
		if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift
		if (Input::isKeyPressed(SDL_SCANCODE_W)) camera->move(Vector3(0.0f, 0.0f, 1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) camera->move(Vector3(0.0f, 0.0f, -1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_A)) camera->move(Vector3(1.0f, 0.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_D)) camera->move(Vector3(-1.0f, 0.0f, 0.0f) * speed);

		EntityMesh* ent = (EntityMesh*)scene->entities.back();
		if (Input::isKeyPressed(SDL_SCANCODE_UP)) ent->model->translate(0.0f, 0.0f, 1.0f * seconds_elapsed);
		if (Input::isKeyPressed(SDL_SCANCODE_DOWN)) ent->model->translate(0.0f, 0.0f, -1.0f * seconds_elapsed);
		if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) ent->model->translate(1.0f * seconds_elapsed, 0.0f, 0.0f);
		if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) ent->model->translate(-1.0f * seconds_elapsed, 0.0f, 0.0f);

		if (Input::isKeyPressed(SDL_SCANCODE_Z)) ent->model->rotate(90.0f * seconds_elapsed * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		if (Input::isKeyPressed(SDL_SCANCODE_X)) ent->model->rotate(-90.0f * seconds_elapsed * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

		//move up or down the camera using Q and E
		if (Input::isKeyPressed(SDL_SCANCODE_Q)) camera->move(Vector3(0.0f, -1.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_E)) camera->move(Vector3(0.0f, 1.0f, 0.0f) * speed);

		if (Input::wasKeyPressed(SDL_SCANCODE_BACKSPACE))scene->entities.pop_back();
		if (Input::wasKeyPressed(SDL_SCANCODE_KP_ENTER)) scene->exportEscene();

		//to navigate with the mouse fixed in the middle
		if (mouse_locked)
			Input::centerMouse();

		AddObjectInFront();
	}
	else if (combat_mode)
	{
		if (comb_m == ATTACK)
		{
			if (Input::wasKeyPressed(SDL_SCANCODE_UP)) { attack = UP; attack_change = true; }
			if (Input::wasKeyPressed(SDL_SCANCODE_DOWN)) { attack = DOWN; attack_change = true; }
			if (Input::wasKeyPressed(SDL_SCANCODE_RIGHT)) { attack = RIGHT; attack_change = true; }
			if (Input::wasKeyPressed(SDL_SCANCODE_LEFT)) { attack = LEFT; attack_change = true; }

			if (attack_change)
			{
				int dice = rand() % (6 - 1 + 1) + 1;

				if (dice <= 3) {
					defense = attack;
					attack_change = false;
				}
				else {
					defense = (eSwordPositions)(rand() % (4 - 0 + 1) + 0);
					attack_change = false;
				}
			}
		}
		if (comb_m == DEFENSE)
		{
			if (attack_change)
			{
				attack = (eSwordPositions)(rand() % (4 - 0 + 1) + 0);
				attack_change = false;
			}

			if (Input::isKeyPressed(SDL_SCANCODE_UP)) defense = UP;
			if (Input::isKeyPressed(SDL_SCANCODE_DOWN)) defense = DOWN;
			if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) defense = RIGHT;
			if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) defense = LEFT;
		}
	}
	else {
		for (int i = 0; i < scene->entities.size(); ++i)
		{
			Entity* ent = scene->entities[i];

			//is an object
			if (ent->entity_type == OBJECT)
			{
				EntityMesh* oent = (EntityMesh*)ent;
				if (oent->mesh)
					oent->update(seconds_elapsed);
			}
		}
	}
}

//Keyboard event handler (sync input)
void Game::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: must_exit = true; break; //ESC key, kill the app
		case SDLK_F1: Shader::ReloadAll(); break; 
	}
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

void Game::onMouseButtonDown( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_MIDDLE) //middle mouse
	{
		mouse_locked = !mouse_locked;
		SDL_ShowCursor(!mouse_locked);
	}
}

void Game::onMouseButtonUp(SDL_MouseButtonEvent event)
{
}

void Game::onMouseWheel(SDL_MouseWheelEvent event)
{
	mouse_speed *= event.y > 0 ? 1.1 : 0.9;
}

void Game::onResize(int width, int height)
{
    std::cout << "window resized: " << width << "," << height << std::endl;
	glViewport( 0,0, width, height );
	camera->aspect =  width / (float)height;
	window_width = width;
	window_height = height;
}

void Game::AddObjectInFront()
{
	Vector3 origin = camera->eye;
	Vector3 dir = camera->getRayDirection(Input::mouse_position.x, Input::mouse_position.y, window_width, window_height);
	Vector3 pos = RayPlaneCollision(Vector3(), Vector3(0, 1, 0), origin, dir);

	if (Input::wasKeyPressed(SDL_SCANCODE_1))
	{
		EntityMesh* entity = new EntityMesh("casa");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/casa.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_2))
	{
		EntityMesh* entity = new EntityMesh("arbol");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tree_04_74.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_3))
	{
		EntityMesh* entity = new EntityMesh("camino recto");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/caminardo.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_4))
	{
		EntityMesh* entity = new EntityMesh("camino esquina");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Path_Corner_01_31.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_5))
	{
		EntityMesh* entity = new EntityMesh("camino 4");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Path_4Way_01_43.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_6))
	{
		EntityMesh* entity = new EntityMesh("tulipanes");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tulips_02_2.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_7))
	{
		EntityMesh* entity = new EntityMesh("arco");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Archway_01_23.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_8))
	{
		EntityMesh* entity = new EntityMesh("cubo 1");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Props/SM_Prop_Bucket_01_37.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_9))
	{
		EntityMesh* entity = new EntityMesh("cubo 2");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Props/SM_Prop_Bucket_02_17.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_R))
	{
		EntityMesh* entity = new EntityMesh("arbol 2");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tree_03_64.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_T))
	{
		EntityMesh* entity = new EntityMesh("arbol 3");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tree_02_57.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_Y))
	{
		EntityMesh* entity = new EntityMesh("arbol 4");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tree_01_12.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_U))
	{
		EntityMesh* entity = new EntityMesh("castillo");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/castillo.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_I))
	{
		EntityMesh* entity = new EntityMesh("puerta");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/puerta.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_O))
	{
		EntityMesh* entity = new EntityMesh("arbol 4");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Buildings/SM_Bld_Tower_01_1.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_G))
	{
		EntityMesh* entity = new EntityMesh("puerta bar");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/solo_puerta_bar.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_H))
	{
		EntityMesh* entity = new EntityMesh("bar");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Buildings/SM_Bld_Room_Long_01_54.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_J))
	{
		EntityMesh* entity = new EntityMesh("flag bar");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Props/SM_Prop_Flag_04_24.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_K))
	{
		EntityMesh* entity = new EntityMesh("flag bar");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/WesternPack/Vehicles/SM_Veh_Cart_01_0.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_L))
	{
		EntityMesh* entity = new EntityMesh("zen");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/zen.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_B))
	{
		EntityMesh* entity = new EntityMesh("tienda");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/tienda2.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_N))
	{
		EntityMesh* entity = new EntityMesh("tienda");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Props/SM_Prop_Table_02_71.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_F))
	{
		EntityMesh* entity = new EntityMesh("tienda 2");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/tienda3.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_C))
	{
		EntityMesh* entity = new EntityMesh("tienda 3");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/tienda4.obj");

		entity->texture = Texture::Get("data/biglib/GiantGeneralPack/color-atlas-new.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}

	if (Input::wasKeyPressed(SDL_SCANCODE_V))
	{
		EntityMesh* entity = new EntityMesh("tienda 3");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/GiantGeneralPack/Medieval_T/well_17.obj");

		entity->texture = Texture::Get("data/biglib/GiantGeneralPack/color-atlas-new.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}
}

