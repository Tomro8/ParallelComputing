
/* TIE-51257 Parallelization Excercise 2019
   Copyright (c) 2016 Matias Koskela matias.koskela@tut.fi
					  Heikki Kultala heikki.kultala@tut.fi

VERSION 1.1 - updated to not have stuck satellites so easily
VERSION 1.2 - updated to not have stuck satellites hopefully at all.
VERSION 19.0 - make all satelites affect the color with weighted average.
			   add physic correctness check.
*/

// Example compilation on linux
// no optimization:   gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm
// most optimizations: gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm -O2
// +vectorization +vectorize-infos: gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm -O2 -ftree-vectorize -fopt-info-vec
// +math relaxation:  gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm -O2 -ftree-vectorize -fopt-info-vec -ffast-math
// prev and OpenMP:   gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm -O2 -ftree-vectorize -fopt-info-vec -ffast-math -fopenmp
// prev and OpenCL:   gcc -o parallel parallel.c -std=c99 -lglut -lGL -lm -O2 -ftree-vectorize -fopt-info-vec -ffast-math -fopenmp -lOpenCL

// Example compilation on macos X
// no optimization:   gcc -o parallel parallel.c -std=c99 -framework GLUT -framework OpenGL
// most optimization: gcc -o parallel parallel.c -std=c99 -framework GLUT -framework OpenGL -O3



#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h> // printf
#include <math.h> // INFINITY
#include <stdlib.h>
#include <string.h>

// Window handling includes
#ifndef __APPLE__
#include <GL/GL.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
// These are used to decide the window size
#define WINDOW_HEIGHT 1024
#define WINDOW_WIDTH  1024

// The number of satelites can be changed to see how it affects performance.
// Benchmarks must be run with the original number of satellites
#define SATELITE_COUNT 64

// These are used to control the satelite movement
#define SATELITE_RADIUS 3.16f
#define MAX_VELOCITY 0.1f
#define GRAVITY 1.0f
#define DELTATIME 32
#define PHYSICSUPDATESPERFRAME 100000

// Some helpers to window size variables
#define SIZE WINDOW_HEIGHT*WINDOW_HEIGHT
#define HORIZONTAL_CENTER (WINDOW_WIDTH / 2)
#define VERTICAL_CENTER (WINDOW_HEIGHT / 2)

// Is used to find out frame times
int previousFrameTimeSinceStart = 0;
int previousFinishTime = 0;
unsigned int frameNumber = 0;
unsigned int seed = 0;

// Stores 2D data like the coordinates
typedef struct {
	float x;
	float y;
} floatvector;

// Stores 2D data like the coordinates
typedef struct {
	double x;
	double y;
} doublevector;

// Stores rendered colors. Each float may vary from 0.0f ... 1.0f
typedef struct {
	float red;
	float green;
	float blue;
} color;

// Stores the satelite data, which fly around black hole in the space
typedef struct {
	color identifier;
	floatvector position;
	floatvector velocity;
} satelite;

// Pixel buffer which is rendered to the screen
color* pixels;

// Pixel buffer which is used for error checking
color* correctPixels;

// Buffer for all satelites in the space
satelite* satelites;
satelite* backupSatelites;








// ## You may add your own variables here ##




// ## You may add your own initialization routines here ##
void init() {


}

