#pragma once
#include <iostream>
#include "framework.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "camera.h"
#include "extra\textparser.h"

enum eReadingType {
    ENT_OBJ = 1,
    LIGHT_OBJ = 2
};

enum eLightType : uint8 {
    POINT_LIGHT = 1,
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

enum eObjectType {
    STATIC,
    DYNAMIC
};

enum eCameraMode {
    STATIC_CAM,
    FOLLOWING,
    FOLLOWING_LATERAL,
    CENTERED
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

    EntityLight(std::string name);
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
    eObjectType type;

    bool object;

    EntityMesh(std::string name);

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

    std::vector<Entity*> static_entities;
    std::vector<Entity*> dynamic_entities;

    Entity* player;
    Vector3 player_pos_orig;
    Vector3 player_front_orig;

    EntityMesh* fondo;

    Camera* scene_camera;
    eCameraMode camera_mode;

    Scene(const char* filename);
    void drawSky(Camera* camera);
    void exportEscene();

    //manager of images
    static std::map<const char*, Scene*> s_loaded_scenes;
    static Scene* Get(const char* filename);
    void registerAs(const char* filename);
};
