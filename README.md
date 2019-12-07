# Parallel Computing
## Second Part : OpenCL

Students participating in the project:
 - Khoa Nguyen: 272580
 - Kianoush Jafari: 267923
 - Thomas Pasquier: 291169

For this project, we are using the computer in room TC219, on Windows with Visual Studio 2019.
The computer is provided with the following hardware:
- CPU: Intel 4Core (8 logical processors) i7-6700 @ 3.40GHz
- GPU: NVIDIA Quadro K620

We also have an another work station, which runs on Linux with
- CPU:   
       ![LinuxSpecs](images/LinuxSpecs.png)

- GPU: NVIDIA Corporation GP102 [GeForce GTX 1080 Ti]   
       ![LinuxSpecs](images/1080TiSpecs.png)


**What is the proximal average total frametime (milliseconds) after the first frames for the following cases:**  
Note that we run with SATELITE_COUNT = 64, WINDOW_HEIGHT = WINDOW_WIDTH = 1024.

<center> Window work station </center>  

1. The original C Version on CPU without optimizations ?
Total frametime: 823ms, satelite moving: 152ms, space coloring: 650ms.

2. The original C Version on CPU with best optimizations ?
Total frametime: 703ms, satelite moving: 47ms, space coloring: 656ms.

3. The OpenCL version on GPU, WG size 1x1?
Total frametime: 428ms, satelite moving: 47ms, space coloring: 366ms.

4. The OpenCL version on GPU, WG size 4x4?
Total frametime: 78ms, satelite moving: 47ms, space coloring: 31ms.

5. The OpenCL version on GPU, WG size 8x4?
Total frametime: 78ms, satelite moving: 47ms, space coloring: 31ms.

6. The OpenCL version on GPU, WG size 8x8?
Total frametime: 78ms, satelite moving: 47ms, space coloring: 31ms.

7. The OpenCL version on GPU, WG size 16x16?
Total frametime: 78ms, satelite moving: 47ms, space coloring: 31ms.

<center> Linux work station </center>

1. The original C Version on CPU without optimizations ?
Total frametime: 1297ms, satelite moving: 106ms, space coloring: 1185ms.

2. The original C Version on CPU with best optimizations (including OpenMP)?
Total frametime: 105ms, satelite moving: 61ms, space coloring: 37ms.

3. The OpenCL version on GPU, WG size 1x1?
Total frametime: 80ms, satelite moving: 49ms, space coloring: 25ms.

4. The OpenCL version on GPU, WG size 4x4?
Total frametime: 56ms, satelite moving: 40ms, space coloring: 5ms.

5. The OpenCL version on GPU, WG size 8x4?
Total frametime: 60ms, satelite moving: 49ms, space coloring: 4ms.

6. The OpenCL version on GPU, WG size 8x8?
Total frametime: 17ms, satelite moving: 6ms, space coloring: 4ms.

7. The OpenCL version on GPU, WG size 16x16?
Total frametime: 48ms, satelite moving: 37ms, space coloring: 4ms.


**If some work group size or sizes did not work, try to explain why it did not work (This question is not mandatory, but may help to get a bonus point from the exercise)?**


**Try to analyze the performance with diﬀerent work group sizes. What might explain the diﬀerences**

Work group size during the execution will be break down into smaller units called warps. Each warp has 32 threads, and it completely runs  in parallel. If the number of the threads inside a work group is less than the number of the threads of a warp (32), then we will have waste of resources. The GPU will dedicate 32 cores for the block, and we will not be using all the 32 cores.  
When the work group size is less than the size of a warp (which typically is 32) then we will loose resources. This explains why with work group size of 1x1 or 4x4, the program runs rather slow in both work stations.  
When the work group size is equal or bigger than the size of a warp (8x4, 8x8, 16x16), then theoretically we have faster performance, which can be seen in both cases. However, as we observed, for the Windows workstation, the total time frame does not change for these work group size, this maybe due to the fact that the GPU is smart enough to allocate the resources accordingly.  
For the Linux workstation, we see that the performance varies when the work group size is either 8x4, 8x8, or 16x16, but they give the same time for space coloring, which is the part we parallelize with OpenCL.

**Try reducing the window size to very small, 80x80. The WINDOW WIDTH and WINDOW HEIGHT deﬁnes in the beginning of the code control this.**  
Note that we run this on the Linux workstation

1. On Original C version on CPU with optimizations on (-03 ﬂag)?  
Total frametime: 55ms, satelite moving: 52ms, space coloring: 3ms.

2. On OpenCL version on GPU, fastest WG size (can be diﬀerent than with the default image size)?  
Total frametime: 59ms, satelite moving: 58ms, space coloring: 1ms.

**If the OpenCL version running on GPU was slower than C on CPU with this image size, what is the reason for this?**  
We can still see that with small WINDOW_SIZE, the GPU version is still a bit faster for space coloring (1ms vs 3 ms for CPU). However, if the GPU version happens to be slower than the CPU version, we think that it is due to the OpenCL's data communicating process between the CPU and the GPU. 

**What was the most diﬃcult thing in this exercise work?**
For us, the most difficult work was understanding how OpenCL work, and then actually implement the concepts into the code given.  
Also, we spent quite a lot of time to install and run the project on both work stations, which is not necessary fun but it is an inevitable part of any project, especially with a new technology. 
