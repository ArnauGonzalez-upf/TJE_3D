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

Scene::Scene()
{
	//instance = this;

	/*fondo = new EntityMesh("fondo");
	fondo->texture->load("data/cielo/cielo.tga");
	// example of loading Mesh from Mesh Manager
	fondo->mesh = Mesh::Get("data/cielo/cielo.ASE");
	// example of shader loading using the shaders manager
	fondo->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	//fondo->model->setScale(20, 20, 20);
	//entities.push_back(fondo);

	fondo->texture->load("data/cielo/cielo.tga");
	// example of loading Mesh from Mesh Manager
	fondo->mesh = Mesh::Get("data/cielo/cielo.ASE");
	 example of shader loading using the shaders manager
	fondo->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");

	EntityMesh* plano = new EntityMesh("plano");
	plano->texture->load("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
	// example of loading Mesh from Mesh Manager
	plano->mesh = Mesh::Get("data/plano.obj");
	// example of shader loading using the shaders manager
	plano->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");

	EntityMesh* muros = new EntityMesh("muros");
	muros->texture->load("data/biglib/GiantGeneralPack/color-atlas-new.png");
	// example of loading Mesh from Mesh Manager
	muros->mesh = Mesh::Get("data/muros.obj");
	// example of shader loading using the shaders manager
	muros->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");

	EntityMesh* bro = new EntityMesh("bro");
	bro->texture->load("data/biglib/GiantGeneralPack/color-atlas-new.png");
	// example of loading Mesh from Mesh Manager
	bro->mesh = Mesh::Get("data/biglib/SamuraiPack/Characters/Character_Samurai_Warrior_Black_5.obj");
	// example of shader loading using the shaders manager
	bro->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/shadows_fragment.fs");
	bro->model->translate(50,0,0);
	//bro->model->scale(0.8, 0.8, 0.8);
	//bro->model->rotate(90.0f * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));*/

	//entities.push_back(bro);
	//player = bro;

	EntityLight* sun = new EntityLight("sol");
	lights.push_back(sun);

	TextParser* parser = new TextParser("data/pueblo.txt");

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
		if (type == "POSITION")
		{
			EntityMesh* ent = (EntityMesh*)entities.back();
			int x = parser->getfloat();
			int y = parser->getfloat();
			int z = parser->getfloat();
			ent->model->setTranslation(x, y, z);
		}
	}

	std::cout << entities.size() << std::endl;

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
	myfile.open("data/pueblo.txt");
	for (int i = 0; i < entities.size(); ++i)
	{
		EntityMesh* ent = (EntityMesh*)entities[i];

		myfile << "NAME " + ent->name + "\n";
		myfile << "MESH " + ent->mesh->name + "\n";
		myfile << "TEXTURE " + ent->texture->filename + "\n";
		myfile << "SHADER " + ent->shader->ps_filename + "\n";
		
		Vector3 pos = ent->model->getTranslation();
		if (i == entities.size() - 1) { myfile << "POSITION " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z); }
		else { myfile << "POSITION " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z) + "\n"; }

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
}

void EntityMesh::update(float dt)
{   
	if (this == Game::instance->scene->player)
	{
		//if (Input::isKeyPressed(SDL_SCANCODE_W)) model->translate(0.0f, 0.0f, -1.0f * 10 * dt);
		
		//if (Input::isKeyPressed(SDL_SCANCODE_D)) model->rotate(90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		//if (Input::isKeyPressed(SDL_SCANCODE_A)) model->rotate(-90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

		//model->translate(0.0f, 0.0f, -1.0f * 20 * dt);

		/*Vector3 last_pos = model->getTranslation();

		Vector3 col_point;     //temp var para guardar el punto de colision si lo hay
		Vector3 col_normal;     //temp var para guardar la normal al punto de colision
		float max_ray_dist = 20;*/

		if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) model->translate(0.0f, 0.0f, 1.0f * 15 * dt); //move faster with left shift
		if (Input::isKeyPressed(SDL_SCANCODE_W)) model->translate(0.0f, 0.0f, 1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) model->translate(0.0f, 0.0f, -1.0f * 10 * dt);
		if (Input::isKeyPressed(SDL_SCANCODE_D)) model->rotate(90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		if (Input::isKeyPressed(SDL_SCANCODE_A)) model->rotate(-90.0f * dt * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

		/*for (int i = 0; i < Game::instance->scene->entities.size(); ++i) {
			Matrix44 m = *(Game::instance->scene->entities[i]->model);
			EntityMesh* ent = (EntityMesh*)Game::instance->scene->entities[i];
			Mesh* mesh = ent->mesh;
			//max_ray_dist = model->getTranslation().distance(last_pos);

			if (Game::instance->scene->entities[i] != Game::instance->scene->player) {
				//la funcion testRayCollision construye el CollisionModel3D si no ha sido creado antes y luego testea el rayo
				if (mesh->testRayCollision(m,     //the model of the entity to know where it is
					last_pos,        //the origin of the ray we want to test
					model->frontVector(),        //the dir vector of the ray
					col_point,        //here we will h
					col_normal,        //a temp var to store the collision normal
					max_ray_dist,    //max ray distance to test
					false            //false if we want the col_point in world space (true if in object)
				) == true)
				{
					model->translate(0.0f, 0.0f, -1.0f * 10 * dt);
				}
			}
		}*/
	}
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
	cam->setOrthographic(-100, 100, -100, 100, 0.1, 500);

	light_type = DIRECTIONAL;

	shadow_fbo = new FBO();
	shadow_fbo->setDepthOnly(8192, 8192);

	intensity = 1.0;
	color = Vector3(1, 1, 1);
}
