### IMPORTANT ###
If you are launching the program, make sure to set the NUM_CORES global variable in terrain.h to the number of cores in your CPU. 
The program will run super slowly if it is greater than the number of cores in your CPU.
### IMPORTANT ###


# MILESTONE 1 #

Efficient Rendering

I made extensive use of static objects for cleaner code. 

I used a static map which sends BlockTypes to vec3 colors for more efficient coloring. I used a static array of a struct called BlockFace, corresponding to the six neighboring faces of a block, allowing me to iterate over each face when populating my VBO. I used a similar static array to check for/create neighboring chunks in terrain generation. These static objects allow for both cleaner code and faster code. For example, we do not have to create a new variable for direction every time we check a block's neighboring faces.

The rest of my code involves the usual setting up of handles for buffers, sending data to GPU, triangulation for the index buffer, etc. I also enabled GL_CULL_FACE to avoid drawing faces not visible to the player, so that our program runs faster.

I had a little bit of trouble with the index buffer, but I soon realized my logic was incorrect and I was able to remedy this issue. I was incorrectly associating the number of vertices with the size of the index buffer. 

---

**Game Engine/Player Physics**

- game engine is used to process inputs and pass them to the player object using a switch case
	- inputs are later handled in `processInputs` of the player that applies acceleration
- movement is handled in `computePhysics`
	- gravity is calculated when not in flight mode
	- velocity is based on acceleration
	- movement is handled by taking raw velocity in fly mode and using `collideAndMove` in not fly mode
	- acceleration and velocity are decayed
- `collideAndMove` uses grid-marching to determine collisions along cardinal aixes then moves based on allowed distance
	- allowed distance calculated in grid march
	- additional checks happen here to determine if player in the air using if the player touched the ground in flight mode or left the ground in not flight mode
- `placeBlock` and `breakBlock` use grid marching to determine the block the player is looking at and break/place there
	- `breakBlock` has some additional checking based on ray distance to see which face on the block is clicked
	- the `BlockType` of the placed block is based on the number keys

Most implementations were pretty standard, not much to say about them.

---

Procedural Biome generation. The mountains are using worley noise raised to a slightly higher power to raise jaggedness. The Worley function uses 2nd min dist - 1st min dist as advised in the paper linked in the write up. The grasslands use a mix of perlin, worley, and fbm that are scaled to provide variety. The implementation uses a terrain gen class with static functions that are called by the chunk in the constructor to set block heights and type.

---

# MILESTONE 2 #

Texturing and Animation

I reworked the way Chunks are drawn so that they contain two DrawableChunks, one for the opaque blocks and one for the transparent blocks. This allowed me to not have to edit the Drawable class to contain transparent and opaque VBOs and just draw the opaque DrawableChunk followed by the transparent DrawableChunk. Additionally, Chunk contains much of the interface of a Drawable still to make it easier for my group mates. Additionally I use the ChunkVBOData to package data to send to multithreading. For UV and animation I added a new vec4 into the interleaved vbo data so to that i can hold the vec2 for UV coordinates and a float for if it is animated or not (it was just easier to push another vec4 than to mess with pushing different types to the vbo data). Also for animating I used the u_Time uniform variable to offset the uv cooridinate of water and lava every couple of seconds so that it becomes animated.

---

Daniel

The water and lava post process effects were made using the framebuffer class and following along with the implementation in homework 4. There is a post process shader pointer in MyGL that changes based on flags in the Player class for being in water/lava, and the post process shader changes accordingly to add the appropriate tint. The movement adjustments were made by doing a 0.66 multiplication by the velocity after collision detection, and the floating was done by removing the inAir flag if the Player is in water, so they essentially jump smoothly and infinitely while they are in liquid. The caves are a 3d Perlin Noise function with some smoothstep added so that when the y is higher, it "zips" up the caves to make them less frequent at higher y values.

---

Multithreading

Multithreading is implemented using C++'s std::thread and std::mutex. We create a thread for each core in the user's CPU (they will have to change this global variable themselves in terrain.h), which are stored in an array. Each thread does both the work for generating chunks and creating VBO data for chunks in the multithread_work() function. Each thread continuously looks for data in its respective input vector for chunks that need to be generated and chunks that need VBO data. When generating chunks, each thread pushes the chunk for VBO data creation to the next thread (modulo the number of cores). 

The main thread populates the input vectors for the threads by checking if we are in a new terrain zone every tick. It then adds NEW chunks to be generated, OLD chunks without VBO data to be populated with VBO data, and DESTROYS chunks far away.

---

# MILESTONE 3 #

Saving/Loading

Building off of the multithreading system, I built a system for saving and loading. Loading pulls from the saves directory where each save is its own directory containing a main save file with the player position, the world seed, and the set of all chunks that have been saved, the directory also conains a file for each saved chunk which is a serialization of the chunks blocktype data. Chunks are only saved if they are modified. saving and loading of chunks occurs in the mulithreaded terrain generation. Chunks that are known to have saved data (as given by the main save file) are loaded from disk otherwise they are generated like normal. additionally chunks will only now have their VBO data generated when all their neighbors are generated (so you dont ever make faces that are not nessicary depending on block type). at the end of the program all modified chunks are saved before the game closes and all the threads are joined (to avoid any segfaults). for saving and loading I also added some quality of life features like a "main menu" which allows you to create a new game with a specific seed or load an old game, then wait until all the terrain is loaded to start playing. this required remaking some of the multitrheading aspects so that the main thread knows when the worker threads are done their jobs.
Most of the challenges in this section came from visual erros that were coming up because of draw order as well as learning more about QT to make the main menu.
refrences: https://en.cppreference.com/w/cpp/thread, qt docs

---

GUI/"inventory"

I built an on screen overlay within the MyGL object to display a simple GUI made of a hotbar and a crosshair. the GUI is drawn after the post process passes so that it is drawn the same way always no mater the effect. The hot bar displays the current block the player is "holding" so that they know what they are placing. the crosshair adds a visual indicator to where exactly the player is looking.
The challenges I encountered while making this were just remembering all the opengl things i needed to do because i had spent so long in multithreading world before this part as well as remering how to make all the textures draw correctly with UVs
refrences: slides

---

Wolf

Custom AI and Skeleton Shader from MiniMaya project

---

Growing SDF Trees with L System randomization

Trees that grow based on L Systems with randomization and turtle using capsule SDF

---

Volcano Biome

Worley Noise volcano biomes using interpolation on height to add smooth blending


