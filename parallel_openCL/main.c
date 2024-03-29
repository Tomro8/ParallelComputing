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
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

// OpenCL includes
#include "CL/cl.h"

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
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
typedef struct{
   float x;
   float y;
} floatvector;

// Stores 2D data like the coordinates
typedef struct{
   double x;
   double y;
} doublevector;

// Stores rendered colors. Each float may vary from 0.0f ... 1.0f
typedef struct{
   float red;
   float green;
   float blue;
} color;

// Stores the satelite data, which fly around black hole in the space
typedef struct{
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

#define WG_SIZE_X 8
#define WG_SIZE_Y 8

#define MAX_SOURCE_SIZE (0x100000)

// Use this to check the output of each API call
cl_int status;
// Context object
cl_context context;
// Kernel object
cl_kernel kernel;
// Hold satelite data for the kernel
cl_mem satelite_data_buffer;
// Hold pixel data from the kernel
cl_mem pixel_data_buffer;
// Command queue to assiciate with the device
cl_command_queue cmdQueue;
// Program object
cl_program program;


// ## You may add your own initialization routines here ##
void init(){

	// Retrieve the number of platforms
	cl_uint numPlatforms = 0;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) 
	{
		printf("Error retrieving platforms\n");
		return;
	} 
	printf("Number of platforms: %d\n", numPlatforms);

	// Allocate enough space for each platform
	cl_platform_id* platforms = NULL;
	platforms = (cl_platform_id*)malloc(
		numPlatforms * sizeof(cl_platform_id));
	if (platforms == NULL)
	{
		printf("Error while allocating space for platforms\n");
	}
	printf("Space for platforms allocated\n");

	// Fill in the platforms
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);
	if (status != CL_SUCCESS)
	{
		printf("Error while filling the platforms\n");
		return;
	}
	printf("Platforms filled\n");

	// Retrieve the number of devices
	cl_uint numDevices = 0;
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0,
		NULL, &numDevices);
	if (status != CL_SUCCESS)
	{
		printf("Error while retrieving number of devices\n");
		return;
	}
	printf("Number of devices: %d\n", numDevices);

	// Allocate enough space for each device
	cl_device_id* devices;
	devices = (cl_device_id*)malloc(
		numDevices * sizeof(cl_device_id));
	if (devices == NULL)
	{
		printf("Error while allocating space for devices\n");
	}
	printf("Space for devices allocated\n");

	// Fill in the devices 
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU,
		numDevices, devices, NULL);
	if (status != CL_SUCCESS)
	{
		printf("Error while filling devices\n");
		return;
	}
	printf("Devices filled\n");

	// Create a context and associate it with the devices
	context = clCreateContext(NULL, numDevices, devices, NULL,
		NULL, &status);
	if (status != CL_SUCCESS)
	{
		printf("Error while creating context\n");
		return;
	}
	printf("Context created\n");

	// Create a command queue and associate it with the device 
	cmdQueue = clCreateCommandQueue(context, devices[0], 0,
		&status);
	if (status != CL_SUCCESS)
	{
		printf("Error while creating and associating command queue to device\n");
		return;
	}
	printf("Command queue created and associated to device\n");

	// Create a buffer object that will contain the data 
    // For satelite data
	satelite_data_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, SATELITE_COUNT * sizeof(satelite),
		NULL, &status);
	
	if (status != CL_SUCCESS)
	{
		printf("Error while creating satelite buffer input\n");
		return;
	}
	printf("Satelite input buffer created\n");

	// For pixel data
	pixel_data_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(color),
		NULL, &status);
	if (status != CL_SUCCESS)
	{
		printf("Error while creating pixel buffer output\n");
		return;
	}
	printf("Pixel output buffer created\n");

	// Load the kernel source code into the array source_str
	// Small piece of code comming from https://github.com/pratikone/OpenCL-learning-VS2012/blob/master/main.cpp
	FILE* fp;
	char* source_str;
	size_t source_size;

	fopen_s(&fp, "graphicsEngine.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel source.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
	printf("Kernel source loading done\n");

	// Create a program with source code
	program = clCreateProgramWithSource(context, 1,
		(const char**)& source_str, NULL, &status);
	if (status != CL_SUCCESS)
	{
		printf("Error while creating program\n");
		return;
	}
	printf("Program created\n");

	// Build (compile) the program for the device
	status = clBuildProgram(program, numDevices, devices,
		NULL, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		// Printing logs of program build failure
		printf("Error while building the program: Status=%d\n", status);

		char* log;
		size_t logLen;
		status = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &logLen);
		log = (char*)malloc(sizeof(char) * logLen);
		status = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, logLen, (void*)log, NULL);
		fprintf(stdout, "\nCL Error %d: Failed to build program! Log:\n%s", status, log);
		free(log);

		return;
	}
	printf("Program built\n");

	// Create the vector addition kernel
	kernel = clCreateKernel(program, "graphicsEngine", &status);
	if (status != CL_SUCCESS)
	{
		printf("Error creating the kernel\n");
		return;
	}
	printf("Kernel created\n");

}

