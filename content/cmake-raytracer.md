+++
title = "Ray Tracing in pure CMake"
date = 2020-12-27

[taxonomies]
categories = ["Graphics"]
+++

Without further ado, I present: a basic whitted ray tracer, complete with multicore rendering, written in 100% pure CMake. If you don't care about the details, and just want to see the code, you can [find it here](https://github.com/64/cmake-raytracer).

<!-- more -->

{{ figure(src="https://github.com/64/cmake-raytracer/raw/master/render.png") }}

At this point, those familiar with CMake may have some questions, so keep reading to find out how it all works.

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

Division is similar:
{% katex(block=true) %}
1.5 \div 4.0 \mapsto \frac{1500 \times 1000}{4000} = 375 \mapsto 0.375
{% end %}
We could have multiplied by 1000 after doing the division, but as integer division rounds towards zero this would wipe out all our precision (as {% katex() %} \frac{1500}{4000}\times 1000 = 0 \times 1000 = 0 {% end %}). Multiplying first gives us better results, as long as the dividend isn't too huge (which would cause overflow).

CMake's `math` command only supports basic integer arithmetic. For more complicated operations, like square root, we use [Newton-Raphson iteration](https://en.wikipedia.org/wiki/Newton%27s_method). You can read more about this [here](https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Babylonian_method), but the basic idea is to make a 'guess' as to what the output should be then iteratively refine the guess towards the answer. This gives a surprisingly accurate result within only three or four iterations, subject to the quality of the initial guess:
```cmake
function(sqrt x res)
    div_by_2(${x} guess)

    foreach(counter RANGE 4)
        if(${guess} EQUAL 0)
            set("${res}" 0 PARENT_SCOPE)
            return()
        endif()

        div(${x} ${guess} tmp)
        add(${tmp} ${guess} tmp)
        div_by_2(${tmp} guess)
    endforeach()

    set("${res}" "${guess}" PARENT_SCOPE)
endfunction()

# sqrt(123) = 11.09072626, actual answer is 11.0905365064
```

I also implemented a similar function for computing {% katex() %} \frac{1}{\sqrt{x}} {% end %} separately as I found that it lead to better numerical stability, as opposed to computing the square root as above and then doing the reciprocal. This comes in handy when we need to normalize vectors.

Almost everything in computer graphics is done with vectors, so I started implementing vector operations: `vec3_add`, `vec3_mul`, `vec3_div`, `vec3_dot` etc. These make use of CMake built-in lists, which are pretty horrible, but save me from having to use three separate variables to keep track of the individual components of each vector. For example, here's what the dot product looks like:
```cmake
function(vec3_dot x y res)
    list(GET ${x} 0 x_0)
    list(GET ${x} 1 x_1)
    list(GET ${x} 2 x_2)
    list(GET ${y} 0 y_0)
    list(GET ${y} 1 y_1)
    list(GET ${y} 2 y_2)
    mul(${x_0} ${y_0} z_0)
    mul(${x_1} ${y_1} z_1)
    mul(${x_2} ${y_2} z_2)
    add(${z_0} ${z_1} tmp)
    add(${tmp} ${z_2} tmp)
    set("${res}" ${tmp} PARENT_SCOPE)
endfunction()
```
And here's how we'd use it to normalize a vector:
```cmake
function(vec3_normalize x res)
    vec3_dot(${x} ${x} x_2)
    rsqrt(${x_2} one_over_length)
    vec3_mulf(${x} ${one_over_length} tmp)
    set("${res}" ${tmp} PARENT_SCOPE)
endfunction()
```

As well a few other bits and bobs, like `clamp` and `truncate`, that's all the arithmetic that's needed.

# Rendering

