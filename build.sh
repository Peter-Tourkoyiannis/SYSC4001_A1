if [ ! -d "bin" ]; then
    mkdir bin
else
    rm -f bin/*
fi
  g++ -std=c++17 -g -O0 -I . -o bin/interrupts interrupts.cpp
  echo "Build complete: bin/interrupts created"