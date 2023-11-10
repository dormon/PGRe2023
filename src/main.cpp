#include "geGL/DebugMessage.h"
#include "geGL/OpenGL.h"
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

  out vec3 vNormal;

  out vec3 vPosition;

  out vec3 vLambert;
  out vec3 vPhong  ;

  void main(){
    vNormal = normal;

    vPosition = vec3(model*vec4(position,1));
    vNormal   = vec3(transpose(inverse(model))*vec4(normal,0));

    vec3 lightPosition        = vec3(10,10,10);
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

  }
  ).";


  auto fsSrc = R".(
  #version 410
  
  in vec3 vNormal;
  in vec3 vPosition;

  in vec3 vLambert;
  in vec3 vPhong  ;

  uniform int usePhongShading = 0;
  uniform mat4 view  = mat4(1.f);

  out vec4 fColor;
  void main(){

    vec3 lightPosition        = vec3(10,10,10);
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


    vec3 lambert = ambient + diffuse;
    vec3 phong = ambient + diffuse + specular;


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

  out vec4 fColor;
  void main(){
    //fColor = vec4(vTexCoord,0,1);
    fColor = texture(myTex,vTexCoord);
  }

  ).";


  GLuint vs  = createShader(GL_VERTEX_SHADER,vsSrc);
  GLuint fs  = createShader(GL_FRAGMENT_SHADER,fsSrc);
  GLuint prg = createProgram({vs,fs});


  GLuint angleL           = glGetUniformLocation(prg,"angle"          );
  GLuint projL            = glGetUniformLocation(prg,"proj"           );
  GLuint viewL            = glGetUniformLocation(prg,"view"           );
  GLuint modelL           = glGetUniformLocation(prg,"model"          );
  GLuint usePhongShadingL = glGetUniformLocation(prg,"usePhongShading");

  GLuint texVs  = createShader(GL_VERTEX_SHADER,texVsSrc);
  GLuint texFs  = createShader(GL_FRAGMENT_SHADER,texFsSrc);
  GLuint texPrg = createProgram({texVs,texFs});
  GLuint texProjL            = glGetUniformLocation(texPrg,"proj"           );
  GLuint texViewL            = glGetUniformLocation(texPrg,"view"           );
  GLuint texL                = glGetUniformLocation(texPrg,"myTex"          );


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


  float angle = 0.f;
  int usePhongShading = 0;


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
  auto proj = glm::perspective(glm::half_pi<float>(),aspect,0.1f,1000.f);



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

    angle += 1.f;

    glUniformMatrix4fv(projL ,1,GL_FALSE,(float*)&proj );
    glUniformMatrix4fv(viewL ,1,GL_FALSE,(float*)&view );
    glUniform1i(usePhongShadingL,usePhongShading);


    auto T = glm::translate(glm::mat4(1.f),glm::vec3(0.f,0.f,0.f));
    auto S = glm::scale(glm::mat4(1.f),glm::vec3(1.f,1.f,1.f));
    auto R = glm::rotate(glm::mat4(1.f),glm::radians(angle*.2f),glm::vec3(0.f,1.f,0.f));
    auto model1 = T*R*S;
    glUniformMatrix4fv(modelL,1,GL_FALSE,(float*)&model1);
    glDrawElements(GL_TRIANGLES,sizeof(bunnyIndices)/sizeof(uint32_t),GL_UNSIGNED_INT,0);


    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,tex);
    glUseProgram(texPrg);
    glUniform1i(texL,3);
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
