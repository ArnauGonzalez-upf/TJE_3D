#include "scene.h"
#include "game.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"
#include <iostream>
#include <fstream>

std::map<const char*, Scene*> Scene::s_loaded_scenes;
void Scene::registerAs(const char* filename)
{
	//this->name = name;
	s_loaded_scenes[filename] = this;
}

Scene* Scene::Get(const char* filename)
{
	auto it = s_loaded_scenes.find(filename);
	if (it != s_loaded_scenes.end())
		return it->second;
	Scene* scene = new Scene(filename);
	scene->registerAs(filename);
	return scene;
}

Scene::Scene(const char* filename)
{
	//EntityLight* sun = new EntityLight("sol");
	//lights.push_back(sun);

	TextParser* parser = new TextParser(filename);

	scene_camera = new Camera();
	scene_camera->setPerspective(70.f, Game::instance->window_width / (float)Game::instance->window_height, 0.1f, 10000.f); //set the projection, we want to be perspective

	while (!parser->eof())
	{
		std::string type = parser->getword();
		if (type == "NAME")
		{
			std::string name = parser->getword();
			std::cout << name << std::endl;
			EntityMesh* new_ent = new EntityMesh(name);
			entities.push_back(new_ent);
			if (name == "BRO")
				player = new_ent;
			if (name == "FONDO") 
				fondo = new_ent;
		}
		if (type == "MESH")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			ent->mesh = Mesh::Get(parser->getword());
		}
		if (type == "TEXTURE")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			ent->texture = Texture::Get(parser->getword());

		}
		if (type == "SHADER")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			ent->shader = Shader::Get("data/shaders/basic.vs", parser->getword());
		}
		if (type == "TYPE")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			std::string tipo = parser->getword();
			if (tipo == "STATIC") { ent->type = STATIC; static_entities.push_back(ent); }
			else { ent->type = DYNAMIC; dynamic_entities.push_back(ent); }
		}
		if (type == "FRONT")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			std::cout << x << std::endl;
			ent->model->setFrontAndOrthonormalize(Vector3(x, y, z));
		}
		if (type == "POSITION")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			ent->model->translate(x, y, z);
		}
		if (type == "LIGHT_NAME")
		{
			std::string name = parser->getword();
			EntityLight* new_ent = new EntityLight(name);
			new_ent->entity_type = LIGHT;
			lights.push_back(new_ent);
		}
		if (type == "LIGHT_TYPE")
		{
			std::string tipo = parser->getword();
			if (tipo == "DIRECTIONAL")
			{
				EntityLight* light = (EntityLight*)lights.back();
				light->light_type = DIRECTIONAL;

				light->model->translate(100, 100, 100);

				Vector3 target = Vector3(0, 0, 60);
				Vector3 pos = light->model->getTranslation();
				light->model->setFrontAndOrthonormalize(target - pos);

				light->cam->lookAt(light->model->getTranslation(), *light->model * Vector3(0, 0, 1), light->model->rotateVector(Vector3(0, 1, 0)));
				light->cam->setOrthographic(-50, 50, -50, 50, 0.1, 500);

				light->shadow_fbo = new FBO();
				light->shadow_fbo->setDepthOnly(8192, 8192);

				light->intensity = 1.0;
				light->color = Vector3(1, 1, 1);
			}
			if (tipo == "POINT")
			{
				EntityLight* light = (EntityLight*)lights.back();
				light->light_type = POINT_LIGHT;
			}
		}
		if (type == "LIGHT_POS")
		{
			EntityLight* light = (EntityLight*)lights.back();
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			light->model->translate(x, y, z);
		}
		if (type == "LIGHT_INT")
		{
			float intensity = parser->getfloat();
			EntityLight* light = (EntityLight*)lights.back();
			light->intensity = intensity;
		}
		if (type == "LIGHT_COLOR")
		{
			EntityLight* light = (EntityLight*)lights.back();
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			light->color = Vector3(x, y, z);
		}
		if (type == "CAMERA_MODE")
		{
			std::string tipo = parser->getword();
			if (tipo == "STATIC_CAM")
				camera_mode = STATIC_CAM;
			if (tipo == "FOLLOWING")
				camera_mode = FOLLOWING;
			if (tipo == "CENTERED")
				camera_mode = CENTERED;
			if (tipo == "FOLLOWING_LATERAL")
				camera_mode = FOLLOWING_LATERAL;
		}
		if (type == "CAMERA_POS")
		{
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			scene_camera->eye = Vector3(x, y, z);
		}
		if (type == "CAMERA_TARGET")
		{
			float x = parser->getfloat();
			float y = parser->getfloat();
			float z = parser->getfloat();
			scene_camera->lookAt(scene_camera->eye, Vector3(x,y,z), scene_camera->up);
		}
	}

	player_pos_orig = player->getPosition();
	player_front_orig = player->model->frontVector();
}

