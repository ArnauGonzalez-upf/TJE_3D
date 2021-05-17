#include "game.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"

#include <cmath>

std::map<int, Stage*> Stage::stages;
Stage::Stage(int pos)
{
	stages[pos] = this;
}

Stage* Stage::getStage(int pos)
{
	//Buscamos el stage correspondiente almacenado
	auto it = stages.find(pos);
	return it->second;
}

EditorStage::EditorStage() : Stage(0)
{
}

void EditorStage::AddObjectInFront(Camera* camera, Scene* scene)
{
	Vector3 origin = camera->eye;
	Vector3 dir = camera->getRayDirection(Input::mouse_position.x, Input::mouse_position.y, Game::instance->window_width, Game::instance->window_height);
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
/*
void EditorStage::update()
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
	if (Input::wasKeyPressed(SDL_SCANCODE_SPACE)) { cosa = true; } //move faster with left shift
	if (Input::wasKeyPressed(SDL_SCANCODE_V)) { cosa = false; } //move faster with left shift
	if (Input::isKeyPressed(SDL_SCANCODE_UP)) camera->move(Vector3(0.0f, 0.0f, 1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_DOWN)) camera->move(Vector3(0.0f, 0.0f, -1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) camera->move(Vector3(1.0f, 0.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) camera->move(Vector3(-1.0f, 0.0f, 0.0f) * speed);
	if (Input::wasKeyPressed(SDL_SCANCODE_KP_ENTER)) scene->exportEscene();

	//to navigate with the mouse fixed in the middle
	if (mouse_locked)
		Input::centerMouse();
}
*/