// ## You are asked to make this code parallel ##
// Physics engine loop. (This is called once a frame before graphics engine) 
// Moves the satelites based on gravity
// This is done multiple times in a frame because the Euler integration 
// is not accurate enough to be done only once
void parallelPhysicsEngine() {



	// double precision required for accumulation inside this routine,
	// but float storage is ok outside these loops.
	doublevector tmpPosition[SATELITE_COUNT];
	doublevector tmpVelocity[SATELITE_COUNT];

	for (int i = 0; i < SATELITE_COUNT; ++i) {
		tmpPosition[i].x = satelites[i].position.x;
		tmpPosition[i].y = satelites[i].position.y;
		tmpVelocity[i].x = satelites[i].velocity.x;
		tmpVelocity[i].y = satelites[i].velocity.y;
	}

	// Physics iteration loop
	for (int physicsUpdateIndex = 0;
		physicsUpdateIndex < PHYSICSUPDATESPERFRAME;
		++physicsUpdateIndex) {

		// Physics satelite loop
		for (int i = 0; i < SATELITE_COUNT; ++i) {

			// Distance to the blackhole (bit ugly code because C-struct cannot have member functions)
			doublevector positionToBlackHole = { .x = tmpPosition[i].x -
			   HORIZONTAL_CENTER,.y = tmpPosition[i].y - VERTICAL_CENTER };
			double distToBlackHoleSquared =
				positionToBlackHole.x * positionToBlackHole.x +
				positionToBlackHole.y * positionToBlackHole.y;
			double distToBlackHole = sqrt(distToBlackHoleSquared);

			// Gravity force
			doublevector normalizedDirection = {
			   .x = positionToBlackHole.x / distToBlackHole,
			   .y = positionToBlackHole.y / distToBlackHole };
			double accumulation = GRAVITY / distToBlackHoleSquared;

			// Delta time is used to make velocity same despite different FPS
			// Update velocity based on force
			tmpVelocity[i].x -= accumulation * normalizedDirection.x *
				DELTATIME / PHYSICSUPDATESPERFRAME;
			tmpVelocity[i].y -= accumulation * normalizedDirection.y *
				DELTATIME / PHYSICSUPDATESPERFRAME;

			// Update position based on velocity
			tmpPosition[i].x +=
				tmpVelocity[i].x * DELTATIME / PHYSICSUPDATESPERFRAME;
			tmpPosition[i].y +=
				tmpVelocity[i].y * DELTATIME / PHYSICSUPDATESPERFRAME;
		}
	}

	// double precision required for accumulation inside this routine,
	// but float storage is ok outside these loops.
	// copy back the float storage.
	for (int i = 0; i < SATELITE_COUNT; ++i) {
		satelites[i].position.x = tmpPosition[i].x;
		satelites[i].position.y = tmpPosition[i].y;
		satelites[i].velocity.x = tmpVelocity[i].x;
		satelites[i].velocity.y = tmpVelocity[i].y;
	}

}

// ## You are asked to make this code parallel ##
// Rendering loop (This is called once a frame after physics engine) 
// Decides the color for each pixel.
void parallelGraphicsEngine() {

	// Graphics pixel loop
	for (int i = 0; i < SIZE; ++i) {

		// Row wise ordering
		floatvector pixel = { .x = i % WINDOW_WIDTH,.y = i / WINDOW_WIDTH };

		// This color is used for coloring the pixel
		color renderColor = { .red = 0.f,.green = 0.f,.blue = 0.f };

		// Find closest satelite
		float shortestDistance = INFINITY;

		float weights = 0.f;
		int hitsSatellite = 0;

		// First Graphics satelite loop: Find the closest satellite.
		for (int j = 0; j < SATELITE_COUNT; ++j) {
			floatvector difference = { .x = pixel.x - satelites[j].position.x,
									  .y = pixel.y - satelites[j].position.y };
			float distance = sqrt(difference.x * difference.x +
				difference.y * difference.y);

			if (distance < SATELITE_RADIUS) {
				renderColor.red = 1.0f;
				renderColor.green = 1.0f;
				renderColor.blue = 1.0f;
				hitsSatellite = 1;
				break;
			}
			else {
				float weight = 1.0f / (distance * distance * distance * distance);
				weights += weight;
				if (distance < shortestDistance) {
					shortestDistance = distance;
					renderColor = satelites[j].identifier;
				}
			}
		}

		// Second graphics loop: Calculate the color based on distance to every satelite.
		if (!hitsSatellite) {
			for (int j = 0; j < SATELITE_COUNT; ++j) {
				floatvector difference = { .x = pixel.x - satelites[j].position.x,
										  .y = pixel.y - satelites[j].position.y };
				float dist2 = (difference.x * difference.x +
					difference.y * difference.y);
				float weight = 1.0f / (dist2 * dist2);

				renderColor.red += (satelites[j].identifier.red *
					weight / weights) * 3.0f;

				renderColor.green += (satelites[j].identifier.green *
					weight / weights) * 3.0f;

				renderColor.blue += (satelites[j].identifier.blue *
					weight / weights) * 3.0f;
			}
		}
		pixels[i] = renderColor;
	}
}

