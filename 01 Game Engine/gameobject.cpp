#include "gameobject.h"
#include "inputcomponent.h"
#include "soundcomponent.h"
#include "graphicscomponent.h"
#include "physicsmanager.h"

GameObject::GameObject(InputComponent* input, SoundComponent* sound, GraphicsComponent* graphics, const char* name, QVector3D position)
    :  name(name), input_(input), sound_(sound), graphics_(graphics)
{
    graphics->mMatrix.setColumn(3,position.toVector4D());
}

GameObject::~GameObject()
{

}

std::vector<Vertex> GameObject::vertices() const
{
    return graphics_->getVertices();
}

std::vector<GLuint> GameObject::indecies() const
{
    return graphics_->getIndices();
}