void Scene::drawSky(Camera* camera)
{
	Matrix44 model = fondo->getGlobalMatrix();

	//enable shader
	fondo->shader->enable();

	//upload uniforms
	fondo->shader->setUniform("u_color", Vector4(1, 1, 1, 1));
	fondo->shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	fondo->shader->setUniform("u_texture", fondo->texture, 0);
	fondo->shader->setUniform("u_model", model);
	fondo->shader->setUniform("u_time", time);

	//do the draw call
	fondo->mesh->render(GL_TRIANGLES);

	//disable shader
	fondo->shader->disable();
}

void Scene::exportEscene()
{
	std::ofstream myfile;

	//myfile.open("data/casa.txt"); //CAMBIA AL ARCHIVO QUE ESTES EDITANDO

	for (int i = 0; i < entities.size(); ++i)
	{
		EntityMesh* ent = (EntityMesh*)entities[i];

		myfile << "NAME " + ent->name + "\n";
		myfile << "MESH " + ent->mesh->name + "\n";
		myfile << "TEXTURE " + ent->texture->filename + "\n";
		myfile << "SHADER " + ent->shader->ps_filename + "\n";

		if (ent->type == STATIC) { myfile << "TYPE STATIC\n"; }
		else { myfile << "TYPE DYNAMIC\n"; }

		Vector3 pos = ent->model->getTranslation();
		myfile << "POSITION " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z) + "\n";

		Vector3 front = ent->model->frontVector();
		if (i == entities.size() - 1) { myfile << "FRONT " + std::to_string(front.x) + " " + std::to_string(front.y) + " " + std::to_string(front.z); }
		else { myfile << "FRONT " + std::to_string(front.x) + " " + std::to_string(front.y) + " " + std::to_string(front.z) + "\n"; }

	}
	myfile.close();
	std::cout << "Scene exported!" << std::endl;
}

Entity::Entity()
{
	//this->name = name; 
	model = new Matrix44();

	parent = NULL;
	children = {};
}

//get the global transformation of this object (not the relative to the parent)
//this function uses recursivity to crawl the tree upwards
Matrix44 Entity::getGlobalMatrix()
{
	if (parent) //if I have a parent, ask his global and concatenate
		return *model * parent->getGlobalMatrix();
	return *model; //otherwise just return my model as global
}

EntityMesh::EntityMesh(std::string name)
{
	this->name = name;
	mesh = NULL;
	texture = new Texture();
	shader = NULL;
	entity_type = OBJECT;
	object = false;
}

