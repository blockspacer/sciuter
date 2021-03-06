CXX=g++
CXX_FLAGS="-c -Wall -std=c++17 -I include"
LD_FLAGS="-lSDL2 -lSDL2_image"
SRC="src/main.cpp src/sdl.cpp src/animation.cpp src/systems.cpp src/resources.cpp"
OBJS="main.o sdl.o animation.o systems.o resources.o"

redo-ifchange $SRC
$CXX $CXX_FLAGS $SRC

redo-ifchange $OBJS
$CXX $LD_FLAGS $OBJS -o $3
