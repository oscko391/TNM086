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



int main(int argc, char *argv[]){
  
  osg::ref_ptr<osg::Group> root = new osg::Group;

#if 1
  /// Line ---

  osg::Vec3 line_p0 (-1, 0, 0);
  osg::Vec3 line_p1 ( 1, 0, 0);
  
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
  
  osg::ref_ptr<osg::PositionAttitudeTransform> gliderTransform =   new osg::PositionAttitudeTransform();
  
  root->addChild(gliderTransform);
    gliderTransform->addChild(gliderNode);
    
    osg::Vec3 gliderPosit(3, 4, 4);
    gliderTransform->setPosition(gliderPosit);
    gliderTransform->setScale(osg::Vec3(3.0, 3.0, 3.0));
    
    //cessna transform
    osg::ref_ptr<osg::PositionAttitudeTransform> cessnaTransform =   new osg::PositionAttitudeTransform();
  
    root->addChild(cessnaTransform);
    cessnaTransform->addChild(cessnaNode);
    
    osg::Vec3 cessnaPosit(10, 10, 5);
    cessnaTransform->setPosition(cessnaPosit);
    cessnaTransform->setScale(osg::Vec3(0.2, 0.2, 0.2));
    
    //lights
    osg::ref_ptr<osg::Light> light1 = new osg::Light();
    light1->setLightNum(0);
    light1->setPosition(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    osg::Vec4 red = osg::Vec4(1.0, 0.0, 0.0, 1.0);
    light1->setDiffuse(red);
    light1->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    light1->setAmbient(osg::Vec4(0.9,0.2,0.0,1.0));
    
    osg::ref_ptr<osg::Geode> lightMarker[1];
    osg::ref_ptr<osg::LightSource> lightSource[1];
    
    osg::StateSet *lightStateSet;
    osg::PositionAttitudeTransform *lightTransform[1];
    
    for (int i = 0; i < 1; i++)
    {
        lightMarker[i] = new osg::Geode();
        lightMarker[i]->addDrawable(new osg::ShapeDrawable(new 
        osg::Sphere(osg::Vec3(), 1)));
        
        lightSource[i] = new osg::LightSource();
        lightSource[i]->setLight(light1);
        lightSource[i]->setLocalStateSetModes(osg::StateAttribute::ON);
        lightSource[i]->setStateSetModes(*lightStateSet, osg::StateAttribute::ON);
        
        lightTransform[i] = new osg::PositionAttitudeTransform();
        lightTransform[i]->addChild(lightSource[i]);
        lightTransform[i]->addChild(lightMarker[i]);
        lightTransform[i]->setPosition(osg::Vec3(0,0,5));
        lightTransform[i]->setScale(osg::Vec3(0.1,0.1,0.1)); 
        
        root->addChild(lightTransform[i]);
        
    }
    
    
  
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
