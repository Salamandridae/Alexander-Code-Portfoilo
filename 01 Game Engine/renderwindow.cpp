#include "renderwindow.h"
#include <QTimer>
#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QKeyEvent>
#include <QStatusBar>
#include <QDebug>

#include <string>
#include <unordered_map>

#include "visualobject.h"
#include "camera.h"
#include "shader.h"
#include "mainwindow.h"
#include "logger.h"
#include "texture.h"
#include "light.h"
#include "trianglesurface.h"
#include "objectmesh.h"
#include "soundcomponent.h"
#include "gameobject.h"
#include "graphicscomponent.h"
#include "inputcomponent.h"
#include "dialoguecontroller.h"

RenderWindow::RenderWindow(const QSurfaceFormat &format, MainWindow *mainWindow)
    : mContext(nullptr), mInitialized(false), mMainWindow(mainWindow)
{
    //This is sent to QWindow:
    setSurfaceType(QWindow::OpenGLSurface);
    setFormat(format);
    //Make the OpenGL context
    mContext = new QOpenGLContext(this);
    //Give the context the wanted OpenGL format (v4.1 Core)
    mContext->setFormat(requestedFormat());
    if (!mContext->create()) {
        delete mContext;
        mContext = nullptr;
        qDebug() << "Context could not be made - quitting this application";
    }

    //Make the gameloop timer:
    mRenderTimer = new QTimer(this);

    //Start doing Lua stuff
    luaL_openlibs(L);
}

RenderWindow::~RenderWindow()
{
    //cleans up the GPU memory
    glDeleteVertexArrays( 1, &mVAO );
    glDeleteBuffers( 1, &mVBO );
    //Stop doing Lua stuff
    lua_close(L);
}

