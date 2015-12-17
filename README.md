# Team Members
* Yu Mao, Robotics Institute, School of Computer Science, Carnegie Mellon University (ymao1@andrew.cmu.edu)
* Rui Zhu, Robotics Institute, School of Computer Science, Carnegie Mellon University (rz1@andrew.cmu.edu)

# [Source Code (Click to Access the Repository)](https://github.com/YuMao1993/cmu-15462-assignment5-YuMao-RuiZhu)
Also submitted to the afs cloud at 

> /afs/cs/academic/class/15462-f15-users/**ymao1**/asst5/
 
## How to test the code

**Warning**: Before compiling the program, make sure you have:

- CUDA compatible Graphics Card
- CUDA SDK installed

To test Task 1, the BVH build thing, you can type in
```
./pathtracer ../dae/sky/CBbunny.dae
```

and hit `v` for BVH construction (use classic path tracing by default. See parameter `-p` below).

To test Task 2, you can type in (for example to test the CBsphere (and lambertian) case)
```
./pathtracer -t 18 -s 16 -p 1 ../dae/sky/CBspheres.dae
```

and hit `r` for BDPT rendering.

The `-p` parameter is a switch for BDPT or classic path tracing (1 is BDPT; 0 for classic path tracing (default)).

Again notice that only **AreaLight** models (the Cornell Box series) are supported in our little BDPT demo version.


# [Final Report (in .pdf; 15+ pages; Click me to view)](https://github.com/YuMao1993/cmu-15462-assignment5-YuMao-RuiZhu/blob/master/15662_Project_Report.pdf)

# A Brief Overview of the Project
## What did we do
We complemented two tasks according to the proposal:

1. **Parallel BVH Construction with CUDA Acceleration**
2. **Bidirectional Path Tracing**. (**AreaLight** cases supported)

## Overview of the project
### Task 1: Parallel BVH Construction with CUDA Acceleration

For the first task, we first implemented the Morton code based method which only runs on CPU. Then we implemented the parallel binary radix tree(BRT) construction algorithm as proposed in [*Karras, Tero. "Maximizing parallelism in the construction of BVHs, octrees, and k-d trees."*](http://dl.acm.org/citation.cfm?id=2383801) for general BRT construction. Thirdly, we implemented the parallel algorithm for BVH construction based on BRT construction algorithm, which augments it by adding a bottom-up parallel bounding box construction phase. We also compared the efficiency of different approaches.

A demo video about our blazing fast BVH construction is available on Youtube  (**Click the thumbnail below to show**):

[![Everything Is AWESOME](http://7u2p2j.com1.z0.glb.clouddn.com/youtubeCUDA.jpg)](https://youtu.be/MotzB-XGoaE)

We are thrilled to see those complex models rendered which would otherwise take hours to render in the past.

Some constructed BVHs and rendered images, as well as analysis of the performance are listed below:

![](http://7u2p2j.com1.z0.glb.clouddn.com/CUDAres1.png)

![](http://7u2p2j.com1.z0.glb.clouddn.com/CUDAres2.png)

### Task 2: Bidirectional Path Tracing
For the second one, we modified the renderer used in Homework 3, which uses traditional path tracing, to one which uses Bidirectional Path Tracing (BDPT). The new renderer is able to render scenes with AreaLight using BDPT with better illumination and variation in shadowed areas. We compared the efficiency of there two methods, as well as the ``photorealisticity" of the rendered images under these two methods. Our BDPT gives better illumination in the soft shaded area, and less variance in the caustic focused light under a glass sphere.

This is a comparison of our renderer and the classic path tracing:

![](http://7u2p2j.com1.z0.glb.clouddn.com/CGReport.png)

There is also a  really cool screen record of our BDPT renderer (**Click the thumbnail below to show**)

[![Everything Is AWESOME](http://7u2p2j.com1.z0.glb.clouddn.com/youtubeBDPT.jpg)](https://youtu.be/gRIJggimCf4)

# Initial Project Proposal [Changed during implementation]

For the last assignment, we are going to choose option F: **Advanced Monte Carlo Rendering**. We will implement sub-options **1 to 4**.

## Task 1: Improve performance of BVH construction

We would improve the BVH construction process by refering to 
[_Fast Parallel Construction of High-Quality Bounding Volume Hierarchies_](https://research.nvidia.com/publication/fast-parallel-construction-high-quality-bounding-volume-hierarchies). We would implement the method in the previous system for assignment 3.

We would conduct evaluation on the system to compare the efficiency before and after the improvement.

##Task 2: Improve performance of ray tracing

We would refer to [Ray Tracing Deformable Scenes using
Dynamic Bounding Volume Hierarchies](http://graphics.stanford.edu/%7Eboulos/papers/togbvh.pdf) and implement the fast packet-based ray tracing using. 

We would conduct evaluation on the system to compare the efficiency before and after the improvement.

## Task 3: Implement an advanced global sampling algorithm.

We would like to implement **bidirectional path tracing** to help find the best paths and reduce the cost. Maybe We will consider using parallel computing skills to further improve the efficiency.

We should be able to output the variance of the estimator and the total time cost, and make a comparison with the old routine.

We may refer to the _PBRT_ book for details about the algorithm.

## Task 4: Improved sampling patterns.

We would use **blue noise** (maybe **adaptive blue noise**) and **stratified sampling** to improve the sampling pattern. We will compare the estimated variance of the estimator and the time consumption.
