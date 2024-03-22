---
title: "donuts!"
subtitle: "ECE 3375 Final Project, 2024 "
author: James Su, Shiv Patel, Khalid Zabalawi
date: March 22, 2024
geometry: margin=1in
output: pdf_document
---

# Problem Definition

> 3D rendering is a common modern computing task, relevant in many different applications from animation to engineering. It consumes a significant number of clock cycles / computing time on generalized hardware, so it is often desirable to offload the task to a secondary, specialized microprocessor.

> Graphics cards are microcontrollers that are purpose built to handle parallelizable tasks, including graphics rendering. It contains its own microprocessor, the GPU, as well as its own memory and I/O.

> In this project, we'll build a simple 3D graphics engine on the DE10-SoC. It will show a 3D torus on a monitor using the VGA port, and provide controls to rotate the torus. 

### Effect on the user
> 3D rendering is a very generalizable technique with varying applications.
Offloading graphics can allow the main processor to handle other tasks, increasing the efficiency of the device as a whole. Hardware architectures that excel at parallel tasks are also conveniently applicable in other computational tasks like machine learning, scientific computing, data mining and more. 

> Real graphics cards provide this through CUDA and ROCm software tools.

> Specialized hardware is also more effecient than general purpose hardware, and can be optimized for lower resource settings. This allows the users to decrease and limit their power consumption and effect the planet.

# Functional Description

> When the program starts, a connected monitor will show a rendered torus.
> Buttons 0, 1, and 2 will correspond to rotations in the X, Y, and Z axis, and switches 0, 1, and 2 will control the direction of these rotations respectively.
> The 3D torus, in its rotated orientation is rendered to a 2D frame buffer that is then displayed on the connected monitor in real time using the VGA controller.

> 

# Input/Output
- for input, we use the buttons and switches, and timer
  - pressed buttons rotate the object, and the switches control the direction
  - the timer is used to track the elapsed time since the last update
    - this way, we get more precise control over the rotation speed

> Although the DE10-SoC could emulate GPU hardware with the FPGA, we'll stick to using the CPU for this project. 

