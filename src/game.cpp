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
		EntityMesh* entity = new EntityMesh("arbol");
		entity->model->setTranslation(pos.x, pos.y, pos.z);
		entity->mesh = Mesh::Get("data/biglib/SamuraiPack/Environment/SM_Env_Tree_04_74.obj");

		entity->texture = Texture::Get("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
		entity->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
		scene->entities.push_back(entity);
	}
}

void Game::update(double seconds_elapsed)
{
	if (Input::wasKeyPressed(SDL_SCANCODE_E)) { edit_mode = (edit_mode + 1) % 2; } //move faster with left shift
	if (Input::wasKeyPressed(SDL_SCANCODE_SPACE)) { cosa = (cosa + 1) % 2; } //move faster with left shift

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
		if (Input::isKeyPressed(SDL_SCANCODE_UP)) ent->model->translate(0.0f, 0.0f, 1.0f * 10 * seconds_elapsed);
		if (Input::isKeyPressed(SDL_SCANCODE_DOWN)) ent->model->translate(0.0f, 0.0f, -1.0f * 10 * seconds_elapsed);
		if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) ent->model->translate(1.0f * 10 * seconds_elapsed, 0.0f, 0.0f);
		if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) ent->model->translate(-1.0f * 10 * seconds_elapsed, 0.0f, 0.0f);

		if (Input::isKeyPressed(SDL_SCANCODE_Z)) ent->model->rotate(90.0f * seconds_elapsed * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		if (Input::isKeyPressed(SDL_SCANCODE_X)) ent->model->rotate(-90.0f * seconds_elapsed * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

		if (Input::wasKeyPressed(SDL_SCANCODE_BACKSPACE))scene->entities.pop_back();
		if (Input::wasKeyPressed(SDL_SCANCODE_KP_ENTER)) scene->exportEscene();

		//to navigate with the mouse fixed in the middle
		if (mouse_locked)
			Input::centerMouse();

		AddObjectInFront();
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

