typedef struct{
	float x;
	float y;
} floatvector;

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
 
#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 1024
#define SATELITE_COUNT 64
#define SATELITE_RADIUS 3.16f
#define SIZE WINDOW_HEIGHT * WINDOW_HEIGHT


// __kernel void graphicsEngine(__constant satelite *satelites, __global color *pixels) {
// 	printf("In1. Pixels[0] is: r%f g%f b%f\n", pixels[0].red, pixels[0].green, pixels[0].blue);
// 	 pixels[0].red = 1.0f;
// 	pixels[0].blue = 2.0f;
// 	printf("InLast. Pixels[0] is: r%f g%f b%f\n", pixels[0].red, pixels[0].green, pixels[0].blue);
//  }

__kernel void graphicsEngine(__constant satelite *satelites, __global color *pixels, __global color* pixelsOut) {
	
	//printf("In1. Pixels[0] is: r%f g%f b%f\n", pixels[0].red, pixels[0].green, pixels[0].blue);
	//printf("In1. Satelite[0] is: x%f y%f\n", satelites[0].position.x, satelites[0].position.y);
	//pixels[0].red = SATELITE_RADIUS;

	int idx = get_global_id(0);
	int idy = get_global_id(1);
	//printf("idx. %d\n", idx);

	//  #pragma omp parallel for
	// #pragma omp master
	//Graphics pixel loop
	//for(int i = 0; i < SIZE; ++i) {

		// Row wise ordering
		floatvector pixel = {.x = idx % WINDOW_WIDTH, .y = idy / WINDOW_WIDTH};

		// This color is used for coloring the pixel
		color renderColor = {.red = 0.f, .green = 0.f, .blue = 0.f};

		// Find closest satelite
		float shortestDistance = INFINITY;

		float weights = 0.f;
		int hitsSatellite = 0;
      
		// First Graphics satelite loop: Find the closest satellite.
		for(int j = 0; j < SATELITE_COUNT; ++j) {
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
				if(distance < shortestDistance) {
					shortestDistance = distance;
					renderColor = satelites[j].identifier;
				}
			}
		}

		//printf("before if\n");
		// Second graphics loop: Calculate the color based on distance to every satelite.
		if (!hitsSatellite) {
			// #pragma omp parallel for
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
		printf("before pixels out PixelsOut is: r%f g%f b%f\n", pixelsOut[idx + WINDOW_WIDTH * idy].red, pixelsOut[idx + WINDOW_WIDTH * idy].green, pixelsOut[idx + WINDOW_WIDTH * idy].blue);
		pixelsOut[idx + WINDOW_WIDTH * idy] = renderColor;
		printf("after pixels out PixelsOut is: r%f g%f b%f\n", pixelsOut[idx + WINDOW_WIDTH * idy].red, pixelsOut[idx + WINDOW_WIDTH * idy].green, pixelsOut[idx + WINDOW_WIDTH * idy].blue);
	//}

	//printf("InLast. PixelsOut[0] is: r%f g%f b%f\n", pixels[0].red, pixels[0].green, pixels[0].blue);
}