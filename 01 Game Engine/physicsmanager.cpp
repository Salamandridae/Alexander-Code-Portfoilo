#include "physicsmanager.h"

PhysicsComponent::PhysicsComponent()
{

}

PhysicsComponent::~PhysicsComponent()
{
 mPhysics->release();
 mCooking->release();
 mFoundation->release();
}

void PhysicsComponent::initPhysics()
{
    try{
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);

    mPvd = PxCreatePvd(*mFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    mPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);


    mToleranceScale.speed = 9.81f;
    mToleranceScale.length = 1.f;


    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mToleranceScale,true,mPvd);

    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    mDispatcher = PxDefaultCpuDispatcherCreate(mThreads/2);
    sceneDesc.cpuDispatcher	= mDispatcher;
    sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
    mScene = mPhysics->createScene(sceneDesc);
    PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
    mScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.f);  //->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);


        if(pvdClient)
        {
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
        mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(mToleranceScale));
        if (!mCooking)
            std::cerr << "Error with cooking lib";

     //mLogger->logText("Physx Init" + PxS,LogType::LOG);

    }catch (PxErrorCode::Enum errorcode) {
        mErrorCallback.reportError(errorcode,"Erorr in Physics init", "PhysicsManager.h",39);

    }
}

void PhysicsComponent::simulationStep(float dt)
{
  mScene->simulate(dt);
  mScene->fetchResults(true);
}

//convex mesh without serilazation
void PhysicsComponent::createDynamic(GameObject* obj, const char *name, PxTransform pose, PxPhysics* physics, PxCooking* cooking, PxScene* scene)
{
    PxRigidDynamic* dynamic = nullptr;
    PxCookingParams parameter = cooking->getParams();

    PxConvexMesh* mesh;
    PxConvexMeshDesc meshDescription;
    std::vector<Vertex> verticies = obj->vertices();
    std::vector<GLuint>indices = obj->indecies();
    std::vector<PxU16> tempIndecies;
    std::vector<PxVec3> tempVerticies;
    std::vector<PxVec3> tempPolygons;

    parameter.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;

    for(auto i = 0; i < verticies.size();i++)
    {
       float tX = verticies[i].getX();
       float tY = verticies[i].getY();
       float tZ = verticies[i].getZ();

       tempVerticies.push_back(PxVec3(tX,tY,tZ));

    }
    for(auto i = 0 ; i< indices.size();i++){
        tempIndecies.push_back(static_cast<PxU32>(indices[i]));
    }
    meshDescription.points.count = static_cast<PxU32>(tempVerticies.size());
    meshDescription.points.stride = sizeof(PxVec3);
    meshDescription.points.data = tempVerticies.data();
    meshDescription.flags = PxConvexFlag::eCOMPUTE_CONVEX;

    meshDescription.indices.count = tempIndecies.size()/3;
    meshDescription.indices.stride = 3*sizeof(PxU32);
    meshDescription.indices.data =  tempIndecies.data();


//    PxDefaultMemoryOutputStream bufferOutput;

//    PxConvexMeshCookingResult::Enum result;
//    cooking->validateConvexMesh(meshDescription);
//    bool test=meshDescription.isValid();
//    bool cook = cooking->cookConvexMesh(meshDescription, bufferOutput, &result);
//    if(!cook)
//        std::cout << "Cooking failed";

//    PxDefaultMemoryInputData bufferInput(bufferOutput.getData(),bufferOutput.getSize());

//    mesh = physics->createConvexMesh(bufferInput);//mCooking->createConvexMesh(meshDescription,mPhysics->getPhysicsInsertionCallback());

    mesh = cooking->createConvexMesh(meshDescription,physics->getPhysicsInsertionCallback());
    PxConvexMeshGeometry convexGeo(mesh);


    mMaterial = physics->createMaterial(0.5,0.5,0.5);
    dynamic = PxCreateDynamic(*physics,pose,convexGeo,*mMaterial,1.f);//*mPhysics, pose, convexGeo, *mMaterial);
    dynamic->setName(name);
    scene->addActor(*dynamic);
    mRigidBodies.push_back(dynamic);
}
void PhysicsComponent::creatStaticPhysics(GameObject* obj, const char* name, PxTransform pose)
{
        PxTriangleMesh* triogem;
        PxTriangleMeshDesc triangleDescription;

        PxRigidStatic* mTerrain;
        std::vector<Vertex> tempVertices = obj->vertices();
        std::vector<GLuint> tempIndices = obj->indecies();

        std::vector<PxU32> PxIndecies;
        std::vector<PxVec3> PxVerticies;
        for(auto i = 0; i < tempVertices.size(); i++)
        {
             PxReal px = tempVertices[i].getX();
             PxReal py = tempVertices[i].getY();
             PxReal pz = tempVertices[i].getZ();

             PxVerticies.push_back(PxVec3(px,py,pz));
        }
        for(auto i = 0; i< tempIndices.size(); i++)
        {
            PxIndecies.push_back(static_cast<PxU32>(tempIndices[i]));
        }

       triangleDescription.points.count = (PxU32)PxVerticies.size();
       triangleDescription.points.stride = sizeof(PxVec3);
       triangleDescription.points.data = PxVerticies.data();

       triangleDescription.triangles.count = (PxU32)PxIndecies.size()/3;
       triangleDescription.triangles.stride = 3*sizeof(PxU32);
       triangleDescription.triangles.data = PxIndecies.data();


       PxDefaultMemoryOutputStream buffer;

       PxTriangleMeshCookingResult::Enum result;

        mCooking->validateTriangleMesh(triangleDescription);
        bool stat = mCooking->cookTriangleMesh(triangleDescription, buffer, &result);
         if(!stat){
              std::cout << "cooking failed";
         }

        PxDefaultMemoryInputData bufferInput(buffer.getData(),buffer.getSize());

         triogem = mPhysics->createTriangleMesh(bufferInput);
         mMaterial = mPhysics->createMaterial(0.5,0.5,0.5);

         PxTransform transform(PxVec3(0.0f, 0.0f, 0.0f));
         PxTriangleMeshGeometry trigeo(triogem);
         mTerrain = PxCreateStatic(*mPhysics,transform,trigeo,*mMaterial);
         mTerrain->setName(name);

         mScene->addActor(*mTerrain);
}