// ## You may add your own destrcution routines here ##
void destroy() {


}







////////////////////////////////////////////////
// い TO NOT EDIT ANYTHING AFTER THIS LINE い //
////////////////////////////////////////////////

// い DO NOT EDIT THIS FUNCTION い
// Sequential rendering loop used for finding errors
void sequentialGraphicsEngine() {

	// Graphics pixel loop
	for (int i = 0; i < SIZE; ++i) {

		// Row wise ordering
		floatvector pixel = { .x = i % WINDOW_WIDTH,.y = i / WINDOW_WIDTH };

		// This color is used for coloring the pixel
		color renderColor = { .red = 0.f,.green = 0.f,.blue = 0.f };

		// Find closest satelite
		float shortestDistance = INFINITY;

		float weights = 0.f;
		int hitsSatellite = 0;

		// First Graphics satelite loop: Find the closest satellite.
		for (int j = 0; j < SATELITE_COUNT; ++j) {
			floatvector difference = { .x = pixel.x - satelites[j].position.x,
									  .y = pixel.y - satelites[j].position.y };
			float distance = sqrt(difference.x * difference.x +
				difference.y * difference.y);

			if (distance < SATELITE_RADIUS) {
				renderColor.red = 1.0f;
				renderColor.green = 1.0f;
				renderColor.blue = 1.0f;
				hitsSatellite = 1;
				break;
			}
			else {
				float weight = 1.0f / (distance * distance * distance * distance);
				weights += weight;
				if (distance < shortestDistance) {
					shortestDistance = distance;
					renderColor = satelites[j].identifier;
				}
			}
		}

		// Second graphics loop: Calculate the color based on distance to every satelite.
		if (!hitsSatellite) {
			for (int j = 0; j < SATELITE_COUNT; ++j) {
				floatvector difference = { .x = pixel.x - satelites[j].position.x,
										  .y = pixel.y - satelites[j].position.y };
				float dist2 = (difference.x * difference.x +
					difference.y * difference.y);
				float weight = 1.0f / (dist2 * dist2);

				renderColor.red += (satelites[j].identifier.red *
					weight / weights) * 3.0f;

				renderColor.green += (satelites[j].identifier.green *
					weight / weights) * 3.0f;

				renderColor.blue += (satelites[j].identifier.blue *
					weight / weights) * 3.0f;
			}
		}
		correctPixels[i] = renderColor;
	}
}

