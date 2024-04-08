#include "graphicscomponent.h"

GraphicsComponent::GraphicsComponent()
{

}

//Direct vertex data
GraphicsComponent::GraphicsComponent(std::vector<Vertex> V)
{
    mVertices.reserve(V.size());
    mVertices = V;
}

//File reading, needs better condition checking
GraphicsComponent::GraphicsComponent(std::string fileName, GLuint ShaderId, GLuint TextureId)
{
    //fbx and obj
    if(fileName.back() == 'x' || fileName.back() == 'j')
        readMeshFile(fileName);
    //txt
    if(fileName.back() == 't')
        readTextFile(fileName);
}


GraphicsComponent::~GraphicsComponent()
{
    glDeleteVertexArrays( 1, &mVAO );
    glDeleteBuffers( 1, &mVBO );
}

void GraphicsComponent::readMeshFile(std::string fileName)
{
    std::ifstream fileIn;
    fileIn.open(fileName, std::ifstream::in);
    if(!fileIn)
        qDebug() << "Could Not open file for reading: " << QString::fromStdString(fileName);
    std::string oneLine;
    std::string oneWord;
    std::vector<QVector3D> tempVertices;
    std::vector<QVector3D> tempNormals;
    std::vector<QVector2D> tempUVs;

    unsigned int temp_index = 0;
    while(std::getline(fileIn,oneLine))
    {
        std::stringstream sStream;
        sStream << oneLine;
        oneWord = "";
        sStream >>oneWord;
        if(oneWord == "#")
        {
            continue;
        }
        if (oneWord == "")
        {
            continue;
        }
        if(oneWord == "v")
        {
            QVector3D tempVertex;
            sStream >> oneWord;
            tempVertex.setX(std::stof(oneWord));
            sStream >> oneWord;
            tempVertex.setY(std::stof(oneWord));
            sStream >> oneWord;
            tempVertex.setZ(std::stof(oneWord));
            tempVertices.push_back(tempVertex);

            continue;
        }
        if(oneWord == "vt")
        {
            QVector2D tempUV;
            sStream >> oneWord;
            tempUV.setX(std::stof(oneWord));
            sStream >> oneWord;
            tempUV.setY(std::stof(oneWord));
            tempUVs.push_back(tempUV);

            continue;
        }
        if(oneWord == "vn")
        {
            QVector3D tempNormal;
            sStream >> oneWord;
            tempNormal.setX(std::stof(oneWord));
            sStream >> oneWord;
            tempNormal.setY(std::stof(oneWord));
            sStream >> oneWord;
            tempNormal.setZ(std::stof(oneWord));
            tempNormals.push_back(tempNormal);

            continue;
        }
        if(oneWord == "f")
        {
            int index, normal, uv;
            for(int i = 0; i<3; i++)
            {
                sStream >> oneWord;
                std::stringstream tempWord(oneWord);
                std::string segment;
                std::vector<std::string> segmentArray;
                while(std::getline(tempWord, segment, '/'))
                    segmentArray.push_back(segment);
                index =std::stoi(segmentArray[0]);
                if(segmentArray[1] != "")
                    uv = std::stoi(segmentArray[1]);
                else
                    uv = 0;
                normal = std::stoi(segmentArray[2]);
                index--;
                uv--;
                normal--;
                if(uv> -1)
                {
                    Vertex tempVertex(tempVertices[index], tempNormals[normal], tempUVs[uv]);
                    mVertices.push_back(tempVertex);
                }
                else
                {
                    Vertex tempVertex(tempVertices[index], tempNormals[normal], QVector2D{0,0});
                    mVertices.push_back(tempVertex);
                }
                mIndices.push_back(temp_index++);
            }
            continue;
        }

    }
    fileIn.close();
}

void GraphicsComponent::readTextFile(std::string fileName)
{
    std::ifstream inn;
    inn.open(fileName.c_str());
    if (inn.is_open())
    {
        int n;
        Vertex vertex;
        inn >> n;
        mVertices.reserve(n);
        for (int i=0; i<n; i++)
        {
             inn >> vertex;
             mVertices.push_back(vertex);
        }
        inn.close();
    }
}

