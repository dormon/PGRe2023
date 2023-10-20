#include<iostream>

#include<SDL.h>

int main(int argc,char*argv[]){
  auto window = SDL_CreateWindow("PGRe2023",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1024,768,SDL_WINDOW_OPENGL);

  auto context = SDL_GL_CreateContext(window);

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
 
    using GLCLEARTYPE = void(*)(int);
    using GLCLEARCOLORTYPE = void(*)(float,float,float,float);

    GLCLEARTYPE      glClear      = nullptr;
    GLCLEARCOLORTYPE glClearColor = nullptr;
#define GL_COLOR_BUFFER_BIT			0x00004000


    glClear      = (GLCLEARTYPE     )SDL_GL_GetProcAddress("glClear"     );
    glClearColor = (GLCLEARCOLORTYPE)SDL_GL_GetProcAddress("glClearColor");

    glClearColor(0,.5,0,1);
    glClear(GL_COLOR_BUFFER_BIT);


    // after rendering
    SDL_GL_SwapWindow(window);
  }

  //SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  return 0;
}
