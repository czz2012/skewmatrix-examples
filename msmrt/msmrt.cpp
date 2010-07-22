// Copyright (c) 2008 Skew Matrix Software LLC. All rights reserved.

#include <osg/Node>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/GLExtensions>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgwTools/Version.h>

#include <string>


const int winW( 800 ), winH( 600 );


#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 (GL_COLOR_ATTACHMENT0+1)
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1 
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_DRAW_FRAMEBUFFER_BINDING
#define GL_DRAW_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING
#endif
#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER GL_RENDERBUFFER_EXT
#endif

class MSMRTCallback : public osg::Camera::DrawCallback
{
public:
    MSMRTCallback( osg::Camera* cam )
      : _cam( cam ),
        __glGetFramebufferAttachmentParameteriv( NULL ),
        __glFramebufferRenderbuffer( NULL )
    {
    }

    virtual void operator()( osg::RenderInfo& renderInfo ) const
    {
        osg::State& state = *renderInfo.getState();
        const unsigned int ctx = state.getContextID();
        osg::FBOExtensions* fboExt = osg::FBOExtensions::instance( ctx, true );

        if( __glGetFramebufferAttachmentParameteriv == NULL )
        {
            // TBD needs to be per-context and thread-safe
            osg::setGLExtensionFuncPtr( __glGetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv" );
            osg::setGLExtensionFuncPtr( __glFramebufferRenderbuffer, "glFramebufferRenderbuffer" );
        }

        const GLint width = _cam->getViewport()->width();
        const GLint height = _cam->getViewport()->height();

#if 0
        // Make sure something is actually bound.

        // By default, RenderStage unbinds, and the default
        // framebuffer (is 0) is bound.
        // We use the KeepFBOsBoundCallback class to disable FBO unbind.
        // We'll unbind it ourself, at the end of this function.
        GLint drawFBO, readFBO;
        glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO );
        glGetIntegerv( GL_READ_FRAMEBUFFER_BINDING, &readFBO );
        osg::notify( osg::ALWAYS ) << "draw " << std::hex << drawFBO << ",  read " << std::hex << readFBO << std::endl;
#endif

        // BlitFramebuffer blits to all attached color buffers in the
        // draw FBO. We only want to blit to attachment1, so aave
        // attachment0 and then unbind it.
        GLint destColorTex0;
        __glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &destColorTex0 );
        fboExt->glFramebufferTexture2DEXT(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, 0, 0 );

        // Verification
        //osg::notify( osg::ALWAYS ) << "Dest " << std::hex << destColorTex0 << std::endl;

        // Set draw and read buffers to attachment1 to avoid
        // INVALID_FRAMEBUFFER_OPERATION error.
        glDrawBuffer( GL_COLOR_ATTACHMENT1 );
        glReadBuffer( GL_COLOR_ATTACHMENT1 );

        // Blit, from (multisampled read FBO) attachment1 to
        // (non-multisampled draw FBO) attachment1.
        fboExt->glBlitFramebufferEXT( 0, 0, width, height, 0, 0, width, height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST );

        // Restore draw and read buffers
        glDrawBuffer( GL_COLOR_ATTACHMENT0 );
        glReadBuffer( GL_COLOR_ATTACHMENT0 );

        // Restore the draw FBO's attachment0.
        fboExt->glFramebufferTexture2DEXT(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, destColorTex0, 0 );

        // We disabled FBO unbinding in the RenderStage,
        // so do it ourself here.
        fboExt->glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
    }

protected:
    osg::ref_ptr< osg::Camera > _cam;

    typedef void APIENTRY TglGetFramebufferAttachmentParameteriv( GLenum, GLenum, GLenum, GLint* );
    mutable TglGetFramebufferAttachmentParameteriv* __glGetFramebufferAttachmentParameteriv;

    typedef void APIENTRY TglFramebufferRenderbuffer( GLenum, GLenum, GLenum, GLuint );
    mutable TglFramebufferRenderbuffer* __glFramebufferRenderbuffer;
};

class KeepFBOsBoundCallback : public osg::NodeCallback
{
public:
    KeepFBOsBoundCallback() {}

    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        if( nv->getVisitorType() != osg::NodeVisitor::CULL_VISITOR )
        {
            traverse( node, nv );
            return;
        }

        osgUtil::CullVisitor* cv = dynamic_cast< osgUtil::CullVisitor* >( nv );
        osgUtil::RenderStage* rs = cv->getRenderStage();
        rs->setDisableFboAfterRender( false );

        traverse( node, nv );
    }
};

