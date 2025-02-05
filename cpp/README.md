# VM-1 C++ Prototype

## Prerequisites

### Packages
```

sudo apt install cmake libavfilter-dev

```

### Compile SDL
```

# Clone SDL3 (into a directory of choice)
git clone https://github.com/libsdl-org/SDL.git

# Check out release
cd SDL
git checkout tags/release-3.2.2

# Configure cmake project
cmake -S . -B build

# Build
cmake --build build

# Install
sudo cmake --install build --prefix /usr/local

# Clone SDL3_image (into a directory of choice)
git clone https://github.com/libsdl-org/SDL_image.git
cd SDL_image
git checkout tags/release-3.2.0
cmake -S . -B build
cmake --build build
sudo cmake --install build --prefix /usr/local

```

### Clone VM-1 repository

```
# Clone VM-1
git clone https://github.com/zwodev/vm1-video-mixer.git

# Update the submodules
git submodule update --recursive

```

### Compiling
```
meson setup builddir
meson compile

```


 
