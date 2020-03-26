+++
title = "Monte Carlo Integration Basics"
date = 2020-03-27

[taxonomies]
categories = ["Graphics"]
+++

## Monte Carlo Integration Basics

Monte Carlo Integration is a method for numerically computing an integral {% katex() %} \int_a^b f(x)\mathrm{d}x {% end %} which only requires being able to evaluate the function {% katex() %} f {% end %} at arbitrary points. It is especially useful for higher-dimensional integrals (involving multiple variables) such as {% katex() %} \int_A \int_B f(x, y)\mathrm{d}x\mathrm{d}y {% end %}. In this article I will touch on the basics of Monte Carlo Integration as well as explain a number of *variance reduction* techniques which we can use to get a more accurate computation in less time.
<!-- more -->

Foo