//------------------------------------------------------------------------------
//  InfiniteSpheres.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Application/App.h"
#include "Render/RenderFacade.h"
#include "Render/Util/RawMeshLoader.h"
#include "Render/Util/ShapeBuilder.h"
#define GLM_FORCE_RADIANS
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"

using namespace Oryol;
using namespace Oryol::Application;
using namespace Oryol::Render;
using namespace Oryol::Resource;

OryolApp("InfiniteSpheres", "1.0");

// derived application class
class InfiniteSpheresApp : public App {
public:
    virtual AppState::Code OnInit();
    virtual AppState::Code OnRunning();
    virtual AppState::Code OnCleanup();
    
private:
    glm::mat4 computeModel(float32 rotX, float32 rotY, const glm::vec3& pos);
    glm::mat4 computeMVP(const glm::mat4& proj, const glm::mat4& model);

    RenderFacade* render = nullptr;
    Resource::Id renderTargets[2];
    Resource::Id sphere;
    Resource::Id prog;
    glm::mat4 view;
    glm::mat4 offscreenProj;
    glm::mat4 displayProj;
    float32 angleX = 0.0f;
    float32 angleY = 0.0f;
    int32 frameIndex = 0;
    
    // shader slots
    static const int32 ModelViewProjection = 0;
    static const int32 Texture = 2;
};

// vertex shader for rendering to render target
static const char* vsSource =
"uniform mat4 mvp;\n"
"VS_INPUT(vec4, position);\n"
"VS_INPUT(vec4, normal);\n"
"VS_INPUT(vec2, texcoord0);\n"
"VS_OUTPUT(vec4, nrm);\n"
"VS_OUTPUT(vec2, uv);\n"
"void main() {\n"
"  gl_Position = mvp * position;\n"
"  nrm = normal;\n"
"  uv  = texcoord0;\n"
"}\n";

// fragment shader for rendering to render target
static const char* fsSource =
"uniform sampler2D tex;\n"
"FS_INPUT(vec4, nrm);\n"
"FS_INPUT(vec2, uv);\n"
"void main() {\n"
"  vec4 texColor = TEXTURE2D(tex, uv * vec2(5.0, 3.0));"
"  FragmentColor = ((nrm * 0.5) + 0.5) * 0.75 + texColor * texColor * texColor * texColor;\n"
"}\n";

//------------------------------------------------------------------------------
void
OryolMain() {
    // execution starts here, create our app and start the main loop
    InfiniteSpheresApp app;
    app.StartMainLoop();
}

//------------------------------------------------------------------------------
AppState::Code
InfiniteSpheresApp::OnInit() {
    // setup rendering system
    this->render = RenderFacade::CreateSingleton();
    this->render->AttachLoader(RawMeshLoader::Create());
    this->render->Setup(RenderSetup::Windowed(800, 600, "Oryol Infinite Spheres Sample"));

    // create 2 offscreen render targets
    for (int32 i = 0; i < 2; i++) {
        auto rtSetup = TextureSetup::AsRenderTarget(Locator::NonShared(), 512, 512, PixelFormat::R8G8B8, PixelFormat::D16);
        rtSetup.SetMinFilter(TextureFilterMode::Linear);
        rtSetup.SetMagFilter(TextureFilterMode::Linear);
        rtSetup.SetWrapU(TextureWrapMode::Repeat);
        rtSetup.SetWrapV(TextureWrapMode::Repeat);
        this->renderTargets[i] = this->render->CreateResource(rtSetup);
    }
    
    // create a sphere mesh with normals and uv coords
    ShapeBuilder shapeBuilder;
    shapeBuilder.AddComponent(VertexAttr::Position, VertexFormat::Float3);
    shapeBuilder.AddComponent(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder.AddComponent(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder.AddSphere(0.75f, 72.0f, 40.0f);
    shapeBuilder.Build();
    this->sphere = this->render->CreateResource(MeshSetup::FromData("sphere"), shapeBuilder.GetStream());

    // build shader for rendering to render-target
    Id vs = this->render->CreateResource(ShaderSetup::FromSource("vs", ShaderType::VertexShader, vsSource));
    Id fs = this->render->CreateResource(ShaderSetup::FromSource("fs", ShaderType::FragmentShader, fsSource));
    ProgramBundleSetup progSetup("rtProg");
    progSetup.AddProgram(0, vs, fs);
    progSetup.AddUniform("mvp", ModelViewProjection);
    progSetup.AddTextureUniform("tex", Texture);
    this->prog = this->render->CreateResource(progSetup);
    
    // can release vertex- and fragment shader handles now
    this->render->DiscardResource(vs);
    this->render->DiscardResource(fs);
    
    // setup static transform matrices
    this->offscreenProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.01f, 20.0f);
    this->displayProj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 20.0f);
    this->view = glm::mat4();
    
    return AppState::Running;
}

