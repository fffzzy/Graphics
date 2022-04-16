# mini-minecraft-560fp
mini-minecraft-560fp created by GitHub Classroom

MILESTONE 1:
Konstantinos Gkatzonis: Efficient Terrain Rendering and Chunking
After watching the lectures giving advice on implementing efficient chunking I designed the algorithm for rendering only the faces adjacent to empty blocks. The hard part
about this section was ensuring that blocks at the edges of a chunk correctly interracted with neighboring chunks during those checks. This required a bit of math and debugging.
The hardest part of my section was dynamically rendering only the 9 chunks surrounding the player. I ran into errors during spawn where the player spawned and the scene tried to
render before all the proper chunks had been generated. This was solved by ensuring that all the chunks whose close proximity the player is in are generated before the scene is
rendered. This ensures that the necessary chunks always exist in memory.

Charlie Herrmann: Procedural Terrain

To implement the noise function for the height of the mountain I used fractal brownian noise overtop of perlin noise. For the rolling hills I used Worley noise. To interpolate bewteen biomes I used Perlin noise with a very large grid size. When this perlin noise was above 0.5 that signified mountains and below signified rolling hills. In between 0.4 and 0.6 I interpolated with the glm::mix function to provide a smoother transition between the regions. 

To test the noise functions I created I modified my HW4 and using a shader, I created greyscale images representing my functions. One difficulty I encountered was converting between glsl and C++. For some reason I was not getting the same exact results with seemingly similar noise functions across the two languages. 

Zhouyang Fang: Game Engine
I used inputBundle to store all key inputs. The exception is right click and left click, which invoke addBlock() and removeBlock() directly. Flight mode has a seperate function to toggle directly else it would switch several times for single press. For collision detection, I dectect all 12 points in all 3 directions with gridMarch function. When the distance is shorter than the movement, shrink the movement. 

Our video link:
https://drive.google.com/file/d/1CJrhafz2W3ZNDQWmeDjgCuYKNflFSwAg/view?usp=sharing


MILESTONE 2:
Konstantinos Gkatzonis: Texturing and Texture Animation

Charlie Herrmann: Cave Systems

To implement the caves I created a new Perlin Noise function. This function took in a 3 vector instead of a 2 vector and used 8 surflets instead of 4. I used a decently large grid size to acheive this affect. I also created two new blocks, lava and bedrock. Lava is seen in pools at the bottom of caves and bedrock is seen at the very bottom layer of caves. The player will move more slowly when moving through either lava or water. The player will also sink in both of these. A good attempt was made to create a shader program for water and lava but I wasn't able to get it working. I will continue to work on this for milestone 3. 

Zhouyang Fang: Multithreading

After certain period, main thread would clear the far away region and load nearby region. If Terrain is already existed, it will spawn VBO workers to compute the interleaved buffer and index buffer data, else it would spawn BlockTypeWorkers to generation zone in certain radius. There are four separate vectors to hold opaque and transparent vertex and index data for Chunk VBOs. Currently there are still some bugs in tryExpansion() function which I plan to fix this weekend.

Videos:
Walking through water: https://drive.google.com/file/d/1GeBH4xUGa9WlqhQTTIA4RlClwVQxl_xu/view?usp=sharing
Caves: https://drive.google.com/file/d/1T2zAyBnavTi0_CYtk6KVBUwVr0ICXZHk/view?usp=sharing

