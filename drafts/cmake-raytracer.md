+++
title = "Ray Tracing in pure CMake"
date = 2020-10-07

[taxonomies]
categories = ["Graphics"]
+++

Without further ado, I present: a basic whitted ray tracer, complete with multicore rendering, written in 100% pure CMake. If you don't care about the details, and just want to see the code, you can [find it here](https://github.com/64/cmake-raytracer).

<!-- more -->

{{ figure(src="https://github.com/64/cmake-raytracer/raw/master/render.png", caption="512x512, rendered on a **insert CPU details** in **insert time details** using **insert number procs** cores") }}

At this point, those familiar with CMake may have some questions, so keep reading to find out how it all works.

# Outputting an Image

# Fixed-Point Arithmetic

**Good news:** CMake has a [`math`](https://cmake.org/cmake/help/latest/command/math.html?highlight=math) command. **Bad news:** it only supports integers. If you've written a ray tracer before, you probably did it with floating point numbers. So how do you go from representing signed integers to representing something-resembling-floating-point numbers? One answer is to use [**fixed-point arithmetic**](https://en.wikipedia.org/wiki/Fixed-point_arithmetic).

The basic idea with fixed point is simple. We define some large integer to represent the number 1.0; let's choose **1000**. Then we can represent 2.0 as 2000, 0.5 as 500, -3.0 as -3000 etc. When we want to add two numbers, we simply add their fixed-point representations. Here's how that looks in CMake:
```cmake
function(add a b res)
    math(EXPR tmp "(${a}) + (${b})")
    set("${res}" "${tmp}" PARENT_SCOPE)
endfunction()
```
This takes two values `a` and `b` to be added and stored in the variable `res`. I use `PARENT_SCOPE` so that the variable we create is actually visible from the calling function, otherwise CMake will destroy it when the function ends.

To multiply two numbers, we simply multiply their fixed-point representations, and then divide by the thing we chose to represent 1.0:
{% katex(block=true) %}
1.5 \times 4.0 \mapsto \frac{1500 \times 4000}{1000} = 6000 \mapsto 6.0
{% end %}

# Render Loop

# Multicore Rendering

# Conclusion