void PhysicsComponent::createTestDynamic()
{
  PxRigidDynamic* dynamic;
  PxMaterial* mat = mPhysics->createMaterial(0.5f, 0.5f, 0.5f);
  dynamic = PxCreateDynamic(*mPhysics,PxTransform(PxVec3(0,0,10)),PxSphereGeometry(1),*mat,1.f);
  dynamic->setAngularVelocity(PxVec3(0,0,10));
  dynamic->setMass(100);
  mScene->addActor(*dynamic);
  mRigidBodies.push_back(dynamic);
}

void PhysicsComponent::update(GameObject *obj)
{
      for(auto i = 0; i < mRigidBodies.size();i++)
      {

          if(mRigidBodies[i]->getName() == obj->getName())
          {
              PxMat44 m = mRigidBodies[i]->getGlobalPose();
              obj->mMatrix.setColumn(0,QVector4D(m.column0.x,m.column0.y,m.column0.z,m.column0.w));
              obj->mMatrix.setColumn(1,QVector4D(m.column1.x,m.column1.y,m.column1.z,m.column1.w));
              obj->mMatrix.setColumn(2,QVector4D(m.column2.x,m.column2.y,m.column2.z,m.column2.w));
              obj->mMatrix.setColumn(3,QVector4D(m.column3.x,m.column3.y,m.column3.z,m.column3.w));
          }
      }
}

void PhysicsComponent::helloWorldSnippets()
{
    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

        PxRigidStatic* groundPlane = PxCreatePlane(*mPhysics, PxPlane(0,0,1,0), *mMaterial);
        mScene->addActor(*groundPlane);

    const PxTransform t(PxVec3(0,5,10));
            PxU32 size = 10;
    PxReal halfExtent = 10;
    PxShape* shape = mPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *mMaterial);
    for(PxU32 i=0; i<size;i++)
    {
        for(PxU32 j=0;j<size-i;j++)
        {
            PxTransform localTm(PxVec3(PxReal(j*2) - PxReal(size-i), PxReal(i*2+1), 0) * halfExtent);
            PxRigidDynamic* body = mPhysics->createRigidDynamic(t.transform(localTm));
            body->attachShape(*shape);
            PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
            mScene->addActor(*body);
        }
    }
    shape->release();
    PxRigidDynamic* dynamic = PxCreateDynamic(*mPhysics, t, PxSphereGeometry(1), *mMaterial, 10.0f);
    dynamic->setAngularDamping(0.5f);
    dynamic->setLinearVelocity(PxVec3(0,10,10));
    mScene->addActor(*dynamic);


}