// Sets up the general OpenGL stuff and the buffers needed to render a Cube
void RenderWindow::init()
{
    //Get the instance of the utility Output logger
    //Have to do this, else program will crash (or you have to put in nullptr tests...)
    mLogger = Logger::getInstance();

    //Connect the gameloop timer to the render function:
    //This makes our render loop
    connect(mRenderTimer, SIGNAL(timeout()), this, SLOT(render()));
    //********************** General OpenGL stuff **********************

    //The OpenGL context has to be set.
    //The context belongs to the instanse of this class!
    if (!mContext->makeCurrent(this)) {
        mLogger->logText("makeCurrent() failed", LogType::REALERROR);
        return;
    }

    //just to make sure we don't init several times
    //used in exposeEvent()
    if (!mInitialized)
        mInitialized = true;

    //must call this to use OpenGL functions
    initializeOpenGLFunctions();
        Phys.initPhysics();
    //Print render version info (what GPU is used):
    //Nice to see if you use the Intel GPU or the dedicated GPU on your laptop
    // - can be deleted
    mLogger->logText("The active GPU and API:", LogType::HIGHLIGHT);
    std::string tempString;
    tempString += std::string("  Vendor: ") + std::string((char*)glGetString(GL_VENDOR)) + "\n" +
            std::string("  Renderer: ") + std::string((char*)glGetString(GL_RENDERER)) + "\n" +
            std::string("  Version: ") + std::string((char*)glGetString(GL_VERSION));

    //Print info about opengl texture limits on this GPU:
    int textureUnits;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &textureUnits);
    tempString += std::string("  This GPU as ") + std::to_string(textureUnits) + std::string(" texture units / slots in total, \n");

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);
    tempString += std::string("  and supports ") + std::to_string(textureUnits) + std::string(" texture units pr shader");

    mLogger->logText(tempString);

    //Start the Qt OpenGL debugger
    //Really helpfull when doing OpenGL
    //Supported on most Windows machines - at least with NVidia GPUs
    //reverts to plain glGetError() on Mac and other unsupported PCs
    // - can be deleted
    startOpenGLDebugger();

    //general OpenGL stuff:
    glEnable(GL_DEPTH_TEST);            //enables depth sorting - must then use GL_DEPTH_BUFFER_BIT in glClear
    glEnable(GL_CULL_FACE);       //draws only front side of models - usually what you want - test it out!
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);    //gray color used in glClear GL_COLOR_BUFFER_BIT

    //set up alpha blending for textures
    glEnable(GL_BLEND);// you enable blending function
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Compile shaders:
    //NB: hardcoded path to files! You have to change this if you change directories for the project.
    //Qt makes a build-folder besides the project folder. That is why we go down one directory
    // (out of the build-folder) and then up into the project folder.
    mShaders.push_back(new Shader("../GEA2022/plainshader.vert", "../GEA2022/plainshader.frag"));
    mLogger->logText("Plain shader program id: " + std::to_string(mShaders.back()->getProgram()) );
    mShaders.push_back(new Shader("../GEA2022/textureshader.vert", "../GEA2022/textureshader.frag"));
    mLogger->logText("Texture shader program id: " + std::to_string(mShaders.back()->getProgram()) );
    mShaders.push_back( new Shader("../GEA2022/phongshader.vert", "../GEA2022/phongshader.frag"));
    mLogger->logText("Texture shader program id: " + std::to_string(mShaders.back()->getProgram()) );

    for(unsigned int i = 0; i < mShaders.size(); i++)
    setupShader(i);

    //********************** Texture stuff: **********************
    //Returns a pointer to the Texture class. This reads and sets up the texture for OpenGL
    //and returns the Texture ID that OpenGL uses from Texture::id()
    mTextures.push_back(new Texture);
    mTextures.push_back(new Texture("../GEA2022/assets/grass.bmp"));

    //Set the textures loaded to a texture unit (also called a texture slot)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextures[0]->id());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextures[1]->id());

    mMMatrix = new QMatrix4x4{};
    mMMatrix->setToIdentity();    //1, 1, 1, 1 in the diagonal of the matrix

    mCamera = new Camera();
    mSound = new SoundComponent("../GEA2022/Assets/laser.wav", {pos.x(), pos.y(), pos.z()}, {0,0,0});

    VisualObject* temp;

    if(CheckLua(L, luaL_dofile(L, "../GEA2022/init.lua")))
    {
        lua_getglobal(L, "GetObject");
        if (lua_isfunction(L, -1))
        {
            lua_pushnumber(L, 1);

            if (CheckLua(L, lua_pcall(L, 1, 1, 0)))
            {
                lua_pushstring(L, "FilePath");
                lua_gettable(L, -2);
                qDebug() << "[Lua] has found " << lua_tostring(L, -1) << "/n";
                temp = new ObjectMesh(lua_tostring(L, -1), mShaders[0]->getProgram(), mTextures[0]->id());
                lua_pop(L, 1);
                //need to delete something here? memory leak from "new"
            }

        }
    }

    surface = new TriangleSurface("../GEA2022/assets/terrain.txt",
                                  mShaders[2]->getProgram(), mTextures[1]->id(),Phys.getPhysics(),Phys.getScene(),Phys.getCooking(),"Terrain");
    surface->init(mMMatrixUniform[2]);

    //Creating a Game Object
            GameObject* testObject = new GameObject(new InputComponent(),
                    new SoundComponent("../GEA2022/Assets/laser.wav", {pos.x(), pos.y(), pos.z()}, {0,0,0}),
                    new GraphicsComponent("../GEA2022/assets/test.obj", mShaders[0]->getProgram(), mTextures[0]->id()),
                    "test",
                    QVector3D(0,0,10));
    testObject->mMatrix.setColumn(3,QVector3D(0,0,10).toVector4D());
    Phys.createDynamic(testObject,testObject->name,PxTransform(PxVec3(testObject->graphics()->mMatrix.column(3).x(),testObject->graphics()->mMatrix.column(3).y(),testObject->graphics()->mMatrix.column(3).z())),
                       Phys.getPhysics(),Phys.getCooking(),Phys.getScene());

    //Phys.helloWorldSnippets();

    mGameObjects.insert(std::pair("testObject", testObject));
    mLight = new Light(mShaders[0]->getProgram(), mTextures[0]->id());
    mLight->setName("light");
    mLight->mMatrix.translate(1.f, 1.f, 1.f);
    mObjects.push_back(mLight);

    aspectratio = static_cast<float>(width()) / height();
    mCamera->init();
    mCamera->perspective(60.f, aspectratio, 0.1f, 400.f);


    for(auto it : mGameObjects)
    {
        if (it.second->graphics()->getShaderId()==mShaders[1]->getProgram())
        {
            it.second->graphics()->init(mMMatrixUniform[1]);
        }
        else if(it.second->graphics()->getShaderId()==mShaders[2]->getProgram())
        {
            it.second->graphics()->init(mMMatrixUniform[2]);
        }
        else
        {
            it.second->graphics()->init(mMMatrixUniform[0]);
        }
    }

    glBindVertexArray(0);       //unbinds any VertexArray - good practice
    TestDia = DialogueController::getInstance();
}

