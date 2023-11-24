#include "geGL/DebugMessage.h"
#include "geGL/OpenGL.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"
#include<iostream>

#include<SDL.h>
#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>

#include<bunny.hpp>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<libs/stb_image/stb_image.h>

#define OPENGL_OLD

#ifndef CMAKE_ROOT_DIR
#define CMAKE_ROOT_DIR "."
#endif

using namespace ge::gl;

GLuint createShader(GLenum type,std::string const&src){
  GLuint vs = glCreateShader(type);
  char const*p[]={src.c_str()};
  glShaderSource(vs,1,p,0);
  glCompileShader(vs);
  GLint status;
  glGetShaderiv(vs,GL_COMPILE_STATUS,&status);
  if(status != GL_TRUE){
    char buffer[1000];
    glGetShaderInfoLog(vs,1000,0,buffer);
    std::cerr << buffer << std::endl;
  }
  return vs;
}

GLuint createProgram(std::vector<GLuint>const&shaders){
  GLuint prg = glCreateProgram();
  for(auto const&s:shaders)
    glAttachShader(prg,s);
  glLinkProgram(prg);

  GLint status;
  glGetProgramiv(prg,GL_LINK_STATUS,&status);
  if(status != GL_TRUE){
    char buffer[1000];
    glGetProgramInfoLog(prg,1000,0,buffer);
    std::cerr << buffer << std::endl;
  }
  return prg;
}

void addAttrib(GLuint vao,GLuint vbo,GLint a,GLuint n,GLenum t,GLsizei o,GLsizei s){
#ifdef OPENGL_OLD
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glEnableVertexAttribArray(a);
  glVertexAttribPointer(a,n,t,GL_FALSE,s,(void*)((size_t)o));
#else
  glVertexArrayAttribBinding(vao,a,a);
  glEnableVertexArrayAttrib(vao,a);
  glVertexArrayAttribFormat(vao,
    a,//attrib index
    n,//nof components (vec2)
    t,//type
    GL_FALSE,//normalization
    0);//relative offset
  glVertexArrayVertexBuffer(vao,a,
    vbo,
    o,//offset
    s);//stride

#endif
}

void addElementBuffer(GLuint vao,GLuint ebo){
#ifdef OPENGL_OLD
  glBindVertexArray(vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
#else
  glVertexArrayElementBuffer(vao,ebo);
#endif
}

GLuint createBuffer(GLsizeiptr size,void const*data){
  GLuint id;
#ifdef OPENGL_OLD
  glGenBuffers(1,&id);
  glBindBuffer(GL_ARRAY_BUFFER,id);
  glBufferData(
      GL_ARRAY_BUFFER,
      size,
      data,
      GL_DYNAMIC_DRAW);
#else
  glCreateBuffers(1,&id);
  glNamedBufferData(
      id,
      size,
      data,
      GL_DYNAMIC_DRAW);
#endif
  return id;
}

GLuint createVertexArray(){
  GLuint vao;
#ifdef OPENGL_OLD
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);
#else
  glCreateVertexArrays(1,&vao);
#endif
  return vao;
}

