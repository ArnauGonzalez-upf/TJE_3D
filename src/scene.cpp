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

Scene::Scene(const char* filename)
{
	EntityLight* sun = new EntityLight("sol");
	lights.push_back(sun);

	TextParser* parser = new TextParser(filename);

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
	}
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
	myfile.open("data/combate.txt");
	for (int i = 0; i < entities.size(); ++i)
	{
		EntityMesh* ent = (EntityMesh*)entities[i];

		myfile << "NAME " + ent->name + "\n";
		myfile << "MESH " + ent->mesh->name + "\n";
		myfile << "TEXTURE " + ent->texture->filename + "\n";
		myfile << "SHADER " + ent->shader->ps_filename + "\n";

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
		EntityLight* light = Game::instance->scene->lights[0];

		//enable shader
		shader->enable();

		shader->setVector3("u_ambient_light", Vector3(0.3, 0.3, 0.3));


		//upload uniforms
		shader->setUniform("u_color", Vector4(1, 1, 1, 1));
		shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
		shader->setUniform("u_texture", texture, 0);
		shader->setUniform("u_model", model);
		shader->setUniform("u_time", time);

		shader->setVector3("u_light_vector", light->model->frontVector());
		shader->setUniform("u_light_intensity", light->intensity);
		shader->setVector3("u_light_color", light->color);
		shader->setUniform("u_shadow_bias", 0.001f);

		Texture* shadowmap = light->shadow_fbo->depth_texture;
		shader->setTexture("shadowmap", shadowmap, 1);
		Matrix44 shadow_proj = light->cam->viewprojection_matrix;
		shader->setUniform("u_shadow_viewproj", shadow_proj);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();

		for (int i = 0; i < children.size(); i++)
			children[i]->render(camera);  //repeat for every child	
	}

	if (object)
		drawText(350, 200, "open door", Vector3(1, 1, 1), 5);
}

void EntityMesh::update(float dt)
{   
	bool change_stage = false;

	if (this == Game::instance->scene->player)
	{
		Vector3 last_pos = model->getTranslation();

		if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) model->translate(0.0f, 0.0f, 1.0f * 15 * dt); //move faster with left shift
		if (Input::isKeyPressed(SDL_SCANCODE_W)) model->translate(0.0f, 0.0f, 1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) model->translate(0.0f, 0.0f, -1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_D)) model->rotate(90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		if (Input::isKeyPressed(SDL_SCANCODE_A)) model->rotate(-90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

		Vector3 characterTargetCenter = model->getTranslation() + Vector3(0, 1.5, 0);
		for (int i = 0; i < Game::instance->scene->entities.size() - 1; ++i)
		{
			EntityMesh* current = (EntityMesh*)Game::instance->scene->entities[i];

			Vector3 coll;
			Vector3 collnorm;

			if (current->name == "PUERTA_BAR")
			{
				if (current->mesh->testRayCollision(*current->model,     //the model of the entity to know where it is
					characterTargetCenter,        //the origin of the ray we want to test
					model->frontVector(),        //the dir vector of the ray
					coll,        //here we will h
					collnorm,        //a temp var to store the collision normal
					5,    //max ray distance to test
					false            //false if we want the col_point in world space (true if in object)
				) == true)
				{
					object = true;
					if (Input::wasKeyPressed(SDL_SCANCODE_F))
						change_stage = true;
				}
				else { object = false; }
			}

			/*if (current->name == "PUERTA_BAR" && current->mesh->testSphereCollision(*current->model, characterTargetCenter, 3, coll, collnorm))
			{
				object = true;
			}*/

			if (!current->mesh->testSphereCollision(*current->model, characterTargetCenter, 0.5, coll, collnorm))
				continue;

			Vector3 push_away = normalize(coll - characterTargetCenter) * 10 * dt;

			Vector3 front = model->frontVector();
			model->setTranslation(last_pos.x - push_away.x, 0, last_pos.z - push_away.z);
			model->setFrontAndOrthonormalize(front);
		}
	}

	if (change_stage)
		Game::instance->scene = new Scene("data/combate.txt");
}

EntityLight::EntityLight(std::string name)
{
	this->name = name;
	model->translate(100, 100, 100);

	entity_type = LIGHT;

	Vector3 target = Vector3(0,0,60);
	Vector3 pos = model->getTranslation();
	model->setFrontAndOrthonormalize(target - pos);

	cam = new Camera();
	cam->lookAt(model->getTranslation(), *model * Vector3(0, 0, 1), model->rotateVector(Vector3(0, 1, 0)));
	cam->setOrthographic(-50, 50, -50, 50, 0.1, 500);

	light_type = DIRECTIONAL;

	shadow_fbo = new FBO();
	shadow_fbo->setDepthOnly(8192, 8192);

	intensity = 1.0;
	color = Vector3(1, 1, 1);
}