void RenderWindow::setupShader(int index)
{
    {
        mMMatrixUniform.push_back(glGetUniformLocation(mShaders[index]->getProgram(), "mMatrix"));
        mVMatrixUniform.push_back(glGetUniformLocation(mShaders[index]->getProgram(), "vMatrix"));
        mPMatrixUniform.push_back(glGetUniformLocation(mShaders[index]->getProgram(), "pMatrix"));
        if(index == 1)
            mTextureUniform = glGetUniformLocation(mShaders[index]->getProgram(), "textureSampler");
        if(index == 2)
            {
                mLightColorUniform = glGetUniformLocation( mShaders[index]->getProgram(), "lightColor" );
                mObjectColorUniform = glGetUniformLocation( mShaders[index]->getProgram(), "objectColor" );
                mAmbientLightStrengthUniform = glGetUniformLocation( mShaders[index]->getProgram(), "ambientStrength" );
                mLightPositionUniform = glGetUniformLocation( mShaders[index]->getProgram(), "lightPosition" );
                mSpecularStrengthUniform = glGetUniformLocation( mShaders[index]->getProgram(), "specularStrength" );
                mSpecularExponentUniform = glGetUniformLocation( mShaders[index]->getProgram(), "specularExponent" );
                mLightPowerUniform = glGetUniformLocation( mShaders[index]->getProgram(), "lightPower" );
                mCameraPositionUniform = glGetUniformLocation( mShaders[index]->getProgram(), "cameraPosition" );
                mTextureUniform2 = glGetUniformLocation(mShaders[index]->getProgram(), "textureSampler");
            }
    }
}

// Called each frame - doing the rendering!!!
void RenderWindow::render()
{
    mTimeStart.restart(); //restart FPS clock
    mContext->makeCurrent(this); //must be called every frame (every time mContext->swapBuffers is called)

    initializeOpenGLFunctions();    //must call this every frame it seems...

    //clear the screen for each redraw
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Phys.simulationStep(dt);

    moveCamera(); //Move to Input component?



    for(auto it : mGameObjects){
        if (it.second->input() != nullptr)
        {
            it.second->input()->update(*it.second, &mInput);
        }
        it.second->mMatrix = it.second->mPosition;
         Phys.update(it.second);
        //it.second->sound.update();
        it.second->graphics()->update(it.second->mMatrix, mVMatrixUniform, mPMatrixUniform, mShaders, mCamera, mLight);
    }
    if (surface)
    {
        if (bShader)
        {
            glUseProgram(mShaders[2]->getProgram());
            glUniformMatrix4fv(mVMatrixUniform[2], 1, GL_FALSE, mCamera->mVMatrix.constData());
            glUniformMatrix4fv(mPMatrixUniform[2], 1, GL_FALSE, mCamera->mPMatrix.constData());
            glUniform3f(mLightPositionUniform, mLight->mMatrix.column(3).x(), mLight->mMatrix.column(3).y(), mLight->mMatrix.column(3).z());
            glUniform3f(mCameraPositionUniform, mCamera->position().x(), mCamera->position().y(), mCamera->position().z());
            glUniform3f(mLightColorUniform, mLight->mLightColor.x(), mLight->mLightColor.y(), mLight->mLightColor.z());
            glUniform1f(mSpecularStrengthUniform, mLight->mSpecularStrength);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, (surface->getTexId()));
        }
        else
        {
            glUseProgram(mShaders[0]->getProgram());
            glUniformMatrix4fv(mVMatrixUniform[0], 1, GL_FALSE, mCamera->mVMatrix.constData());
            glUniformMatrix4fv(mPMatrixUniform[0], 1, GL_FALSE, mCamera->mPMatrix.constData());
        }
        surface->draw();
    }
    static float rotate{0.f};
    mLight->mMatrix.translate(sinf(rotate)/10, cosf(rotate)/10, cosf(rotate)/60);//Move to Input component
    rotate += 0.01f;

    //Calculate framerate before
    // checkForGLerrors() because that call takes a long time
    // and before swapBuffers(), else it will show the vsync time
    calculateFramerate();

    //using our expanded OpenGL debugger to check if everything is OK.
    checkForGLerrors();

    //Qt require us to call this swapBuffers() -function.
    // swapInterval is 1 by default which means that swapBuffers() will (hopefully) block
    // and wait for vsync.
    mContext->swapBuffers(this);
}

