#ifndef GRAPHICSCOMPONENT_H
#define GRAPHICSCOMPONENT_H

#include <QOpenGLFunctions_4_1_Core>
#include <QMatrix4x4>
#include <vector>
#include <sstream>
#include "vertex.h"
#include "shader.h"
#include "camera.h"
#include "light.h"

class GraphicsComponent : QOpenGLFunctions_4_1_Core
{
public:
    GraphicsComponent();
    GraphicsComponent(std::vector<Vertex> V);
    GraphicsComponent(std::string fileName, GLuint ShaderId, GLuint TextureId);
    ~GraphicsComponent();
    void init(GLint matrixUniform);
    void draw();
    QMatrix4x4 mMatrix;
    virtual GLuint getShaderId(){return mShaderId;}
    virtual GLuint getTexId(){return mTextureId;}
    void update(QMatrix4x4 model, std::vector<GLint> vMatrixUniform, std::vector<GLint> pMatrixUniform, std::vector<Shader*> shaders, Camera* mCamera, Light* light);

    const std::vector<Vertex> &getVertices() const;
    const std::vector<GLuint> &getIndices() const;

protected:
    std::vector<Vertex> mVertices;
    std::vector<GLuint> mIndices;
    std::vector<Vertex::Triangle> mTriangles;

    GLuint mVAO{0};
    GLuint mVBO{0};
    GLuint mIBO{0};

    GLint mMatrixUniform{0};
    GLuint mShaderId;
    GLuint mTextureId;
private:
    void readMeshFile(std::string fileName);
    void readTextFile(std::string fileName);
};

#endif // GRAPHICSCOMPONENT_H
