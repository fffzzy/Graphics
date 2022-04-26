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
To implement texturing I added replaced the color parameter in the VBOs with the UV coordinates of corresponding to each vertex. Then, in the createVBOData function I made sure to
replace the section where a different color is exported for each block type with a section that determines the UV offset on the texture map that corresponds to the current block type.
To render transparent components I separated the opaque and transparent VBOs into 2 different buffers and set up the interleaved drawing function so that it could be asked to draw either the primary
or the secondary buffered VBOs. First I drew all the primary VBOs from each chunk (the opaque objects) and then the secondsry ones (transparent). Finally, for the texture animation segment I exported
the time of the game to the shader as a uniform variable and shifted the UVs that corresponded to water and lava so that they appeared to be moving. I also implemented the mechanic for falling through water along with Charlie.

Charlie Herrmann: Cave Systems

To implement the caves I created a new Perlin Noise function. This function took in a 3 vector instead of a 2 vector and used 8 surflets instead of 4. I used a decently large grid size to acheive this affect. I also created two new blocks, lava and bedrock. Lava is seen in pools at the bottom of caves and bedrock is seen at the very bottom layer of caves. The player will move more slowly when moving through either lava or water. The player will also sink in both of these. A good attempt was made to create a shader program for water and lava but I wasn't able to get it working. I will continue to work on this for milestone 3. 

Zhouyang Fang: Multithreading

After certain period, main thread would clear the far away region and load nearby region. If Terrain is already existed, it will spawn VBO workers to compute the interleaved buffer and index buffer data, else it would spawn BlockTypeWorkers to generation zone in certain radius. There are four separate vectors to hold opaque and transparent vertex and index data for Chunk VBOs.

Videos:

Walking through water: https://drive.google.com/file/d/1GeBH4xUGa9WlqhQTTIA4RlClwVQxl_xu/view?usp=sharing

Caves: https://drive.google.com/file/d/1T2zAyBnavTi0_CYtk6KVBUwVr0ICXZHk/view?usp=sharing


MILESTONE 3:
Konstantinos Gkatzonis: Additional Biomes
I decided to create 2 new biomes for this milestone. A generally flat and dry desert biome with cactuses and a humid canion biome with mossy rocks and deep narrow caverns with water inside.
To do so, I created two noise maps, one for humidity and one for temperature, and proceeded to use those to distribute the different biomes. I used bilinear interpolation to smoothly transition between them,
something that took quite a lot of work with tuning the noise functions of the new biomes to get it working and looking right. Finally, I improved some of the incomplete segments of previous sections. More
specifically I improved the movement mechanics from Milestone 1 so that they worked more consistently and implemented block creation and destruction, something that broke when multithreading was added initially.
I also improved the previously existing biomes (mountains and grasslands) to look more natural.

I ran into some trouble with actually playing the game. Specifically, because our multithreading segment had many issues, the VBOs were not buffered appropriately and quickly enough to not be asked to render
before they were ready. This caused many OpenGL errors and required me to keep movement slow so that they all had time to generate and buffer.

Konstantinos Gkatzonis - Video (for all parts): https://drive.google.com/open?id=1OcQSeRKf67sCy1LHNfR8_t2GhYXrczXA&authuser=kgatz%40seas.upenn.edu&usp=drive_fs

Charlie Herrmann:

For this milestone I decided to create rivers. To do this I used L systems to define lines where I wanted my rivers. My L system was define as F -> F[+F][-F], where F is a forward line, + is a positive rotation of some angle and - is a negative rotation of some angle. I used random noise to vary the distance that the rivers ran as well as the angles of the branches. I also made it so the lower levels of recursion had thinner branches, meaning the base of the river was the widest. To carve out the river from the terrain I used a signed distance function of a round cone. If any blocks were within the defined cone, I made them water. Also all blocks above the river were set to empty. 

Final video Charlie: https://drive.google.com/file/d/1W-yibY-DIp8IvDC86eIRtKaiRTwCrjpm/view?usp=sharing

Zhouyang Fang: Day and night cycle (60 pts)

I create both sun and moon since they are only different in color. Worley noise is calculated every time frame to render the sky based on pixel space and camera position, making the game kinda laggy. Three thresholds are set to determine the sky color.
