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

#define OPENGL_OLD

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

  out vec3 vColor;

  layout(location=0)in vec3 position;
  layout(location=1)in vec3 color;

  uniform mat4 proj  = mat4(1.f);
  uniform mat4 view  = mat4(1.f);
  uniform mat4 model = mat4(1.f);

  void main(){
    vColor = color;

    gl_Position = proj*view*model*vec4(position,1);

  }
  ).";

  auto fsSrc = R".(
  #version 410
  
  in vec3 vColor;

  out vec4 fColor;
  void main(){
    fColor = vec4(vColor,1);
    //fColor = vec4(0,0,0,1);
  }

  ).";


  GLuint vs  = createShader(GL_VERTEX_SHADER,vsSrc);
  GLuint fs  = createShader(GL_FRAGMENT_SHADER,fsSrc);
  GLuint prg = createProgram({vs,fs});

  GLuint angleL  = glGetUniformLocation(prg,"angle" );
  GLuint projL   = glGetUniformLocation(prg,"proj"  );
  GLuint viewL   = glGetUniformLocation(prg,"view"  );
  GLuint modelL  = glGetUniformLocation(prg,"model" );

  float angle = 0.f;


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
 



    glClearColor(0.3,0.3,0.3,1);
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


    auto T = glm::translate(glm::mat4(1.f),glm::vec3(1.f,0.f,0.f));
    auto S = glm::scale(glm::mat4(1.f),glm::vec3(1.f,2.f,1.f));
    auto R = glm::rotate(glm::mat4(1.f),glm::radians(angle),glm::vec3(0.f,1.f,0.f));

    auto model = T*R*S;


    glUniformMatrix4fv(projL ,1,GL_FALSE,(float*)&proj );
    glUniformMatrix4fv(viewL ,1,GL_FALSE,(float*)&view );
    glUniformMatrix4fv(modelL,1,GL_FALSE,(float*)&model);

    glDrawElements(GL_TRIANGLES,
        sizeof(bunnyIndices)/sizeof(uint32_t),
        GL_UNSIGNED_INT,0);

    // after rendering
    SDL_GL_SwapWindow(window);
  }

  //SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  return 0;
}
