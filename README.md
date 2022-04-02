# mini-minecraft-560fp
mini-minecraft-560fp created by GitHub Classroom

Konstantinos Gkatzonis: Efficient Terrain Rendering and Chunking

Charlie Herrmann: Procedural Terrain

Zhouyang Fang: Game Engine
I used inputBundle to store all key inputs. The exception is right click and left click, which invoke addBlock() and removeBlock() directly. Flight mode has a seperate function to toggle directly else it would switch several times for single press. For collision detection, I dectect all 12 points in all 3 directions with gridMarch function. When the distance is shorter than the movement, shrink the movement. 