// ## You are asked to make this code parallel ##
// Physics engine loop. (This is called once a frame before graphics engine) 
// Moves the satellites based on gravity
// This is done multiple times in a frame because the Euler integration 
// is not accurate enough to be done only once
void parallelPhysicsEngine(){

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
   // Physics satelite loop
   #pragma omp parallel for
   for(int j = 0; j < SATELITE_COUNT/4; ++j){
      for(int physicsUpdateIndex = 0; 
          physicsUpdateIndex < PHYSICSUPDATESPERFRAME; ++physicsUpdateIndex){

         for(int i = j*4; i <= j*4+3; i++) {
         // Distance to the blackhole (bit ugly code because C-struct cannot have member functions)

         doublevector positionToBlackHole = {.x = tmpPosition[i].x -
            HORIZONTAL_CENTER, .y = tmpPosition[i].y - VERTICAL_CENTER};
         double distToBlackHoleSquared =
            positionToBlackHole.x * positionToBlackHole.x +
            positionToBlackHole.y * positionToBlackHole.y;
         double distToBlackHole = sqrt(distToBlackHoleSquared);

         // Gravity force
         doublevector normalizedDirection = {
            .x = positionToBlackHole.x / distToBlackHole,
            .y = positionToBlackHole.y / distToBlackHole};
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

	// Filling satelite input buffer with satelite data
	status = clEnqueueWriteBuffer(cmdQueue, satelite_data_buffer, CL_FALSE,
		0, SATELITE_COUNT * sizeof(satelite), satelites, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		printf("Error while feeding data into satelite buffer to the kernel\n");
		return;
	}
	printf("Satelite buffer initalized with data\n");

	// Filling pixel buffer with pixel data
	status = clEnqueueWriteBuffer(cmdQueue, pixel_data_buffer, CL_FALSE,
		0, SIZE * sizeof(color), pixels, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		printf("Error while feeding data into pixel buffer to the kernel\n");
		return;
	}
	printf("Pixel buffer initalized with data\n");


	// Associating satelite buffer to kernel (Associate buffer to kernel)
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &satelite_data_buffer);
	if (status != CL_SUCCESS)
	{
		printf("Error while associating satelite data buffer\n");
		return;
	}
	printf("Satelite buffer associated with kernel\n");

	// Associating pixel buffer output to kernel
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &pixel_data_buffer);
	if (status != CL_SUCCESS)
	{
		printf("Error while associating pixel data buffer to the kernel\n");
		return;
	}
	printf("Pixel buffer associated with kernel\n");


	// Define an index space (global work size) of work 
	// items for execution. A workgroup size (local work size) 
	// is not required, but can be used.
   int global_size_x, global_size_y;
   global_size_x = ceil((float)WINDOW_WIDTH / (float)WG_SIZE_X) * WG_SIZE_X;
   global_size_y = ceil((float)WINDOW_WIDTH / (float)WG_SIZE_Y) * WG_SIZE_Y;
	size_t globalWorkSize[] = {global_size_x, global_size_y};
	size_t localWorkSize[] = { WG_SIZE_X, WG_SIZE_Y };

	// Executing kernel
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, 0,
		globalWorkSize, localWorkSize, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		printf("Error while executing kernel\n");
		return;
	}
	printf("Kernel executed\n");

	// Make sure command queue has issued all commands
	status = clFinish(cmdQueue);
	if (status != CL_SUCCESS)
	{
		printf("Error clFinish is called\n");
		return;
	}

	// Read device output buffer to the host pixel array
	clEnqueueReadBuffer(cmdQueue, pixel_data_buffer, CL_TRUE, 0,
		SIZE * sizeof(color), pixels, 0, NULL, NULL);
}

