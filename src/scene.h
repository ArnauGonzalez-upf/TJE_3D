#pragma once
#include <iostream>
#include "framework.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "camera.h"

enum eLightType {
    SPOT = 1,
    DIRECTIONAL = 2
};

enum eEntityType {
    NONE = 0,
    OBJECT = 1,
    LIGHT = 2,
    CAMERA = 3,
    REFLECTION_PROBE = 4,
    DECALL = 5
};

class Entity
{
public:
    Entity(); //constructor
    //virtual ~Entity(); //destructor

    //some attributes 
    std::string name;
    Matrix44* model;
    eEntityType entity_type;

    //pointer to my parent entity 
    Entity* parent;

    //pointers to my children
    std::vector<Entity*> children;

    //methods overwritten by derived classes 
    virtual void render(Camera* camera) {}
    virtual void update(float elapsed_time) {}

    //methods
    void addChild(Entity* ent);
    void removeChild(Entity* ent);

    Matrix44 getGlobalMatrix(); //returns transform in world coordinates

    //some useful methods...
    Vector3 getPosition() { return *model * Vector3(0, 0, 0); }
};

class EntityLight : public Entity
{
public:
    //Attributes of this class 
    Vector3 color;
    float intensity;
    eLightType light_type;
    float max_distance;

    Camera* cam;
    FBO* shadow_fbo;

    EntityLight();
    //methods overwritten 
    //void render(std::vector<EntityMesh*> entities);
    //void update(float dt);
};

class EntityMesh : public Entity
{
public:
    //Attributes of this class 
    Mesh* mesh;
    Texture* texture;
    Shader* shader;
    Vector4 color;

    EntityMesh();

    //methods overwritten 
    void render(Camera* camera);
    void update(float elapsed_time);
};

class Scene
{
public:
    //static Scene* instance;
    std::vector<Entity*> entities;
    std::vector<EntityLight*> lights;
    Entity* player;

    EntityMesh* fondo;

    Scene();
    void drawSky(Camera* camera);
};
