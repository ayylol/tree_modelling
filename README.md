# Using Implicit Line Primitives to Create Tree Geometry

![cover](tree.png)
Cover generated using this program, and rendered in Blender.

This program takes in a volume-less 3d tree skeleton, along with a set of 
parameters to create an implicit field representing the tree. The extracted
iso-surface of the tree mesh will automatically have smoothly blended junctions,
and appropriate thickness of branches.

This is done by running a series of implicit poly-line primitives along the
skeleton. These primitives collide and blend with one another, thus building
up the thickness of the branches lower on the tree.

## Building And Running
This Program has only been tested on Linux. imagemagick is required for
screenshotting

**To Build:** create a `build` folder, and run `cmake ..` and `make` while 
inside the `build` folder.

**To Run:** execute `./tree_strands <params.json>`. You can find a set of
pretested `params.json` files in `resources/trees`. If you would like to
tweak the options, you can do so directly through a text editor, or using 
the [`tree_panel` program](https://github.com/ayylol/tree_panel)

## Usage
Once the program completes generating the tree you can look at it by moving
the camera with the mouse or keyboard.

- WASD/M-mouse    -> Panning the camera
- IJKL/L-mouse    -> Rotating the camera
- UO/Scroll Wheel -> Zoom in/out
- R               -> Reset Camera

You can export the mesh or take screenshots.

- \        -> export mesh
- <Enter>  -> take screenshot

You Can also toggle the various display modes by using the number keys.
- 1: The tree model
- 2: The texture strands that add texture to the isosurface
- 3: The strands that define the implicit line primitives
- 4: Visualization for strand extension steps
- 5: The input tree skeleton

There are more controls, I can't be bothered to write them down rn so its a todo 