void
mrtStateSet( osg::StateSet* ss )
{
    ss->addUniform( new osg::Uniform( "tex", 0 ) );

    std::string fragsource = 
        "uniform sampler2D tex; \n"
        "void main() \n"
        "{ \n"
            "gl_FragData[0] = texture2D( tex, gl_TexCoord[0].st ); // gl_Color; \n"
            "gl_FragData[1] = vec4( 0.0, 0.0, 1.0, 0.0 ); \n"
        "} \n";
    osg::Shader* fragShader = new osg::Shader();
    fragShader->setType( osg::Shader::FRAGMENT );
    fragShader->setShaderSource( fragsource );

    osg::Program* program = new osg::Program();
    program->addShader( fragShader );
    ss->setAttribute( program, osg::StateAttribute::ON );
}
osg::StateSet*
mrtStateSetTriPair( osg::Texture2D* tex0, osg::Texture2D* tex1 )
{
    osg::StateSet* ss = new osg::StateSet;

    ss->setTextureAttributeAndModes( 0, tex0, osg::StateAttribute::ON );
    ss->setTextureAttributeAndModes( 1, tex1, osg::StateAttribute::ON );

    ss->addUniform( new osg::Uniform( "tex0", 0 ) );
    ss->addUniform( new osg::Uniform( "tex1", 1 ) );

    std::string fragsource = 
        "uniform sampler2D tex0; \n"
        "uniform sampler2D tex1; \n"
        "void main() \n"
        "{ \n"
            "gl_FragData[0] = texture2D( tex0, gl_TexCoord[0].st ) \n"
                " + texture2D( tex1, gl_TexCoord[0].st ); \n"
        "} \n";
    osg::Shader* fragShader = new osg::Shader();
    fragShader->setType( osg::Shader::FRAGMENT );
    fragShader->setShaderSource( fragsource );

    osg::Program* program = new osg::Program();
    program->addShader( fragShader );
    ss->setAttribute( program, osg::StateAttribute::ON );

    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    return( ss );
}


osg::Node*
postRender( osgViewer::Viewer& viewer )
{
    osg::Camera* rootCamera( viewer.getCamera() );

    // MRT: Attach two color buffers to the root camera, one for
    // the standard color image, and another for the glow color.
    osg::Texture2D* tex0 = new osg::Texture2D;
    tex0->setTextureWidth( winW );
    tex0->setTextureHeight( winH );
    tex0->setInternalFormat( GL_RGBA );
    tex0->setBorderWidth( 0 );
    tex0->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex0->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );
    // Full color: attachment 0
    rootCamera->attach( osg::Camera::COLOR_BUFFER0, tex0, 0, 0, false, 8, 8 );

    osg::Texture2D* tex1 = new osg::Texture2D;
    tex1->setTextureWidth( winW );
    tex1->setTextureHeight( winH );
    tex1->setInternalFormat( GL_RGBA );
    tex1->setBorderWidth( 0 );
    tex1->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex1->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );
    // Glow color: attachment 1
    rootCamera->attach( osg::Camera::COLOR_BUFFER1, tex1, 0, 0, false, 8, 8 );

    rootCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
#if( OSGWORKS_OSG_VERSION >= 20906 )
    rootCamera->setImplicitBufferAttachmentMask(
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT );
#endif

    // Post-draw callback on root camera handles resolving
    // multisampling for the MRT case.
    MSMRTCallback* msmrt = new MSMRTCallback( rootCamera );
    rootCamera->setPostDrawCallback( msmrt );

    // Set fragment program for MRT.
    mrtStateSet( rootCamera->getOrCreateStateSet() );



    // Configure postRenderCamera to draw fullscreen textured quad
    osg::ref_ptr< osg::Camera > postRenderCamera( new osg::Camera );
    postRenderCamera->setClearColor( osg::Vec4( 0., 1., 0., 1. ) ); // should never see this.
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );

    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->setViewMatrix( osg::Matrixd::identity() );
    postRenderCamera->setProjectionMatrix( osg::Matrixd::identity() );

    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osg::createTexturedQuadGeometry(
        osg::Vec3( -1,-1,0 ), osg::Vec3( 2,0,0 ), osg::Vec3( 0,2,0 ) ) );
    geode->setStateSet( mrtStateSetTriPair( tex0, tex1 ) );

    postRenderCamera->addChild( geode );

    return( postRenderCamera.release() );
}

int
main( int argc, char** argv )
{
    osg::ref_ptr< osg::Group > root( new osg::Group );
    root->addChild( osgDB::readNodeFile( "cow.osg" ) );
    if( root->getNumChildren() == 0 )
        return( 1 );

    // Do not unbind the FBOs after the BlitFramebuffer call.
    root->setCullCallback( new KeepFBOsBoundCallback() );


    osgViewer::Viewer viewer;
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.setUpViewInWindow( 10, 30, winW, winH );
    viewer.setSceneData( root.get() );
    viewer.realize();

    root->addChild( postRender( viewer ) );

    // Clear to white to make AA extremely obvious.
    viewer.getCamera()->setClearColor( osg::Vec4( 1., 1., 1., 1. ) );

    return( viewer.run() );
}