// ## You may add your own destrcution routines here ##
void destroy(){
	// Free OpenCL resources
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(satelite_data_buffer);
	clReleaseMemObject(pixel_data_buffer);
	clReleaseContext(context);
}


////////////////////////////////////////////////
// ¤¤ TO NOT EDIT ANYTHING AFTER THIS LINE ¤¤ //
////////////////////////////////////////////////

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
// Sequential rendering loop used for finding errors
void sequentialGraphicsEngine(){

    // Graphics pixel loop
    for(int i = 0 ;i < SIZE; ++i) {

      // Row wise ordering
      floatvector pixel = {.x = i % WINDOW_WIDTH, .y = i / WINDOW_WIDTH};

      // This color is used for coloring the pixel
      color renderColor = {.red = 0.f, .green = 0.f, .blue = 0.f};

      // Find closest satelite
      float shortestDistance = INFINITY;

      float weights = 0.f;
      int hitsSatellite = 0;

      // First Graphics satelite loop: Find the closest satellite.
      for(int j = 0; j < SATELITE_COUNT; ++j){
         floatvector difference = {.x = pixel.x - satelites[j].position.x,
                                   .y = pixel.y - satelites[j].position.y};
         float distance = sqrt(difference.x * difference.x + 
                               difference.y * difference.y);

         if(distance < SATELITE_RADIUS) {
            renderColor.red = 1.0f;
            renderColor.green = 1.0f;
            renderColor.blue = 1.0f;
            hitsSatellite = 1;
            break;
         } else {
            float weight = 1.0f / (distance*distance*distance*distance);
            weights += weight;
            if(distance < shortestDistance){
               shortestDistance = distance;
               renderColor = satelites[j].identifier;
            }
         }
      }

      // Second graphics loop: Calculate the color based on distance to every satelite.
      if (!hitsSatellite) {
         for(int j = 0; j < SATELITE_COUNT; ++j){
            floatvector difference = {.x = pixel.x - satelites[j].position.x,
                                      .y = pixel.y - satelites[j].position.y};
            float dist2 = (difference.x * difference.x +
                           difference.y * difference.y);
            float weight = 1.0f/(dist2* dist2);

            renderColor.red += (satelites[j].identifier.red *
                                weight /weights) * 3.0f;

            renderColor.green += (satelites[j].identifier.green *
                                  weight / weights) * 3.0f;

            renderColor.blue += (satelites[j].identifier.blue *
                                 weight / weights) * 3.0f;
         }
      }
      correctPixels[i] = renderColor;
    }
}

