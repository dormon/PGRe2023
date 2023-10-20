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

    }
  }

  SDL_DestroyWindow(window);

  return 0;
}
