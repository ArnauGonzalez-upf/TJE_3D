#pragma once
#include <iostream>
#include "includes.h"
#include "utils.h"
#include "framework.h"
#include <map>
#include "includes.h"
#include "camera.h"
#include "utils.h"
#include "scene.h"
#include "stage.h"

class Stage {
public:
	//Definimos un map para el manager de stages
	static std::map<int, Stage*> Stage::stages;

	Stage(int pos);

	//Render y Update son metodos virtuales para que los reescriban las clases que heredan de esta
	virtual void render() {}
	virtual Stage* update() { return getStage(0); }

	static Stage* getStage(int pos);
};

class EditorStage : public Stage {
public:
	EditorStage();

	//virtual Stage* update();
	void AddObjectInFront(Camera* camera, Scene* scene);
};