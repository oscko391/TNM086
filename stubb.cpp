#include <osg/Version>

#include <osgViewer/Viewer>
#include <osgUtil/Optimizer>

#include <iostream>
#include <osg/Notify>
#include <osg/ShapeDrawable>
#include <osg/Node>
#include <osgDB/ReadFile>
#include <osg/PositionAttitudeTransform>
#include <osg/StateSet>
#include <osgUtil/Simplifier>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osg/AnimationPath>
#include <osg/MatrixTransform>



class IntersectRef : public osg::Referenced 
{
public:
  IntersectRef(osgUtil::IntersectionVisitor iv, osg::PositionAttitudeTransform* _model, osg::Light* l)
  {
    this->iv = iv;
    this->model = _model;
    this->light = l;
  }
        
  osgUtil::IntersectionVisitor getVisitor()  { return this->iv; }

  osg::Light* getLight() { return this->light; }
  osg::PositionAttitudeTransform* getModel() { return this->model; }

protected:
  osgUtil::IntersectionVisitor iv;
  osg::Group* root;
  osg::PositionAttitudeTransform* model;
  osg::Light* light;
};

class IntersectCallback : public osg::NodeCallback 
{
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    osg::ref_ptr<IntersectRef> intersectRef = 
      dynamic_cast<IntersectRef*>(node->getUserData());
    
    osgUtil::IntersectionVisitor visitor = intersectRef->getVisitor();
    node->accept(visitor);
    osg::ref_ptr<osgUtil::Intersector> lsi = visitor.getIntersector();
    
    if(lsi->containsIntersections())
    {
       std::cout<<"INTERSECTION!" << std::endl;
      intersectRef->getLight()->setDiffuse(osg::Vec4(0,0,1,1));
      intersectRef->getModel()->setPosition(osg::Vec3(10,10,10));
    }
    else
    {
        intersectRef->getLight()->setDiffuse(osg::Vec4(0,1,0,1));
        intersectRef->getModel()->setPosition(osg::Vec3(10,10,5));
    }
    
    lsi->reset();
    traverse(node,nv);
  }
};