void sequentialPhysicsEngine(satelite* s) {

	// double precision required for accumulation inside this routine,
	// but float storage is ok outside these loops.
	doublevector tmpPosition[SATELITE_COUNT];
	doublevector tmpVelocity[SATELITE_COUNT];

	for (int i = 0; i < SATELITE_COUNT; ++i) {
		tmpPosition[i].x = s[i].position.x;
		tmpPosition[i].y = s[i].position.y;
		tmpVelocity[i].x = s[i].velocity.x;
		tmpVelocity[i].y = s[i].velocity.y;
	}

	// Physics iteration loop
	for (int physicsUpdateIndex = 0;
		physicsUpdateIndex < PHYSICSUPDATESPERFRAME;
		++physicsUpdateIndex) {

		// Physics satelite loop
		for (int i = 0; i < SATELITE_COUNT; ++i) {

			// Distance to the blackhole
			// (bit ugly code because C-struct cannot have member functions)
			doublevector positionToBlackHole = { .x = tmpPosition[i].x -
			   HORIZONTAL_CENTER,.y = tmpPosition[i].y - VERTICAL_CENTER };
			double distToBlackHoleSquared =
				positionToBlackHole.x * positionToBlackHole.x +
				positionToBlackHole.y * positionToBlackHole.y;
			double distToBlackHole = sqrt(distToBlackHoleSquared);

			// Gravity force
			doublevector normalizedDirection = {
			   .x = positionToBlackHole.x / distToBlackHole,
			   .y = positionToBlackHole.y / distToBlackHole };
			double accumulation = GRAVITY / distToBlackHoleSquared;

			// Delta time is used to make velocity same despite different FPS
			// Update velocity based on force
			tmpVelocity[i].x -= accumulation * normalizedDirection.x *
				DELTATIME / PHYSICSUPDATESPERFRAME;
			tmpVelocity[i].y -= accumulation * normalizedDirection.y *
				DELTATIME / PHYSICSUPDATESPERFRAME;

			// Update position based on velocity
			tmpPosition[i].x +=
				tmpVelocity[i].x * DELTATIME / PHYSICSUPDATESPERFRAME;
			tmpPosition[i].y +=
				tmpVelocity[i].y * DELTATIME / PHYSICSUPDATESPERFRAME;
		}
	}

	// double precision required for accumulation inside this routine,
	// but float storage is ok outside these loops.
	// copy back the float storage.
	for (int i = 0; i < SATELITE_COUNT; ++i) {
		s[i].position.x = tmpPosition[i].x;
		s[i].position.y = tmpPosition[i].y;
		s[i].velocity.x = tmpVelocity[i].x;
		s[i].velocity.y = tmpVelocity[i].y;
	}
}

// い DO NOT EDIT THIS FUNCTION い
void errorCheck() {
	for (int i = 0; i < SIZE; ++i) {
		if (correctPixels[i].red != pixels[i].red ||
			correctPixels[i].green != pixels[i].green ||
			correctPixels[i].blue != pixels[i].blue) {

			printf("Buggy pixel at (x=%i, y=%i). Press enter to continue.\n", i % WINDOW_WIDTH, i / WINDOW_WIDTH);
			getchar();
			return;
		}
	}
	printf("Error check passed!\n");
}

// い DO NOT EDIT THIS FUNCTION い
void compute(void) {
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	previousFrameTimeSinceStart = timeSinceStart;

	// Error check during first frames
	if (frameNumber < 2) {
		memcpy(backupSatelites, satelites, sizeof(satelite) * SATELITE_COUNT);
		sequentialPhysicsEngine(backupSatelites);
	}
	parallelPhysicsEngine();
	if (frameNumber < 2) {
		for (int i = 0; i < SATELITE_COUNT; i++) {
			if (memcmp(&satelites[i], &backupSatelites[i], sizeof(satelite))) {
				printf("Incorrect satelite data of satelite: %d\n", i);
				getchar();
			}
		}
	}

	int sateliteMovementMoment = glutGet(GLUT_ELAPSED_TIME);
	int sateliteMovementTime = sateliteMovementMoment - timeSinceStart;

	// Decides the colors for the pixels
	parallelGraphicsEngine();

	int pixelColoringMoment = glutGet(GLUT_ELAPSED_TIME);
	int pixelColoringTime = pixelColoringMoment - sateliteMovementMoment;

	// Sequential code is used to check possible errors in the parallel version
	if (frameNumber < 2) {
		sequentialGraphicsEngine();
		errorCheck();
	}

	int finishTime = glutGet(GLUT_ELAPSED_TIME);
	// Print timings
	int totalTime = finishTime - previousFinishTime;
	previousFinishTime = finishTime;

	printf("Total frametime: %ims, satelite moving: %ims, space coloring: %ims.\n",
		totalTime, sateliteMovementTime, pixelColoringTime);

	// Render the frame
	glutPostRedisplay();
}

