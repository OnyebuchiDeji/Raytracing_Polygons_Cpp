##### 13-07-2026
##### Author: Ebenezer Ayo, Onyebuchi

# Raytracing Polygons - C++ + OpenGL Adaptation

+ The C++ adaptation of the Raytracing Polygons Pypy Project

+	Demonstrates rendering of polygons using raytracing and the optimization using a Bounding Volumne Hierarchy structure, BVH.

+	The use of a Bounding Volumne Hierarchy significantly improves performance by drastically reducing the number of ray-triangle misses during the calculation for intersection between the ray and the triangles of the Polygon Mesh.
	-	This is possible as the BVH structure pre-partitions the rendering space into equal parts recursively. 
	-	When a ray is fired, what outer partitions it hits is tested, and then is tested against the subdivided partitions within that outer partition, and so on. Therefore the ray intersection is only for the outer most, reducing checking the whole screen space everytime. If it doesn't fall within the first outer bounding partition, it goes to the next, skipping that whole screen search space early. This is what reduces the overhead.
	-	Beforehand to the BVH traversal, it utilizes the 3D DDA method to determine the starting distance along the ray as well as the furthest.

### Github Repo:
[`Git Repo`](https://github.com/OnyebuchiDeji/Raytracing_Polygons_Cpp)


###	Key Features

+	GPU-accelerated rendering using the rendering pipeline.
+	SDF raytracing for visualising Polygon Meshes
+	Performance improvement using Bounding Volume Hierarchy structure.
+	Use of structures like SSBOs (Shader Storage Buffer Objects) to send the Polygon triangle data to GPU.

###	Tech Stack

+	C++, Modern Opengl, GLM, GLFW, GLAD.


### References

For Inspiration and Understanding: 

+ The Cherno (YouTube)
+ GetIntoGameDev (YouTube)


---


### Setup Instructions
> Install CMake

+ To Run, type any of these:
	1. Full build (first run):
		`proj_build --full`
	2. Partial build (after changes):
		`proj_build --partial`
	3. Clean build (if needed):
		`proj_build --clean`


---

###	Architecture Diagram

Raytracing_Polygons_Cpp/
 ├── vendor/
 │	└── stb_image.h
 ├── test.sh
 ├── src/
 │	├── shaders/
 │	├── shader2-compute.glsl
 │	├── shader1.frag
 │	├── shader1-tri_quad.vert
 │	├── shader1-ssbos_bvh.frag
 │	├── shader1-ssbos.frag
 │	├── shader1-rect_quad.vert
 │	├── shader1-2dtexture_buffer.frag
 │	└── shader1-1dtexture_buffer.frag
 │	├── shader.h
 │	├── shader.cpp
 │	├── renderer.h
 │	├── renderer.cpp
 │	├── program.h
 │	├── program.cpp
 │	├── model.h
 │	├── model.cpp
 │	├── main.cpp
 │	└── bvh.h
 ├── README.md
 ├── proj_build_v1.sh
 ├── proj_build_v1.bat
 ├── proj_build.sh
 ├── proj_build.bat
 ├── models/
 │	├── wall.obj
 │	├── tank.obj
 │	├── ground.obj
 │	├── deino.obj
 │	├── cube.obj
 │	└── baryonx.obj
 ├── gb_info.html
 ├── CMakeLists.txt
 ├── .pddignore
 └── .gitignore


###	Screenshots

![image0](./_scrnshots/scrnshot0.png)
![image1](./_scrnshots/scrnshot1.png)
![image2](./_scrnshots/scrnshot2.png)
![image3](./_scrnshots/scrnshot3.png)
![image4](./_scrnshots/scrnshot4.png)
![image5](./_scrnshots/scrnshot5.png)
![image6](./_scrnshots/scrnshot6.png)
![image7](./_scrnshots/scrnshot7.png)
![image8](./_scrnshots/scrnshot8.png)
![image9](./_scrnshots/scrnshot9.png)
![image10](./_scrnshots/scrnshot10.png)
![image11](./_scrnshots/scrnshot11.png)
![image12](./_scrnshots/scrnshot12.png)
![image13](./_scrnshots/scrnshot13.png)
![image14](./_scrnshots/scrnshot14.png)
![image15](./_scrnshots/scrnshot15.png)
![image16](./_scrnshots/scrnshot16.png)
![image17](./_scrnshots/scrnshot17.png)
![image18](./_scrnshots/scrnshot18.png)
![image19](./_scrnshots/scrnshot19.png)
![image20](./_scrnshots/scrnshot20.png)
![image21](./_scrnshots/scrnshot21.png)
![image22](./_scrnshots/scrnshot22.png)
![image23](./_scrnshots/scrnshot23.png)
![image24](./_scrnshots/scrnshot24.png)
![image25](./_scrnshots/scrnshot25.png)
![image26](./_scrnshots/scrnshot26.png)
![image27](./_scrnshots/scrnshot27.png)


---