int main(int argc, char *argv[]){
  
  osg::ref_ptr<osg::Group> root = new osg::Group;

#if 1
  /// Line ---

  osg::Vec3 line_p0 (10, -30, 0);
  osg::Vec3 line_p1 ( 10, 0, 0);
  
  osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
  vertices->push_back(line_p0);
  vertices->push_back(line_p1);
  
  osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
  colors->push_back(osg::Vec4(0.9f,0.2f,0.3f,1.0f));

  osg::ref_ptr<osg::Geometry> linesGeom = new osg::Geometry();
  linesGeom->setVertexArray(vertices);
  linesGeom->setColorArray(colors, osg::Array::BIND_OVERALL);
  
  linesGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,0,2));
  
  osg::ref_ptr<osg::Geode> lineGeode = new osg::Geode();
  lineGeode->addDrawable(linesGeom);
  lineGeode->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
  
  root->addChild(lineGeode);
  /// ---
  
  // heightfield
  unsigned int numCols = 30;
  unsigned int numRows = 30;
  osg::ref_ptr<osg::HeightField> heightField = new osg::HeightField;
  heightField->allocate(numCols, numRows);
  heightField->setOrigin(osg::Vec3(0, 0, 0));
  heightField->setXInterval(1.0f);
  heightField->setYInterval(1.0f);
  
  // set height
  for (int row = 0; row < heightField->getNumRows(); row++)
  {
      for (int col = 0; col < heightField->getNumColumns(); col++)
      {
          heightField->setHeight(col, row, 0.5 * (cos(col * 1.0) + sin(row * 2.0)));
      }
  }
  
  osg::ref_ptr<osg::Geode> heightGeode = new osg::Geode();
  heightGeode->addDrawable(new osg::ShapeDrawable(heightField));
  
  
  osg::Texture2D* tex = new osg::Texture2D(osgDB::readImageFile("groundtex.jpg"));
  tex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR_MIPMAP_LINEAR);
  tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
  tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
  tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
  heightGeode->getOrCreateStateSet()->setTextureAttributeAndModes(0,tex);
  
  root->addChild(heightGeode);
  /// ---
  
  //load objects
  osg::ref_ptr<osg::Node> gliderNode = NULL;
  osg::ref_ptr<osg::Node> cessnaNode = NULL;
  
  gliderNode = osgDB::readNodeFile("glider.osg");
  
  cessnaNode = osgDB::readNodeFile("cessna.osg");
  
   //simplifier LOD cessna
  float sampleRatio = 0.5f;
  osgUtil::Simplifier simplifier(sampleRatio);
  osg::ref_ptr<osg::Node> cessnaLowerDet = 
        dynamic_cast<osg::Node*>(cessnaNode->clone(osg::CopyOp::DEEP_COPY_ALL));
    
  cessnaLowerDet->accept(simplifier);
  
  osg::ref_ptr<osg::Node> cessnaEvenLowerDet = 
        dynamic_cast<osg::Node*>(cessnaNode->clone(osg::CopyOp::DEEP_COPY_ALL));

  simplifier.setSampleRatio(0.1);
  cessnaEvenLowerDet->accept(simplifier);
  
  //set up LOD nodes
  osg::ref_ptr<osg::LOD> cessnaLOD = new osg::LOD();
  cessnaLOD->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
  cessnaLOD->addChild(cessnaNode, 0.0, 50.0); 
  cessnaLOD->addChild(cessnaLowerDet, 50.0, 65.0);
  cessnaLOD->addChild(cessnaEvenLowerDet, 65.0, 99999.0);
  
  // transform
  osg::ref_ptr<osg::PositionAttitudeTransform> gliderTransform =   new osg::PositionAttitudeTransform();
  
  
    gliderTransform->addChild(gliderNode);
    
    osg::Vec3 gliderPosit(10, -10, 20);
    root->addChild(gliderTransform);
    gliderTransform->setPosition(gliderPosit);
    //gliderTransform->setScale(osg::Vec3(10.0, 10.0, 10.0));
    
    //Animation
    osg::ref_ptr<osg::AnimationPath> animationPath = new osg::AnimationPath;
    animationPath->setLoopMode(osg::AnimationPath::LOOP);
    int numberOfSamples = 40;
    float time = 0.0f;
    float deltaTime = 10.0f / (float)numberOfSamples;
    
    for (int i = 0; i < numberOfSamples; i++)
    {
        osg::Vec3 position(gliderPosit[0] + 10.0 * cosf(time), gliderPosit[1] + 10.0 * sinf(time), 0.0f); 
        animationPath->insert(time, osg::AnimationPath::ControlPoint(position));
        time += deltaTime;
    }
    gliderTransform->setUpdateCallback(new osg::AnimationPathCallback(animationPath, 0.0, 1.0));
    
    
    //cessna transform
    osg::ref_ptr<osg::PositionAttitudeTransform> cessnaTransform =   new osg::PositionAttitudeTransform();
  
    root->addChild(cessnaTransform);
    cessnaTransform->addChild(cessnaLOD);
    
    osg::Vec3 cessnaPosit(10, 10, 5);
    cessnaTransform->setPosition(cessnaPosit);
    cessnaTransform->setScale(osg::Vec3(0.5, 0.5, 0.5));
    
    //lights
    osg::StateSet* lightStateSet = root->getOrCreateStateSet();
    //light1
    osg::ref_ptr<osg::Light> light1 = new osg::Light();
    light1->setLightNum(0);
    osg::ref_ptr<osg::LightSource> lightSrc1 = new osg::LightSource();
    light1->setPosition(osg::Vec4(0.0, 0.0, 50.0, 1.0));
    light1->setDiffuse(osg::Vec4(0.0, 1.0, 0.0, 1.0));
    lightSrc1->setLight(light1);
    lightSrc1->setStateSetModes(*lightStateSet, osg::StateAttribute::ON);
    
    
    // animate light
    osg::ref_ptr<osg::AnimationPath> lightAnimation = new osg::AnimationPath;
    lightAnimation->setLoopMode(osg::AnimationPath::LOOP);
    time = 0.0f;
    
    for (int i = 0; i < numberOfSamples; i++)
    {
        osg::Vec3 position( 10.0*cosf(time*5.0), 10.0*sinf(time*5.0), 0.0f); 
        lightAnimation->insert(time, osg::AnimationPath::ControlPoint(position));
        time += deltaTime;
    }
    
    osg::ref_ptr<osg::MatrixTransform> light1T = new osg::MatrixTransform;
    light1T->setUpdateCallback(new osg::AnimationPathCallback(lightAnimation, 0.0, 1.0));
    light1T->addChild(lightSrc1);
    root->addChild(light1T);
    
    
    //light2
    osg::ref_ptr<osg::Light> light2 = new osg::Light();
    light2->setLightNum(1);
    osg::ref_ptr<osg::LightSource> lightSrc2 = new osg::LightSource();
    light2->setPosition(osg::Vec4(20.0, 0.0, 50.0, 1.0));
    light2->setDiffuse(osg::Vec4(1.0, 0.0,0.0, 1.0));
    lightSrc2->setLight(light2);
    lightSrc2->setStateSetModes(*lightStateSet, osg::StateAttribute::ON);
    root->addChild(lightSrc2);
    
    //intersector
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(line_p0, line_p1);
    
    osgUtil::IntersectionVisitor intersectVisitor;
    intersectVisitor.setIntersector(intersector);
    osg::ref_ptr<IntersectCallback> intersectCallback = new IntersectCallback();
    root->setUserData( new IntersectRef(intersectVisitor, cessnaTransform, light1));
    root->addUpdateCallback(intersectCallback);
  
#endif

  
  // Add your stuff to the root node here...
  
  
  // Optimizes the scene-graph
  //osgUtil::Optimizer optimizer;
  //optimizer.optimize(root);
  
  // Set up the viewer and add the scene-graph root
  osgViewer::Viewer viewer;
  viewer.setSceneData(root);

  osg::ref_ptr<osg::Camera> camera = new osg::Camera;
  camera->setProjectionMatrixAsPerspective(60.0, 1.0, 0.1, 100.0);
  camera->setViewMatrixAsLookAt (osg::Vec3d(0.0, 0.0, 2.0),
                                 osg::Vec3d(0.0, 0.0, 0.0),
                                 osg::Vec3d(0.0, 1.0, 0.0));
  camera->getOrCreateStateSet()->setGlobalDefaults();
  viewer.setCamera(camera);
  
  return viewer.run();
  

  
  
}
