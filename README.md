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

**What is the proximal average total frametime (milliseconds) after the first frames for the following cases:**
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



**If some work group size or sizes did not work, try to explain why it did not work (This question is not mandatory, but may help to get a bonus point from the exercise)?**


**Try to analyze the performance with diﬀerent work group sizes. What might explain the diﬀerences**


**Try reducing the window size to very small, 80x80. The WINDOW WIDTH and WINDOW HEIGHT deﬁnes in the beginning of the code control this.**
1. On Original C version on CPU with optimizations on (-03 ﬂag)?


2. On OpenCL version on GPU, fastest WG size (can be diﬀerent than with the default image size)?


**If the OpenCL version running on GPU was slower than C on CPU with this image size, what is the reason for this?**


**What was the most diﬃcult thing in this exercise work?**
