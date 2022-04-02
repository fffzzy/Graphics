# mini-minecraft-560fp
mini-minecraft-560fp created by GitHub Classroom

Konstantinos Gkatzonis: Efficient Terrain Rendering and Chunking
After watching the lectures giving advice on implementing efficient chunking I designed the algorithm for rendering only the faces adjacent to empty blocks. The hard part
about this section was ensuring that blocks at the edges of a chunk correctly interracted with neighboring chunks during those checks. This required a bit of math and debugging.
The hardest part of my section was dynamically rendering only the 9 chunks surrounding the player. I ran into errors during spawn where the player spawned and the scene tried to
render before all the proper chunks had been generated. This was solved by ensuring that all the chunks whose close proximity the player is in are generated before the scene is
rendered. This ensures that the necessary chunks always exist in memory.

Charlie Herrmann: Procedural Terrain

Zhouyang Fang: Game Engine