//This function is called from Qt when window is exposed (shown)
// and when it is resized
//exposeEvent is a overridden function from QWindow that we inherit from
void RenderWindow::exposeEvent(QExposeEvent *)
{
    //if not already initialized - run init() function - happens on program start up
    if (!mInitialized)
        init();

    //This is just to support modern screens with "double" pixels (Macs and some 4k Windows laptops)
    const qreal retinaScale = devicePixelRatio();

    //Set viewport width and height to the size of the QWindow we have set up for OpenGL
    glViewport(0, 0, static_cast<GLint>(width() * retinaScale), static_cast<GLint>(height() * retinaScale));

    //If the window actually is exposed to the screen we start the main loop
    //isExposed() is a function in QWindow
    if (isExposed())
    {
        //This timer runs the actual MainLoop
        //16 means 16ms = 60 Frames pr second (should be 16.6666666 to be exact...)
        mRenderTimer->start(16);
        mTimeStart.start();
    }
}

//The way this function is set up is that we start the clock before doing the draw call,
// and check the time right after it is finished (done in the render function)
//This will approximate what framerate we COULD have.
//The actual frame rate on your monitor is limited by the vsync and is probably 60Hz
void RenderWindow::calculateFramerate()
{
    long nsecElapsed = mTimeStart.nsecsElapsed();
    static int frameCount{0};                       //counting actual frames for a quick "timer" for the statusbar

    if (mMainWindow)            //if no mainWindow, something is really wrong...
    {
        ++frameCount;
        if (frameCount > 30)    //once pr 30 frames = update the message == twice pr second (on a 60Hz monitor)
        {
            //showing some statistics in status bar
            mMainWindow->statusBar()->showMessage(" Time pr FrameDraw: " +
                                                  QString::number(nsecElapsed/1000000.f, 'g', 4) + " ms  |  " +
                                                  "FPS (approximated): " + QString::number(1E9 / nsecElapsed, 'g', 7));
            frameCount = 0;     //reset to show a new message in 30 frames
        }
    }
}

//Uses QOpenGLDebugLogger if this is present
//Reverts to glGetError() if not
void RenderWindow::checkForGLerrors()
{
    if(mOpenGLDebugLogger)  //if our machine got this class to work
    {
        const QList<QOpenGLDebugMessage> messages = mOpenGLDebugLogger->loggedMessages();
        for (const QOpenGLDebugMessage &message : messages)
        {
            if (!(message.type() == message.OtherType)) // get rid of uninteresting "object ...
                                                        // will use VIDEO memory as the source for
                                                        // buffer object operations"
                // valid error message:
                mLogger->logText(message.message().toStdString(), LogType::REALERROR);
        }
    }
    else
    {
        GLenum err = GL_NO_ERROR;
        while((err = glGetError()) != GL_NO_ERROR)
        {
            mLogger->logText("glGetError returns " + std::to_string(err), LogType::REALERROR);
            switch (err) {
            case 1280:
                mLogger->logText("GL_INVALID_ENUM - Given when an enumeration parameter is not a "
                                "legal enumeration for that function.");
                break;
            case 1281:
                mLogger->logText("GL_INVALID_VALUE - Given when a value parameter is not a legal "
                                "value for that function.");
                break;
            case 1282:
                mLogger->logText("GL_INVALID_OPERATION - Given when the set of state for a command "
                                "is not legal for the parameters given to that command. "
                                "It is also given for commands where combinations of parameters "
                                "define what the legal parameters are.");
                break;
            }
        }
    }
}

