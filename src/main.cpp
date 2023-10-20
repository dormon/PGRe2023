#include<iostream>

#include<SDL.h>

int main(int argc,char*argv[]){
  SDL_CreateWindow("PGRe2023",0,0,1024,768,0);

  bool running = true;
  while(running){ // main loop
    SDL_Event event;
    while(SDL_PollEvent(&event)){ // event loop
      if(event.type == SDL_QUIT)
        running = false;

    }
  }
  std::cerr << "Hello World!" << std::endl;
  return 0;
}
