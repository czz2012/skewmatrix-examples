// Copyright (c) 2011 Skew Matrix Software LLC. All rights reserved.

#include "world.h"

#include <osgDB/ReadFile>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/NodeVisitor>

#include <osgwTools/AbsoluteModelTransform.h>
#include <osgwTools/Shapes.h>
#include <osgbDynamics/RigidBody.h>
#include <osgbDynamics/CreationRecord.h>



osg::Object* makeCreationRecord( osg::Transform* node, BroadphaseNativeTypes shape,
    double mass )
{
    osg::ref_ptr< osgbDynamics::CreationRecord > cr = new osgbDynamics::CreationRecord;

    cr->_sceneGraph = node;
    cr->_shapeType = shape;
    cr->_mass = mass;

    return( cr.release() );
}

osg::Node* build()
{
    osg::ref_ptr< osg::Group > root = new osg::Group;

    osgwTools::AbsoluteModelTransform* amt = new osgwTools::AbsoluteModelTransform;
    osg::Geode* geode = new osg::Geode;
    osg::Matrix m = osg::Matrix::translate( 0., 0., .5 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 50., 50., .5 ), osg::Vec3s( 100, 100, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );


    // Stairs
    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 1.25 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 20., 20., .25 ), osg::Vec3s( 40, 40, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 1.75 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 19., 19., .25 ), osg::Vec3s( 38, 38, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 2.25 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 18., 18., .25 ), osg::Vec3s( 36, 36, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 2.75 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 17., 17., .25 ), osg::Vec3s( 34, 34, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 3.25 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 16., 16., .25 ), osg::Vec3s( 32, 32, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( -25., -25., 3.75 );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( 15., 15., .25 ), osg::Vec3s( 30, 30, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 0. ) );

    // Floor height at top of stairs platform is 3.5.
    // Put some dynamic objects up here to walk into.


    // Test: Dynamic object right in front of the viewer.
    amt = new osgwTools::AbsoluteModelTransform;
    geode = new osg::Geode;
    m = osg::Matrix::translate( 0., 20., 12. );
    geode->addDrawable( osgwTools::makeBox( m, osg::Vec3( .5, .5, .5 ), osg::Vec3s( 1, 1, 1 ) ) );
    amt->addChild( geode );
    root->addChild( amt );
    amt->setUserData( makeCreationRecord( amt, BOX_SHAPE_PROXYTYPE, 1. ) );

    return( root.release() );
}



EnablePhysicsVisitor::EnablePhysicsVisitor( btDynamicsWorld* bw )
  : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ),
    _bw( bw )
{
}
EnablePhysicsVisitor::~EnablePhysicsVisitor()
{
}

void EnablePhysicsVisitor::apply( osg::Transform& node )
{
    osg::ref_ptr< osgbDynamics::CreationRecord > cr = dynamic_cast<
        osgbDynamics::CreationRecord* >( node.getUserData() );
    if( !( cr.valid() ) )
    {
        traverse( node );
        return;
    }

    osg::NodePath np = getNodePath();
    np.pop_back();
    cr->_parentTransform = osg::computeLocalToWorld( np );
    btRigidBody* rb = osgbDynamics::createRigidBody( cr.get() );

    //node.setUserData( new osgbCollision::RefRigidBody( rb ) );

    _bw->addRigidBody( rb );

    traverse( node );
    return;
}
