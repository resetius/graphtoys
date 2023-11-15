### Introduction
Welcome to the GraphToys project! This repository contains a collection of various graphical models and simulations, showcasing different aspects of computer graphics and simulation techniques.

### Building the Project
You can build the project using either `make` or `CMake`. Both options are supported.

#### Prerequisites
- Ensure you have either `make` or `CMake` installed on your system.
- Git should be installed for managing submodules.
- vulkan headers, vulkan libs, glslc, glfw3, freetype2

#### Steps to Build
1. **Clone the Repository**: First, clone this repository to your local machine using:
   ```
   git clone https://github.com/resetius/graphtoys
   ```
2. **Initialize and Update Git Submodules**: Before building, initialize and download the necessary git submodules:
   ```
   git submodule init
   git submodule update
   ```
3. **Build the Project**:
   - Using `make`:
     ```
     make
     ```
   - Using `CMake`:
     ```
     cmake .
     make
     ```

### Running the Application
After building, you can run the application using the following command:
```
./main.exe model_name --cfg config-file
```
Replace `model_name` with the name of the model you want to run, and `config-file` with the path to your configuration file if needed.

### Models
Here are the models you can explore in this project:

1. **torus**: A simple rotating torus model.
2. **triangle**: A basic triangle shape.
3. **mandelbrot**: Visualization of the Mandelbrot set.
4. **mandelbulb**: A 3D analog of the Mandelbrot set.
5. **stl**: Loads a model in STL format from a file.
6. **particles**: A particle model with calculations based on Newton's laws.
7. **particles2**: A particle model using only vertex shaders for calculations.
8. **particles3**: Particle model algorithms using PM and P3M.
9. **poisson**: Solves the Poisson equation (no visualization).
10. **gltf**: Loads a model in GLTF format.
11. **test**: A simple test with rotating figures.

## Configuration File Format for GraphToys Project

The configuration file for the GraphToys project uses the INI format, which is a simple, standard format for configuration files. The file is divided into sections, each marked by a header in square brackets (`[]`). Here's a breakdown of the sections and parameters you can set:

### `[render]` Section
This section is used to configure rendering settings.

- **api**: Set the graphics API to be used. You can choose between `vulkan` and `opengl`.
   ```
   api = vulkan
   ```
   or
   ```
   api = opengl
   ```

- **width**: Define the width of the rendering window in pixels.
   ```
   width = 800
   ```

- **height**: Define the height of the rendering window in pixels.
   ```
   height = 600
   ```

- **vsync**: Set this to `on` or `off` to enable or disable vertical synchronization.
   ```
   vsync = on
   ```
   or
   ```
   vsync = off
   ```

### `[model_name]` Sections
Each model in the GraphToys project has its own dedicated section in the configuration file. The section name corresponds to the name of the model (for example, `[torus]`, `[triangle]`, etc.).

In these sections, you can specify various parameters that are unique to each model. It's recommended to refer to the source code in `models/model_name.c` for detailed information on what parameters can be set for each specific model.

For example, if you're configuring the `torus` model, you would look for parameters in the `models/torus.c` file and set them in the `[torus]` section of your configuration file.

### Example Configuration File
Here's an example of what a configuration file might look like:

```
[render]
api = vulkan
width = 1024
height = 768
vsync = off

[torus]
; torus-specific settings

[triangle]
; triangle-specific settings

[mandelbrot]
; mandelbrot-specific settings
```

### Notes
- Ensure the format and syntax are correctly followed to avoid any parsing errors.
- Since the parameters for each model can vary significantly, it's crucial to consult the source files for the correct options and their expected values.

# Screenshots

![Galaxies](/screenshots/galaxies.png?raw=true)

![Particles](/screenshots/particles.png?raw=true)

![Mandelbrot](/screenshots/mandelbrot.png?raw=true)

![Mandelbulb](/screenshots/mandelbulb.png?raw=true)

![Skull](/screenshots/skull.png?raw=true)

![Test](/screenshots/test.png?raw=true)