//Tries to start the extended OpenGL debugger that comes with Qt
//Usually works on Windows machines, but not on Mac...
void RenderWindow::startOpenGLDebugger()
{
    QOpenGLContext * temp = this->context();
    if (temp)
    {
        QSurfaceFormat format = temp->format();
        if (! format.testOption(QSurfaceFormat::DebugContext))
            mLogger->logText("This system can not use QOpenGLDebugLogger, so we revert to glGetError()",
                             LogType::HIGHLIGHT);

        if(temp->hasExtension(QByteArrayLiteral("GL_KHR_debug")))
        {
            mLogger->logText("This system can log extended OpenGL errors", LogType::HIGHLIGHT);
            mOpenGLDebugLogger = new QOpenGLDebugLogger(this);
            if (mOpenGLDebugLogger->initialize()) // initializes in the current context
                mLogger->logText("Started Qt OpenGL debug logger");
        }
    }
}

bool RenderWindow::CheckLua(lua_State *L, int r)
{
    if (r != LUA_OK)
    {
        //std::string errormsg = lua_tostring(L, -1);
        qDebug() << lua_tostring(L, -1);
        return false;
    }
    return true;
}

void RenderWindow::shaderToggle()
{
    bShader = !bShader;
    if(bShader)
    {
        surface->shaderToggle(mShaders[2]->getProgram());
        surface->init(mMMatrixUniform[2]);
    }
    else
    {
        surface->shaderToggle(mShaders[0]->getProgram());
        surface->init(mMMatrixUniform[0]);
    }
}

void RenderWindow::moveCamera()
{
    float d = 0.2f;
    if(!bCameraLock)
    {
        if(mInput.W == true)
        {
            mCamera->translate(0.f, d, 0.f);
            pos.setY(pos.y() + d);
        }
        if(mInput.A == true)
        {
            mCamera->translate(-d, 0.f, 0.f);
            pos.setX(pos.x() - d);
        }
        if(mInput.S == true)
        {
            mCamera->translate(0.f, -d, 0.f);
            pos.setY(pos.y() - d);
        }
        if(mInput.D == true)
        {
            mCamera->translate(d, 0.f, 0.f);
            pos.setX(pos.x() + d);
        }

        if(mInput.Q == true)
        {
            mCamera->translate(0.f, 0.f, -d*2);
            pos.setZ(pos.z() - d*2);
        }
        if(mInput.E == true)
        {
            mCamera->translate(0.f, 0.f, d*2);
            pos.setZ(pos.z() + d*2);
        }
    mSound->setListener(pos.x(),pos.y(),pos.z());
    }
}

void RenderWindow::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Escape)
    {
        mMainWindow->close();       //Shuts down the whole program
    }
    //You get the keyboard input like this
    if(event->key() == Qt::Key_W || event->key() == Qt::Key_Up)
    {
        mInput.W = true;
    }
    if(event->key() == Qt::Key_A || event->key() == Qt::Key_Left)
    {
        mInput.A = true;
    }
    if(event->key() == Qt::Key_S || event->key() == Qt::Key_Down)
    {
        mInput.S = true;
    }
    if(event->key() == Qt::Key_D || event->key() == Qt::Key_Right)
    {
        mInput.D = true;
    }
    if(event->key() == Qt::Key_Q)
    {
        mInput.Q = true;
    }
    if(event->key() == Qt::Key_E)
    {
        mInput.E = true;
    }

    if(event->key() == Qt::Key_Space)
    {
        mSound->playMono();
    }
    if(event->key() == Qt::Key_I)
    {
        TestDia->AdvanceDialogueA();
    }
    if(event->key() == Qt::Key_O)
    {
        TestDia->AdvanceDialogueB();
    }
}

void RenderWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_W || event->key() == Qt::Key_Up)
    {
        mInput.W = false;
    }
    if(event->key() == Qt::Key_A || event->key() == Qt::Key_Left)
    {
        mInput.A = false;
    }
    if(event->key() == Qt::Key_S || event->key() == Qt::Key_Down)
    {
        mInput.S = false;
    }
    if(event->key() == Qt::Key_D || event->key() == Qt::Key_Right)
    {
        mInput.D = false;
    }
    if(event->key() == Qt::Key_Q)
    {
        mInput.Q = false;
    }
    if(event->key() == Qt::Key_E)
    {
        mInput.E = false;
    }
}