// い DO NOT EDIT THIS FUNCTION い
// Probably not the best random number generator
float randomNumber(float min, float max) {
	return (rand() * (max - min) / RAND_MAX) + min;
}

// DO NOT EDIT THIS FUNCTION
void fixedInit(unsigned int seed) {

	if (seed != 0) {
		srand(seed);
	}

	// Init pixel buffer which is rendered to the widow
	pixels = (color*)malloc(sizeof(color) * SIZE);

	// Init pixel buffer which is used for error checking
	correctPixels = (color*)malloc(sizeof(color) * SIZE);

	backupSatelites = (satelite*)malloc(sizeof(satelite) * SATELITE_COUNT);


	// Init satelites buffer which are moving in the space
	satelites = (satelite*)malloc(sizeof(satelite) * SATELITE_COUNT);

	// Create random satelites
	for (int i = 0; i < SATELITE_COUNT; ++i) {

		// Random reddish color
		color id = { .red = randomNumber(0.f, 0.15f) + 0.1f,
					.green = randomNumber(0.f, 0.14f) + 0.0f,
					.blue = randomNumber(0.f, 0.16f) + 0.0f };

		// Random position with margins to borders
		floatvector initialPosition = { .x = HORIZONTAL_CENTER - randomNumber(50, 320),
								.y = VERTICAL_CENTER - randomNumber(50, 320) };
		initialPosition.x = (i / 2 % 2 == 0) ?
			initialPosition.x : WINDOW_WIDTH - initialPosition.x;
		initialPosition.y = (i < SATELITE_COUNT / 2) ?
			initialPosition.y : WINDOW_HEIGHT - initialPosition.y;

		// Randomize velocity tangential to the balck hole
		floatvector positionToBlackHole = { .x = initialPosition.x - HORIZONTAL_CENTER,
									  .y = initialPosition.y - VERTICAL_CENTER };
		float distance = (0.06 + randomNumber(-0.01f, 0.01f)) /
			sqrt(positionToBlackHole.x * positionToBlackHole.x +
				positionToBlackHole.y * positionToBlackHole.y);
		floatvector initialVelocity = { .x = distance * -positionToBlackHole.y,
								  .y = distance * positionToBlackHole.x };

		// Every other orbits clockwise
		if (i % 2 == 0) {
			initialVelocity.x = -initialVelocity.x;
			initialVelocity.y = -initialVelocity.y;
		}

		satelite tmpSatelite = { .identifier = id,.position = initialPosition,
								.velocity = initialVelocity };
		satelites[i] = tmpSatelite;
	}
}

// い DO NOT EDIT THIS FUNCTION い
void fixedDestroy(void) {
	destroy();

	free(pixels);
	free(correctPixels);
	free(satelites);

	if (seed != 0) {
		printf("Used seed: %i\n", seed);
	}
}

// い DO NOT EDIT THIS FUNCTION い
// Renders pixels-buffer to the window 
void render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_FLOAT, pixels);
	glutSwapBuffers();
	frameNumber++;
}

// DO NOT EDIT THIS FUNCTION
// Inits glut and start mainloop
int main(int argc, char** argv) {

	if (argc > 1) {
		seed = atoi(argv[1]);
		printf("Using seed: %i\n", seed);
	}

	// Init glut window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Parallelization excercise");
	glutDisplayFunc(render);
	atexit(fixedDestroy);
	previousFrameTimeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	previousFinishTime = glutGet(GLUT_ELAPSED_TIME);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	fixedInit(seed);
	init();

	// compute-function is called when everythin from last frame is ready
	glutIdleFunc(compute);

	// Start main loop
	glutMainLoop();
}