void GraphicsComponent::init(GLint matrixUniform)
{
    mMatrixUniform = matrixUniform;
    initializeOpenGLFunctions();

    //Vertex Array Object - VAO
    glGenVertexArrays( 1, &mVAO );
    glBindVertexArray( mVAO );

    //Vertex Buffer Object to hold vertices - VBO
    glGenBuffers( 1, &mVBO );
    glBindBuffer( GL_ARRAY_BUFFER, mVBO );

    glBufferData( GL_ARRAY_BUFFER, mVertices.size()*sizeof(Vertex), mVertices.data(), GL_STATIC_DRAW );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT,GL_FALSE,sizeof(Vertex), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex),  (GLvoid*)(3 * sizeof(GLfloat)) );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GraphicsComponent::draw()
{
    glBindVertexArray( mVAO );
    glUniformMatrix4fv( mMatrixUniform, 1, GL_FALSE, mMatrix.constData());
    glDrawArrays(GL_TRIANGLES, 0, mVertices.size());//mVertices.size());
}

void GraphicsComponent::update(QMatrix4x4 model, std::vector<GLint> vMatrixUniform,
                               std::vector<GLint> pMatrixUniform, std::vector<Shader*> shaders,
                               Camera* mCamera, Light* light)
{
    GLint mTextureUniform = glGetUniformLocation(shaders[1]->getProgram(), "textureSampler");
    GLint mLightColorUniform = glGetUniformLocation( shaders[2]->getProgram(), "lightColor" );
    GLint mObjectColorUniform = glGetUniformLocation( shaders[2]->getProgram(), "objectColor" );
    GLint mAmbientLightStrengthUniform = glGetUniformLocation( shaders[2]->getProgram(), "ambientStrength" );
    GLint mLightPositionUniform = glGetUniformLocation( shaders[2]->getProgram(), "lightPosition" );
    GLint mSpecularStrengthUniform = glGetUniformLocation( shaders[2]->getProgram(), "specularStrength" );
    GLint mSpecularExponentUniform = glGetUniformLocation( shaders[2]->getProgram(), "specularExponent" );
    GLint mLightPowerUniform = glGetUniformLocation( shaders[2]->getProgram(), "lightPower" );
    GLint mCameraPositionUniform = glGetUniformLocation( shaders[2]->getProgram(), "cameraPosition" );
    GLint mTextureUniform2 = glGetUniformLocation(shaders[2]->getProgram(), "textureSampler");

    if(getShaderId() == shaders[1]->getProgram())
    {
        glUniformMatrix4fv(vMatrixUniform[1], 2, GL_FALSE, mCamera->mVMatrix.constData());
        glUniformMatrix4fv(pMatrixUniform[1], 2, GL_FALSE, mCamera->mPMatrix.constData());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, getTexId());
    }
    else if(getShaderId() == shaders[2]->getProgram())
    {
        glUseProgram(shaders[2]->getProgram());
        glUniformMatrix4fv(vMatrixUniform[2], 1, GL_FALSE, mCamera->mVMatrix.constData());
        glUniformMatrix4fv(pMatrixUniform[2], 1, GL_FALSE, mCamera->mPMatrix.constData());
        glUniform3f(mLightPositionUniform, light->mMatrix.column(3).x(), light->mMatrix.column(3).y(), light->mMatrix.column(3).z());
        glUniform3f(mCameraPositionUniform, mCamera->position().x(), mCamera->position().y(), mCamera->position().z());
        glUniform3f(mLightColorUniform, light->mLightColor.x(), light->mLightColor.y(), light->mLightColor.z());
        glUniform1f(mSpecularStrengthUniform, light->mSpecularStrength);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, getTexId());
    }
    else
    {
        glUseProgram(shaders[0]->getProgram());
        glUniformMatrix4fv(vMatrixUniform[0], 1, GL_FALSE, mCamera->mVMatrix.constData());
        glUniformMatrix4fv(pMatrixUniform[0], 1, GL_FALSE, mCamera->mPMatrix.constData());
    }
    mMatrix = model;
    draw();
}

const std::vector<Vertex> &GraphicsComponent::getVertices() const
{
    return mVertices;
}

const std::vector<GLuint> &GraphicsComponent::getIndices() const
{
    return mIndices;
}
