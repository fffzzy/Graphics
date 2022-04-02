# mini-minecraft-560fp
mini-minecraft-560fp created by GitHub Classroom

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
