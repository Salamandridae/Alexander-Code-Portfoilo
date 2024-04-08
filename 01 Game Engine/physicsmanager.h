#ifndef PHYSICSMANAGER_H
#define PHYSICSMANAGER_H

#include <thread>
#include <vector>

//PhysX includes
#include <PxPhysicsAPI.h>
#include "PhysXClasses.h"
#include <PxPhysXConfig.h>
#include <foundation/PxMathUtils.h>
#include <foundation/PxQuat.h>
#include <PxFoundation.h>
#include <extensions/PxSimpleFactory.h>
#include "PhysXClasses.h"
#include "vertex.h"
#include "QOpenGLFunctions_4_1_Core"
#include "gameobject.h"

using namespace physx;

class PhysicsComponent
{
public:
    PhysicsComponent();
    ~PhysicsComponent();
    void initPhysics();
    void simulationStep(float dt);
    PxPhysics*  getPhysics(){return mPhysics;}
    PxScene*    getScene(){return mScene;}
    PxCooking*  getCooking(){return mCooking;}
    void createDynamic(GameObject* obj, const char* name, PxTransform pose, PxPhysics *physics, PxCooking *cooking, PxScene *scene);
    void creatStaticPhysics(GameObject* obj, const char *name, PxTransform pose);
    void createTestDynamic();
    void update(GameObject* obj);
    void helloWorldSnippets();
private:
     PxDefaultAllocator         mAllocator;
     PxDefaultErrorCallback     mErrorCallback;
     PxFoundation*              mFoundation         = nullptr;
     PxPhysics*                 mPhysics            = nullptr;
     PxDefaultCpuDispatcher*    mDispatcher         = nullptr;
     PxCudaContextManager*      mCudaCotextManager  = nullptr;
     PxScene*                   mScene              = nullptr;
     PxCooking*                 mCooking            = nullptr;
     PxMaterial*                mMaterial           = nullptr;
     PxTolerancesScale          mToleranceScale;
     PxCudaContextManagerDesc   mCudaContexDesc;

public:
    std::vector<PxRigidActor*> mRigidBodies;

private:
   PxPvd* mPvd = nullptr;
   std::thread t;
   const int mThreads = t.hardware_concurrency();
   class Logger* mLogger{nullptr};


};

#endif // PHYSICSMANAGER_H