If you're new to ray tracing, I'd refer you to [Peter Shirley's](https://twitter.com/peter_shirley) wonderful book series '[Ray Tracing in One Weekend](https://raytracing.github.io/)', which my code is loosely based on. The general intuition is to trace rays out from the camera into the scene and see what they intersect. Since we represent all our scene geometry and rays as mathematical objects, computing intersections between rays and geometry is just a case of solving equations. Once we have found an intersection, we compute the color of the point we intersected with, which may itself be computed by tracing rays towards light sources or towards other scene geometry.

{{ figure(src="https://developer.nvidia.com/sites/default/files/pictures/2018/RayTracing/ray-tracing-image-1.jpg", caption="The ray tracing algorithm.", credit="https://developer.nvidia.com", creditsrc="https://developer.nvidia.com/discover/ray-tracing") }} 

To keep it simply I went with a simple scene consisting of a sphere sitting atop an infinite plane in a checkerboard color. I also ended up faking the shadow underneath the sphere, simply drawing a black circle (well done if you spotted it from the image). I had implemented whitted ray tracing and even path tracing at one point, but they were much more complicated and performed a lot worse for the same result. In theory, though, there's no reason why I couldn't do it properly, it would just require some additional effort and patience.

Here's what the main 'trace' function looks like, with some of the unnecessary bits stripped out for clarity:
```cmake
# Traces a ray into the scene, computes the color returned along the ray
function(trace ray_origin ray_dir depth color)
    # Base case for recursion
    if(${depth} GREATER_EQUAL 3)
        return()
    else()
        math(EXPR depth "${depth} + 1")
    endif()

    # Calculate intersection points with the sphere and plane
    sphere_intersect(${ray_origin} ${ray_dir} hit_t_1 hit_point_1 hit_normal_1)
    plane_intersect(${ray_origin} ${ray_dir} hit_t_2 hit_point_2 hit_normal_2)

    # Did we hit the sphere?
    if(${hit_t_1} GREATER ${ray_epsilon})
        # Calculate reflected ray direction
        offset_origin(hit_point_1 hit_normal_1 new_origin)
        vec3_dot(hit_normal_1 ${ray_dir} scalar)
        mul_by_2(${scalar} scalar)
        vec3_mulf(hit_normal_1 ${scalar} refl_a)
        vec3_sub(${ray_dir} refl_a new_dir)

        # Recursively trace the new ray into the scene
        trace(new_origin new_dir ${depth} traced_col)

        # Calculate contribution from lights
        set(col 0 0 0)
        light_contrib(hit_point_1 hit_normal_1 light1_pos light1_col out_col1)
        light_contrib(hit_point_1 hit_normal_1 light2_pos light2_col out_col2)
        vec3_add(col out_col1 col)
        vec3_add(col out_col2 col)
        vec3_add(col traced_col col)

        set(base_col ${sphere_color})
        vec3_mul(base_col col col)

    # Did we hit the plane?
    elseif(${hit_t_2} GREATER ${ray_epsilon})
        # ...snip: Use equation of a circle to fake shadow, if we're within range
        # ...snip: Calculate checkerboard pattern
    else()
        # We hit nothing, return black
        set(col 0 0 0)
    endif()

    set("${color}" ${col} PARENT_SCOPE)
endfunction()
```

# Multicore Rendering (a.k.a making CMake go *brrrrrrrrrrrr...*)

When I started, I wouldn't sure if it would be possible to do in pure CMake, but with a little trickery we can manage it.

For {% katex() %} N {% end %} processes, the basic plan is to divide up the image vertically and let each sub-process render a few rows. We can invoke sub-processes with the [`execute_process`](https://cmake.org/cmake/help/v3.0/command/execute_process.html) command, passing arguments (such as the worker index) via `-D`. Each process then spits their row data into a text file, which gets merged together by the master process once they've all finished.

One subtlety is that as we need all the sub-processes to run in parallel, we can't simply call `execute_process` {% katex() %} N {% end %} times, as it would run them sequentially. Luckily, we can specify multiple processes to run simultaneously in one command (I think this is intended to be used for long chains where one program is piped into the next), but in order to avoid hardcoding {% katex() %} N {% end %} we have to programmatically construct the call to `execute_process` with CMake's [`EVAL CODE`](https://cmake.org/cmake/help/git-stage/command/cmake_language.html) feature (thanks to [martty](https://github.com/martty/vuk) for this idea):
```cmake
    message(STATUS "Launching ray tracer with ${num_procs} processes, ${image_width}x${image_height} image...")

    set(exec_command "execute_process(\n")
    foreach(worker_index RANGE 1 ${num_procs})
        set(exec_command "${exec_command}COMMAND cmake . -Wno-dev -Dworker_index=${worker_index} -Dimage_width=${image_width} -Dimage_height=${image_height} -Dnum_procs=${num_procs}\n")
    endforeach()
    set(exec_command "${exec_command} )")

    # Begin the worker processes
    cmake_language(EVAL CODE ${exec_command})

    message(STATUS "Finished ray tracing, gathering results...")
```

# Outputting an Image

As per [Ray Tracing in One Weekend](https://raytracing.github.io/), I use the [PPM image format](https://en.wikipedia.org/wiki/Netpbm#PPM_example). This is a really simple text-based format which is perfect for my purposes as I don't have to bother with compression. Once we're done rendering we simply read all the data that the workers have spat out, write the PPM header, and print everything to `stderr`:

```cmake
    set(image_contents "P3 ${image_width} ${image_height}\n255\n\n")

    foreach(worker_index RANGE 1 ${num_procs})
        file(READ "worker-${worker_index}.txt" file_contents)
        set(image_contents "${image_contents}${file_contents}")
    endforeach()

    message("${image_contents}")
```

The division of work among the worker processes is pretty sub-optimal as the rows towards the top of the image are mostly empty whereas the rows at the bottom are entirely full, which means that some processes finish very fast while others take much longer. Fixing this problem is left as an exercise to the reader.

# Conclusion

If you made it this far, thanks for reading! Feel free to create issues, send pull requests or star [the code on GitHub](https://github.com/64/cmake-raytracer).
