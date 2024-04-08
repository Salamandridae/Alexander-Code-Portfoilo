#ifndef INPUTCOMPONENT_H
#define INPUTCOMPONENT_H

#include <QVector3D>
#include <QMatrix4x4>

#include "gameobject.h"

class GameObject;
class Input;

class Command
{
public:
virtual ~Command() {}
virtual void execute(GameObject &actor)
    {}
};

class RightCommand : public Command
{
public:
    RightCommand();
    ~RightCommand();
    void execute(GameObject &actor) override
    {actor.mPosition.translate(0.2f, 0.f, 0.f);}
};

class LeftCommand : public Command
{
public:
    LeftCommand();
    ~LeftCommand();
    void execute(GameObject &actor) override
    {actor.mPosition.translate(-0.2f, 0.f, 0.f);}
};

class InputComponent
{
public:
    InputComponent();
    Command* handleInput(Input* mInput);
    void update(GameObject &parent, Input* mInput);
};
#endif // INPUTCOMPONENT_H