void EntityMesh::render(Camera* camera)
{
	if (!Game::instance->cosa) {
		if (this == Game::instance->scene->player) { return; }
	}

	Matrix44 model = getGlobalMatrix();

	//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
	BoundingBox world_bounding = transformBoundingBox(model, mesh->box);

	//if bounding box is inside the camera frustum then the object is probably visible
	if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize))
	{
		glDepthFunc(GL_LEQUAL);
		glBlendFunc(GL_ONE, GL_ONE);
		//enable shader
		shader->enable();

		shader->setVector3("u_ambient_light", Vector3(0.3, 0.3, 0.3));

		//upload uniforms
		shader->setUniform("u_color", Vector4(1, 1, 1, 1));
		shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
		shader->setUniform("u_texture", texture, 0);
		shader->setUniform("u_model", model);
		shader->setUniform("u_time", time);

		for (int i = 0; i < Game::instance->scene->lights.size(); ++i)
		{
			EntityLight* light = Game::instance->scene->lights[i];

			if (i != 0)
			{
				glEnable(GL_BLEND);
				shader->setVector3("u_ambient_light", Vector3(0, 0, 0));
			}

			shader->setVector3("u_light_vector", light->model->frontVector());
			shader->setUniform("u_light_intensity", light->intensity);
			shader->setUniform("u_light_position", light->getPosition());
			shader->setVector3("u_light_color", light->color);
			shader->setUniform("u_shadow_bias", 0.001f);
			shader->setUniform("u_light_type", (int)light->light_type);
			shader->setUniform("u_light_maxdist", 15.0f);

			if (light->light_type == DIRECTIONAL)
			{
				Texture* shadowmap = light->shadow_fbo->depth_texture;
				shader->setTexture("shadowmap", shadowmap, 1);
				Matrix44 shadow_proj = light->cam->viewprojection_matrix;
				shader->setUniform("u_shadow_viewproj", shadow_proj);
			}

			//do the draw call
			mesh->render(GL_TRIANGLES);
		}
		//disable shader
		shader->disable();

		glDisable(GL_BLEND);
		glDepthFunc(GL_LESS); //as default

		for (int i = 0; i < children.size(); i++)
			children[i]->render(camera);  //repeat for every child	
	}

	if (object)
		drawText(350, 200, "open door", Vector3(1, 1, 1), 5);
}

void EntityMesh::update(float dt)
{
	bool change_stage = false;
	Vector3 last_pos = model->getTranslation();

	if (this == Game::instance->scene->player)
	{
		if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) model->translate(0.0f, 0.0f, 1.0f * 15 * dt); //move faster with left shift
		if (Input::isKeyPressed(SDL_SCANCODE_W)) model->translate(0.0f, 0.0f, 1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) model->translate(0.0f, 0.0f, -1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_D)) model->rotate(90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		if (Input::isKeyPressed(SDL_SCANCODE_A)) model->rotate(-90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	}
	//else
	//{	updateNPC_IA();
	//}

	Vector3 characterTargetCenter = model->getTranslation() + Vector3(0, 1.5, 0);
	for (int i = 0; i < Game::instance->scene->entities.size(); ++i)
	{
		EntityMesh* current = (EntityMesh*)Game::instance->scene->entities[i];

		if (this == current)
			continue;

		Vector3 coll;
		Vector3 collnorm;

		if (current->name == "PUERTA_BAR" || current->name == "CASA"  || current->name == "CASA_INTERIOR" || current->name == "PUERTA_BAR_INTERIOR" || current->name == "PUERTA" || current->name == "ZEN" )
		{
			if (current->mesh->testRayCollision(*current->model, characterTargetCenter, model->frontVector(), coll, collnorm, 3, false ) == true)
			{
				object = true;
				if (Input::wasKeyPressed(SDL_SCANCODE_F))
				{
					if(current->name == "PUERTA_BAR")
						Game::instance->scene = Scene::Get("data/bar.txt");
					if (current->name == "CASA")
						Game::instance->scene = Scene::Get("data/casa.txt");
					if (current->name == "PUERTA")
						Game::instance->scene = Scene::Get("data/combate.txt");
					if (current->name == "ZEN")
						Game::instance->scene = Scene::Get("data/final.txt");
					
					Vector3 orig_pos = Game::instance->scene->player_pos_orig;
					Vector3 orig_front = Game::instance->scene->player_front_orig;
					Game::instance->scene->player->model->setTranslation(orig_pos.x, orig_pos.y, orig_pos.z);
					Game::instance->scene->player->model->setFrontAndOrthonormalize(orig_front);

					if (current->name == "CASA_INTERIOR" || current->name == "PUERTA_BAR_INTERIOR")
						Game::instance->scene = Scene::Get("data/pueblo.txt");
				}
			}
			else { object = false; }
		}

		if (!current->mesh->testSphereCollision(*current->model, characterTargetCenter, 0.5, coll, collnorm))
			continue;

		Vector3 push_away = normalize(coll - characterTargetCenter) * 10 * dt;

		Vector3 front = model->frontVector();
		model->setTranslation(last_pos.x - push_away.x, 0, last_pos.z - push_away.z);
		model->setFrontAndOrthonormalize(front);
	}
}

EntityLight::EntityLight(std::string name)
{
	this->name = name;
	entity_type = LIGHT;

	cam = new Camera();
}
