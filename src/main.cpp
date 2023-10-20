#include<iostream>

#include<SDL.h>

int main(int argc,char*argv[]){
  auto window = SDL_CreateWindow("PGRe2023",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1024,768,0);

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
        std::cerr << event.motion.xrel << std::endl;
        std::cerr << event.motion.yrel << std::endl;
      }
      if(event.type == SDL_MOUSEBUTTONDOWN){
        if(event.button.button == SDL_BUTTON_LEFT){
          std::cerr << "left" << std::endl;
        }
        if(event.button.button == SDL_BUTTON_RIGHT){
          std::cerr << "right" << std::endl;
        }
      }

    }
  }

  SDL_DestroyWindow(window);

  return 0;
}
