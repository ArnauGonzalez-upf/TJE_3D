#include "scene.h"
#include "game.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"

Scene::Scene()
{
	//instance = this;

	/*EntityMesh* nave = new EntityMesh();
	nave->texture->load("data/terrain/tree.tga");
	// example of loading Mesh from Mesh Manager
	nave->mesh = Mesh::Get("data/terrain/terrain_trees.ASE");
	// example of shader loading using the shaders manager
	nave->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");

	EntityMesh* terrain = new EntityMesh();
	terrain->texture->load("data/terrain2/terrain.tga");
	// example of loading Mesh from Mesh Manager
	terrain->mesh = Mesh::Get("data/terrain2/terrain.ASE");
	// example of shader loading using the shaders manager
	terrain->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");

	entities.push_back(nave);
	entities.push_back(terrain);

	EntityMesh* bro = new EntityMesh();
	bro->texture->load("data/nave/x3_fighter.tga");
	// example of loading Mesh from Mesh Manager
	bro->mesh = Mesh::Get("data/nave/x3_fighter.ASE");
	// example of shader loading using the shaders manager
	bro->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	//bro->model->setScale(0.2, 0.2, 0.2);
	entities.push_back(bro);
	player = bro;

	fondo = new EntityMesh();
	fondo->texture->load("data/space/space_cubemap.tga");
	// example of loading Mesh from Mesh Manager
	fondo->mesh = Mesh::Get("data/space/space_cubemap.ASE");
	// example of shader loading using the shaders manager
	fondo->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	fondo->model->setScale(20, 20, 20);
	entities.push_back(fondo);*/

	EntityMesh* house2 = new EntityMesh();
	house2->texture->load("data/biglib/SamuraiPack/PolygonSamurai_Tex_01.png");
	// example of loading Mesh from Mesh Manager
	house2->mesh = Mesh::Get("data/casa.obj");
	// example of shader loading using the shaders manager
	house2->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");

	EntityMesh* bro = new EntityMesh();
	bro->texture->load("data/biglib/MiniCharacters/PolygonMinis_Texture_01_A.png");
	// example of loading Mesh from Mesh Manager
	bro->mesh = Mesh::Get("data/biglib/MiniCharacters/Chr_Samurai_SamuraiWarrior_01_48.obj");
	// example of shader loading using the shaders manager
	bro->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	bro->model->translate(50,0,0);

	//bro->model->rotate(90.0f * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));

	entities.push_back(bro);
	player = bro;

	entities.push_back(house2);
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

Entity::Entity()
{
	name = "a";
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

EntityMesh::EntityMesh()
{
	mesh = NULL;
	texture = new Texture();
	shader = NULL;
	entity_type = OBJECT;
}

void EntityMesh::render(Camera* camera)
{
	Matrix44 model = getGlobalMatrix();

	//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
	BoundingBox world_bounding = transformBoundingBox(model, mesh->box);

	//if bounding box is inside the camera frustum then the object is probably visible
	if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize))
	{
		//enable shader
		shader->enable();

		//upload uniforms
		shader->setUniform("u_color", Vector4(1, 1, 1, 1));
		shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
		shader->setUniform("u_texture", texture, 0);
		shader->setUniform("u_model", model);
		shader->setUniform("u_time", time);

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

EntityLight::EntityLight()
{
	model->translate(0,1000,0);

	Vector3 pos = *model * Vector3(0, 0, 0);
	Vector3 target = Vector3(100,0,0.01);
	model->setFrontAndOrthonormalize(pos - target);

	cam = new Camera();
	cam->lookAt(pos, pos + model->rotateVector(Vector3(0, 0, -1)), Vector3(0, 1, 0));
	cam->setOrthographic(-1000, 1000, -1000, 1000, 1, 5000);

	color = Vector3(1, 1, 1);
	light_type = DIRECTIONAL;

	shadow_fbo = new FBO();
	shadow_fbo->setDepthOnly(1024, 1024);

	intensity = 100.0;
	color = Vector3(1, 1, 1);
}