void sequentialPhysicsEngine(satelite *s){

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
   for(int physicsUpdateIndex = 0;
       physicsUpdateIndex < PHYSICSUPDATESPERFRAME;
      ++physicsUpdateIndex){

       // Physics satelite loop
      for(int i = 0; i < SATELITE_COUNT; ++i){

         // Distance to the blackhole
         // (bit ugly code because C-struct cannot have member functions)
         doublevector positionToBlackHole = {.x = tmpPosition[i].x -
            HORIZONTAL_CENTER, .y = tmpPosition[i].y - VERTICAL_CENTER};
         double distToBlackHoleSquared =
            positionToBlackHole.x * positionToBlackHole.x +
            positionToBlackHole.y * positionToBlackHole.y;
         double distToBlackHole = sqrt(distToBlackHoleSquared);

         // Gravity force
         doublevector normalizedDirection = {
            .x = positionToBlackHole.x / distToBlackHole,
            .y = positionToBlackHole.y / distToBlackHole};
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

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
void errorCheck(){
   for(int i=0; i < SIZE; ++i) {
      if(correctPixels[i].red != pixels[i].red ||
         correctPixels[i].green != pixels[i].green ||
         correctPixels[i].blue != pixels[i].blue){ 

         printf("Buggy pixel at (x=%i, y=%i). Press enter to continue.\n", i % WINDOW_WIDTH, i / WINDOW_WIDTH);
		 printf("MyPixel/CorrectPixel are\n r:%f/%f g:%f/%f b:%f/%f \n", 
			 pixels[i].red, correctPixels[i].red,
			 pixels[i].green, correctPixels[i].green,
			 pixels[i].blue, correctPixels[i].blue);
         //getchar();
         return;
       }
   }
   printf("Error check passed!\n");
}

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
void compute(void){
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
         if (memcmp (&satelites[i], &backupSatelites[i], sizeof(satelite))) {
            printf("Incorrect satelite data of satelite: %d\n", i);
            getchar();
         }
      }
   }

   int sateliteMovementMoment = glutGet(GLUT_ELAPSED_TIME);
   int sateliteMovementTime = sateliteMovementMoment  - timeSinceStart;

   // Decides the colors for the pixels
   parallelGraphicsEngine();

   int pixelColoringMoment = glutGet(GLUT_ELAPSED_TIME);
   int pixelColoringTime =  pixelColoringMoment - sateliteMovementMoment;

   // Sequential code is used to check possible errors in the parallel version
   if(frameNumber < 2){
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

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
// Probably not the best random number generator
float randomNumber(float min, float max){
   return (rand() * (max - min) / RAND_MAX) + min;
}

// DO NOT EDIT THIS FUNCTION
void fixedInit(unsigned int seed){

   if(seed != 0){
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
   for(int i = 0; i < SATELITE_COUNT; ++i){

      // Random reddish color
      color id = {.red = randomNumber(0.f, 0.15f) + 0.1f,
                  .green = randomNumber(0.f, 0.14f) + 0.0f,
                  .blue = randomNumber(0.f, 0.16f) + 0.0f};
    
      // Random position with margins to borders
      floatvector initialPosition = {.x = HORIZONTAL_CENTER - randomNumber(50, 320),
                              .y = VERTICAL_CENTER - randomNumber(50, 320) };
      initialPosition.x = (i / 2 % 2 == 0) ?
         initialPosition.x : WINDOW_WIDTH - initialPosition.x;
      initialPosition.y = (i < SATELITE_COUNT / 2) ?
         initialPosition.y : WINDOW_HEIGHT - initialPosition.y;

      // Randomize velocity tangential to the balck hole
      floatvector positionToBlackHole = {.x = initialPosition.x - HORIZONTAL_CENTER,
                                    .y = initialPosition.y - VERTICAL_CENTER};
      float distance = (0.06 + randomNumber(-0.01f, 0.01f))/ 
        sqrt(positionToBlackHole.x * positionToBlackHole.x + 
          positionToBlackHole.y * positionToBlackHole.y);
      floatvector initialVelocity = {.x = distance * -positionToBlackHole.y,
                                .y = distance * positionToBlackHole.x};

      // Every other orbits clockwise
      if(i % 2 == 0){
         initialVelocity.x = -initialVelocity.x;
         initialVelocity.y = -initialVelocity.y;
      }

      satelite tmpSatelite = {.identifier = id, .position = initialPosition,
                              .velocity = initialVelocity};
      satelites[i] = tmpSatelite;
   }
}

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
void fixedDestroy(void){
   destroy();

   free(pixels);
   free(correctPixels);
   free(satelites);

   if(seed != 0){
     printf("Used seed: %i\n", seed);
   }
}

// ¤¤ DO NOT EDIT THIS FUNCTION ¤¤
// Renders pixels-buffer to the window 
void render(void){
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_FLOAT, pixels);
   glutSwapBuffers();
   frameNumber++;
}

// DO NOT EDIT THIS FUNCTION
// Inits glut and start mainloop
int main(int argc, char** argv){

   if(argc > 1){
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