//------------------------------------------------------------------------------
glm::mat4
InfiniteSpheresApp::computeModel(float32 rotX, float32 rotY, const glm::vec3& pos) {
    glm::mat4 modelTform = glm::translate(glm::mat4(), pos);
    modelTform = glm::rotate(modelTform, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
    modelTform = glm::rotate(modelTform, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
    return modelTform;
}

//------------------------------------------------------------------------------
glm::mat4
InfiniteSpheresApp::computeMVP(const glm::mat4& proj, const glm::mat4& modelTform) {
    return proj * this->view * modelTform;
}

//------------------------------------------------------------------------------
AppState::Code
InfiniteSpheresApp::OnRunning() {
    // render one frame
    if (this->render->BeginFrame()) {
        
        // update angles
        this->angleY += 0.01f;
        this->angleX += 0.02f;
        this->frameIndex++;
        const int32 index0 = this->frameIndex % 2;
        const int32 index1 = (this->frameIndex + 1) % 2;
        
        // general render states
        this->render->ApplyState(Render::State::DepthMask, true);
        this->render->ApplyState(Render::State::DepthTestEnabled, true);
        this->render->ApplyState(Render::State::DepthFunc, Render::State::LessEqual);
        this->render->ApplyState(Render::State::ClearDepth, 1.0f);
        
        // render sphere to offscreen render target, using the other render target as
        // source texture
        this->render->ApplyRenderTarget(this->renderTargets[index0]);
        this->render->ApplyState(Render::State::ClearColor, 0.0f, 0.0f, 0.0f, 0.0f);
        this->render->Clear(true, true, true);
        this->render->ApplyMesh(this->sphere);
        this->render->ApplyProgram(this->prog, 0);
        glm::mat4 model = this->computeModel(this->angleX, this->angleY, glm::vec3(0.0f, 0.0f, -2.0f));
        glm::mat4 mvp = this->computeMVP(this->offscreenProj, model);
        this->render->ApplyVariable(ModelViewProjection, mvp);
        this->render->ApplyVariable(Texture, this->renderTargets[index1]);
        this->render->Draw(0);
        
        // ...and again to display
        this->render->ApplyRenderTarget(Resource::Id());
        this->render->ApplyState(Render::State::ClearColor, 0.25f, 0.25f, 0.25f, 0.0f);
        this->render->Clear(true, true, true);
        model = this->computeModel(-this->angleX, -this->angleY, glm::vec3(0.0f, 0.0f, -2.0f));
        mvp = this->computeMVP(this->displayProj, model);
        this->render->ApplyVariable(ModelViewProjection, mvp);
        this->render->ApplyVariable(Texture, this->renderTargets[index0]);
        this->render->Draw(0);
        
        this->render->EndFrame();
    }
    
    // continue running or quit?
    return render->QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
InfiniteSpheresApp::OnCleanup() {
    // cleanup everything
    this->render->DiscardResource(this->prog);
    this->render->DiscardResource(this->sphere);
    for (int32 i = 0; i < 2; i++) {
        this->render->DiscardResource(this->renderTargets[i]);
    }
    this->render->Discard();
    this->render = nullptr;
    RenderFacade::DestroySingleton();
    return AppState::Destroy;
}