int main(int argc,char*argv[]){
  int width = 1024;
  int height = 768;

  auto window = SDL_CreateWindow("PGRe2023",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_OPENGL);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  auto context = SDL_GL_CreateContext(window);

  ge::gl::init(SDL_GL_GetProcAddress);
  //ge::gl::setDefaultDebugMessage();


  GLint major;
  GLint minor;
  glGetIntegerv(GL_MAJOR_VERSION,&major);
  glGetIntegerv(GL_MINOR_VERSION,&minor);
  std::cerr << major << minor << "0" << std::endl;
  std::cerr << glGetString( GL_VERSION) << std::endl;


  auto vsSrc = R".(
  #version 410


  layout(location=0)in vec3 position;
  layout(location=1)in vec3 normal  ;

  uniform mat4 proj  = mat4(1.f);
  uniform mat4 view  = mat4(1.f);
  uniform mat4 model = mat4(1.f);

  uniform mat4 lightView = mat4(1.f);
  uniform mat4 lightProj = mat4(1.f);

  out vec4 vertInsideShadowMap;

  uniform vec3 lightPosition = vec3(10,10,10);

  out vec3 vNormal;

  out vec3 vPosition;

  out vec3 vLambert;
  out vec3 vPhong  ;

  void main(){
    vNormal = normal;

    vPosition = vec3(model*vec4(position,1));
    vNormal   = vec3(transpose(inverse(model))*vec4(normal,0));

    vec3 lightColor           = vec3(1,1,0);
    vec3 ambientLightColor    = vec3(.4,0,0);
    vec3 materialColor        = vec3(.5);
    vec3 specularMatrialColor = vec3(1);

    vec3 cameraPosition = vec3(inverse(view)*vec4(0,0,0,1));

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPosition - vPosition);

    float diffuseFactor = clamp(dot(N,L),0.f,1.f);

    vec3 ambient = ambientLightColor * materialColor;
    vec3 diffuse = lightColor * materialColor * diffuseFactor;

    vec3 R = -reflect(L,N);
    float shininess = 100;
    float specularFactor = pow(clamp(dot(R,L),0.f,1.f),shininess);
    vec3 specular = specularFactor * lightColor * specularMatrialColor;


    vec3 lambert = ambient + diffuse;
    vec3 phong = ambient + diffuse + specular;

    vLambert = lambert;
    vPhong   = phong  ;

    gl_Position = proj*view*model*vec4(position,1);

    vertInsideShadowMap = lightProj*lightView*model*vec4(position,1);

  }
  ).";


  auto fsSrc = R".(
  #version 410
  
  in vec3 vNormal;
  in vec3 vPosition;

  in vec3 vLambert;
  in vec3 vPhong  ;

  uniform int usePhongShading = 1;
  uniform mat4 view  = mat4(1.f);
  uniform vec3 lightPosition = vec3(10,10,10);

  uniform sampler2D shadowMap;

  in vec4 vertInsideShadowMap;

  out vec4 fColor;
  void main(){

    vec3 lightColor           = vec3(1,1,0);
    vec3 ambientLightColor    = vec3(.4,0,0);
    vec3 materialColor        = vec3(.5);
    vec3 specularMatrialColor = vec3(1);

    vec3 cameraPosition = vec3(inverse(view)*vec4(0,0,0,1));

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPosition - vPosition);
    vec3 V = normalize(cameraPosition - vPosition);

    float diffuseFactor = clamp(dot(N,L),0.f,1.f);

    vec3 ambient = ambientLightColor * materialColor;
    vec3 diffuse = lightColor * materialColor * diffuseFactor;

    vec3 R = -reflect(L,N);
    float shininess = 100;
    float specularFactor = pow(clamp(dot(R,L),0.f,1.f),shininess);
    vec3 specular = specularFactor * lightColor * specularMatrialColor;


    vec3 ndc = vertInsideShadowMap.xyz / vertInsideShadowMap.w;
    ndc*=.5f;
    ndc+=.5f;
  
    float shadowSampleDistanceToTheLight = texture(shadowMap,ndc.xy).r;
    float viewSampleDistanceToTheLight  = ndc.z;

    float lit = 0.f;
    if(viewSampleDistanceToTheLight > shadowSampleDistanceToTheLight){
      lit = 0.f; 
    }else{
      lit = 1.f;
    }



    vec3 lambert = ambient + (diffuse)*lit;
    vec3 phong = ambient + (diffuse + specular)*lit;


    if(usePhongShading == 1)
      fColor = vec4(phong,1);
    else
      fColor = vec4(vPhong,1);
  }

  ).";

  auto texVsSrc = R".(
  #version 410
  #line 251
  
  uniform mat4 view = mat4(1.);
  uniform mat4 proj = mat4(1.);

  out vec2 vTexCoord;

  void main(){
    mat4 pv = proj * view;
    if(gl_VertexID==0){gl_Position = pv*vec4(0,0,0,1);vTexCoord=vec2(0,1);}
    if(gl_VertexID==1){gl_Position = pv*vec4(1,0,0,1);vTexCoord=vec2(1,1);}
    if(gl_VertexID==2){gl_Position = pv*vec4(0,1,0,1);vTexCoord=vec2(0,0);}
    if(gl_VertexID==3){gl_Position = pv*vec4(1,1,0,1);vTexCoord=vec2(1,0);}
    
  }
  ).";

  auto texFsSrc = R".(
  #version 410
  #line 263

  in vec2 vTexCoord;

  uniform sampler2D myTex;
  uniform sampler2D shadowMap;

  uniform float near_plane = 0.1f;
  uniform float far_plane = 10.f;

  float linearizeDepth(float depth)
  {
      float z = depth * 2.0 - 1.0; // Back to NDC 
      return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
  }


  out vec4 fColor;
  void main(){
    //fColor = vec4(vTexCoord,0,1);
    //fColor = texture(myTex,vTexCoord);

    float depth = texture(shadowMap,vTexCoord).r;
    float linDepth = linearizeDepth(depth) / far_plane;

    fColor = vec4(linDepth,linDepth,linDepth,1);
  }

  ).";


  GLuint vs  = createShader(GL_VERTEX_SHADER,vsSrc);
  GLuint fs  = createShader(GL_FRAGMENT_SHADER,fsSrc);
  GLuint prg = createProgram({vs,fs});


  GLuint angleL           = glGetUniformLocation(prg,"angle"          );
  GLuint projL            = glGetUniformLocation(prg,"proj"           );
  GLuint viewL            = glGetUniformLocation(prg,"view"           );
  GLuint modelL           = glGetUniformLocation(prg,"model"          );
  GLuint lightViewL       = glGetUniformLocation(prg,"lightView"      );
  GLuint lightProjL       = glGetUniformLocation(prg,"lightProj"      );
  GLuint usePhongShadingL = glGetUniformLocation(prg,"usePhongShading");
  GLuint lightPositionL   = glGetUniformLocation(prg,"lightPosition"  );
  GLuint shadowMapL       = glGetUniformLocation(prg,"shadowMap"      );

  GLuint texVs  = createShader(GL_VERTEX_SHADER,texVsSrc);
  GLuint texFs  = createShader(GL_FRAGMENT_SHADER,texFsSrc);
  GLuint texPrg = createProgram({texVs,texFs});
  GLuint texProjL            = glGetUniformLocation(texPrg,"proj"           );
  GLuint texViewL            = glGetUniformLocation(texPrg,"view"           );
  GLuint texL                = glGetUniformLocation(texPrg,"myTex"          );
  GLuint texShadowMapL       = glGetUniformLocation(texPrg,"shadowMap"      );

  auto lightPosition = glm::vec3(0.1f,3.f,0.1f);


  int x,y,channels;
  auto data = stbi_load(CMAKE_ROOT_DIR "/resources/example.png",&x,&y,&channels,0);


  GLuint tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);

  glPixelStorei(GL_UNPACK_ROW_LENGTH,x);
  glPixelStorei(GL_UNPACK_ALIGNMENT ,1);
  if(channels==4)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,x,y,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
  if(channels==3)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,x,y,0,GL_RGB,GL_UNSIGNED_BYTE,data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);


  GLuint shadowMap;
  glGenTextures(1,&shadowMap);
  glBindTexture(GL_TEXTURE_2D,shadowMap);
  glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,1024,1024,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);


  GLuint shadowFBO;
  glGenFramebuffers(1,&shadowFBO);
  glBindFramebuffer(GL_FRAMEBUFFER,shadowFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowMap,0);
  glBindFramebuffer(GL_FRAMEBUFFER,0);




  float angle = 0.f;
  int usePhongShading = 1;


  GLuint vbo = createBuffer(sizeof(bunnyVertices),bunnyVertices);
  GLuint ebo = createBuffer(sizeof(bunnyIndices),bunnyIndices);

  GLuint vao = createVertexArray();
  addAttrib(vao,vbo,0,3,GL_FLOAT,sizeof(float)*0,sizeof(float)*6);
  addAttrib(vao,vbo,1,3,GL_FLOAT,sizeof(float)*3,sizeof(float)*6);
  addElementBuffer(vao,ebo);


  auto v = glm::vec4(1.f,0.f,0.f,1.f);
  auto M = glm::mat4(1.f);
  v = M*v;

  float aspect = (float)width / (float)height;

  glm::mat4 proj = glm::mat4(1.f);
  auto cameraProj = glm::perspective(glm::half_pi<float>(),aspect,0.1f,1000.f);



  uint32_t triangleCounter = 0;
  bool running = true;

  float cameraYAngle = 0.f;
  float cameraXAngle = 0.f;
  float senstivity = 0.1f;
  float cameraDistance = 3.f;
  float zoomSpeed = 0.1f;

  while(running){ // main loop
    SDL_Event event;
    while(SDL_PollEvent(&event)){ // event loop
      if(event.type == SDL_QUIT)
        running = false;
      if(event.type == SDL_KEYDOWN){
        auto key = event.key.keysym.sym;
        if(key == SDLK_q)
          running = false;
        if(key == SDLK_p)
          usePhongShading = !usePhongShading;
        if(key == SDLK_i)lightPosition.z -= 0.1f;
        if(key == SDLK_k)lightPosition.z += 0.1f;
        if(key == SDLK_l)lightPosition.x += 0.1f;
        if(key == SDLK_j)lightPosition.x -= 0.1f;

      }
      if(event.type == SDL_MOUSEMOTION){
        if(event.motion.state & SDL_BUTTON_LMASK){
          cameraYAngle += event.motion.xrel*senstivity;
          cameraXAngle += event.motion.yrel*senstivity;
          cameraXAngle = glm::clamp(cameraXAngle,-glm::half_pi<float>()+.01f,+glm::half_pi<float>()-.01f);
        }
        if(event.motion.state & SDL_BUTTON_RMASK){
          cameraDistance += event.motion.yrel*zoomSpeed;
        }
      }
      if(event.type == SDL_MOUSEBUTTONDOWN){
        if(event.button.button == SDL_BUTTON_LEFT){
        }
        if(event.button.button == SDL_BUTTON_RIGHT){
        }
      }

    }

    // draw
 



    glClearColor(0.,0.,0.,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);

    glUseProgram(prg);


    auto cameraLocation = glm::vec3(
        glm::cos(cameraYAngle)*glm::cos(cameraXAngle),
        glm::sin(cameraXAngle),
        glm::sin(cameraYAngle)*glm::cos(cameraXAngle)) * cameraDistance;

    auto view = glm::lookAt(cameraLocation,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));

    angle += 0.f;

    auto lightView = glm::lookAt(lightPosition,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
    auto lightProj = glm::perspective(glm::half_pi<float>(),1.f,0.1f,10.f);

    view = lightView;
    proj = lightProj;
    // 1.pass - create shadow map 
    {
      glViewport(0,0,1024,1024);

      glBindFramebuffer(GL_FRAMEBUFFER,shadowFBO);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

      glUniformMatrix4fv(projL ,1,GL_FALSE,(float*)&proj );
      glUniformMatrix4fv(viewL ,1,GL_FALSE,(float*)&view );
      glUniform1i(usePhongShadingL,usePhongShading);

      auto T = glm::translate(glm::mat4(1.f),glm::vec3(0.f,0.f,0.f));
      auto S = glm::scale(glm::mat4(1.f),glm::vec3(1.f,1.f,1.f));
      auto R = glm::rotate(glm::mat4(1.f),glm::radians(angle*.2f),glm::vec3(0.f,1.f,0.f));
      auto model1 = T*R*S;
      glUniformMatrix4fv(modelL,1,GL_FALSE,(float*)&model1);
      glUniform3fv(lightPositionL,1,(float*)&lightPosition);
      glDrawElements(GL_TRIANGLES,sizeof(bunnyIndices)/sizeof(uint32_t),GL_UNSIGNED_INT,0);

      glBindFramebuffer(GL_FRAMEBUFFER,0);
    }

    view = glm::lookAt(cameraLocation,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
    proj = cameraProj;
    // 2.pass - rendering of the scene
    {
      glViewport(0,0,width,height);

      glUniformMatrix4fv(projL ,1,GL_FALSE,(float*)&proj );
      glUniformMatrix4fv(viewL ,1,GL_FALSE,(float*)&view );

      glUniformMatrix4fv(lightProjL ,1,GL_FALSE,(float*)&lightProj );
      glUniformMatrix4fv(lightViewL ,1,GL_FALSE,(float*)&lightView );

      glUniform1i(usePhongShadingL,usePhongShading);


      glUniform1i(shadowMapL,4);
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D,shadowMap);


      auto T = glm::translate(glm::mat4(1.f),glm::vec3(0.f,0.f,0.f));
      auto S = glm::scale(glm::mat4(1.f),glm::vec3(1.f,1.f,1.f));
      auto R = glm::rotate(glm::mat4(1.f),glm::radians(angle*.2f),glm::vec3(0.f,1.f,0.f));
      auto model1 = T*R*S;
      glUniformMatrix4fv(modelL,1,GL_FALSE,(float*)&model1);
      glUniform3fv(lightPositionL,1,(float*)&lightPosition);
      glDrawElements(GL_TRIANGLES,sizeof(bunnyIndices)/sizeof(uint32_t),GL_UNSIGNED_INT,0);
    }


    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,tex);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D,shadowMap);

    glUseProgram(texPrg);
    glUniform1i(texL,3);
    glUniform1i(texShadowMapL,4);
    glUniformMatrix4fv(texViewL ,1,GL_FALSE,(float*)&view );
    glUniformMatrix4fv(texProjL ,1,GL_FALSE,(float*)&proj );
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    // after rendering
    SDL_GL_SwapWindow(window);
  }

  //SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  return 0;
}
