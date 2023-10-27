#include "geGL/DebugMessage.h"
#include "geGL/OpenGL.h"
#include<iostream>

#include<SDL.h>
#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>

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
}

int main(int argc,char*argv[]){

  auto window = SDL_CreateWindow("PGRe2023",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1024,768,SDL_WINDOW_OPENGL);

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

  layout(location=0)in vec2 position;
  layout(location=1)in vec3 color;

  void main(){
    vColor = color;
    gl_Position = vec4(position,0,1);

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


  GLuint vs = createShader(GL_VERTEX_SHADER,vsSrc);
  GLuint fs = createShader(GL_FRAGMENT_SHADER,fsSrc);
  GLuint prg = createProgram({vs,fs});


  float const vertices[] = {
    0,0,1,0,0,
    1,0,0,1,0,
    0,1,0,0,1,
    1,1,1,1,0,
  };

  GLuint vbo;
  glCreateBuffers(1,&vbo);
  glNamedBufferData(vbo,sizeof(vertices),vertices,GL_DYNAMIC_DRAW);

  uint32_t const elements[] = {
    0,1,2,
    2,1,3,
  };

  GLuint ebo;
  glCreateBuffers(1,&ebo);
  glNamedBufferData(ebo,sizeof(elements),elements,GL_DYNAMIC_DRAW);


  GLuint vao;
  glCreateVertexArrays(1,&vao);
  glVertexArrayElementBuffer(vao,ebo);
  addAttrib(vao,vbo,0,2,GL_FLOAT,sizeof(float)*0,sizeof(float)*5);
  addAttrib(vao,vbo,1,3,GL_FLOAT,sizeof(float)*2,sizeof(float)*5);


  bool running = true;
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
    glClear(GL_COLOR_BUFFER_BIT);

  
    glBindVertexArray(vao);

    glUseProgram(prg);

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    // after rendering
    SDL_GL_SwapWindow(window);
  }

  //SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  return 0;
}