//
//#include <stdio.h>
//#include <stdlib.h>
//#include <CL/cl.h>
//
//#define MAX_SOURCE_SIZE (0x100000)
//
//int main(void) {
//	printf("started running\n");
//
//	// Create the two input vectors
//	int i;
//	const int LIST_SIZE = 1024;
//	int* A = (int*)malloc(sizeof(int) * LIST_SIZE);
//	int* B = (int*)malloc(sizeof(int) * LIST_SIZE);
//	for (i = 0; i < LIST_SIZE; i++) {
//		A[i] = i;
//		B[i] = LIST_SIZE - i;
//	}
//
//	// Load the kernel source code into the array source_str
//	FILE* fp;
//	char* source_str;
//	size_t source_size;
//
//	fp = fopen("kernel.cl", "r");
//	if (!fp) {
//		fprintf(stderr, "Failed to load kernel.\n");
//		exit(1);
//	}
//	source_str = (char*)malloc(MAX_SOURCE_SIZE);
//	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
//	fclose(fp);
//	printf("kernel loading done\n");
//	// Get platform and device information
//	cl_device_id device_id = NULL;
//	cl_uint ret_num_devices;
//	cl_uint ret_num_platforms;
//
//
//	cl_int ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
//	cl_platform_id* platforms = NULL;
//	platforms = (cl_platform_id*)malloc(ret_num_platforms * sizeof(cl_platform_id));
//
//	ret = clGetPlatformIDs(ret_num_platforms, platforms, NULL);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	ret = clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_ALL, 1,
//		&device_id, &ret_num_devices);
//	printf("ret at %d is %d\n", __LINE__, ret);
//	// Create an OpenCL context
//	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	// Create a command queue
//	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	// Create memory buffers on the device for each vector 
//	cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
//		LIST_SIZE * sizeof(int), NULL, &ret);
//	cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
//		LIST_SIZE * sizeof(int), NULL, &ret);
//	cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
//		LIST_SIZE * sizeof(int), NULL, &ret);
//
//	// Copy the lists A and B to their respective memory buffers
//	ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
//		LIST_SIZE * sizeof(int), A, 0, NULL, NULL);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0,
//		LIST_SIZE * sizeof(int), B, 0, NULL, NULL);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	printf("before building\n");
//	// Create a program from the kernel source
//	cl_program program = clCreateProgramWithSource(context, 1,
//		(const char**)& source_str, (const size_t*)& source_size, &ret);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	// Build the program
//	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	printf("after building\n");
//	// Create the OpenCL kernel
//	cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	// Set the arguments of the kernel
//	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)& a_mem_obj);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)& b_mem_obj);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)& c_mem_obj);
//	printf("ret at %d is %d\n", __LINE__, ret);
//
//	//added this to fix garbage output problem
//	//ret = clSetKernelArg(kernel, 3, sizeof(int), &LIST_SIZE);
//
//	printf("before execution\n");
//	// Execute the OpenCL kernel on the list
//	size_t global_item_size = LIST_SIZE; // Process the entire lists
//	size_t local_item_size = 64; // Divide work items into groups of 64
//	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
//		&global_item_size, &local_item_size, 0, NULL, NULL);
//	printf("after execution\n");
//	// Read the memory buffer C on the device to the local variable C
//	int* C = (int*)malloc(sizeof(int) * LIST_SIZE);
//	ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,
//		LIST_SIZE * sizeof(int), C, 0, NULL, NULL);
//	printf("after copying\n");
//	// Display the result to the screen
//	for (i = 0; i < LIST_SIZE; i++)
//		printf("%d + %d = %d\n", A[i], B[i], C[i]);
//
//	// Clean up
//	ret = clFlush(command_queue);
//	ret = clFinish(command_queue);
//	ret = clReleaseKernel(kernel);
//	ret = clReleaseProgram(program);
//	ret = clReleaseMemObject(a_mem_obj);
//	ret = clReleaseMemObject(b_mem_obj);
//	ret = clReleaseMemObject(c_mem_obj);
//	ret = clReleaseCommandQueue(command_queue);
//	ret = clReleaseContext(context);
//	free(A);
//	free(B);
//	free(C);
//	return 0;
//}