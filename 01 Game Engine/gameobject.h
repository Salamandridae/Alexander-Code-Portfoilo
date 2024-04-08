#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <QOpenGLFunctions_4_1_Core>
#include <QMatrix4x4>
#include <QVector3D>
#include "vertex.h"
#include "PxPhysicsAPI.h"

class InputComponent;
class GraphicsComponent;
class PhysicsComponent;
class SoundComponent;

class GameObject : public QOpenGLFunctions_4_1_Core
{
public:
    GameObject(InputComponent* input, SoundComponent* sound, GraphicsComponent* graphics, const char* name, QVector3D position);
    GameObject(GLuint ShaderId, GLuint TextureId);
    ~GameObject();
    InputComponent *input() const {return input_;};
    SoundComponent *sound() const {return sound_;};
    GraphicsComponent *graphics() const {return graphics_;};
    PhysicsComponent* physics() const {return physics_;}
    const char* getName(){return name;}
    std::vector<Vertex> vertices() const;
    std::vector<GLuint> indecies() const;
    //Transform data
    QMatrix4x4 mMatrix;
    QMatrix4x4 mPosition;
    QMatrix4x4 mRotation;
    QMatrix4x4 mScale;
    QVector3D mVelocity;
    const char* name = nullptr;

private:
    InputComponent* input_;
    PhysicsComponent* physics_;
    SoundComponent* sound_;
    GraphicsComponent* graphics_;
};

#endif // GAMEOBJECT_H
