+++
title = "Monte Carlo Integration Basics"
date = 2020-03-27

[taxonomies]
categories = ["Graphics", "Maths"]
+++

Monte Carlo Integration is a method for numerically computing an integral {% katex() %} \int_a^b f(x)\mathrm{d}x {% end %} which only requires being able to evaluate the function {% katex() %} f {% end %} at arbitrary points. It is especially useful for higher-dimensional integrals (involving multiple variables) such as {% katex() %} \int_A \int_B f(x, y)\mathrm{d}x\mathrm{d}y {% end %}. In this article I will touch on the basics of Monte Carlo Integration as well as explain a number of *variance reduction* techniques which we can use to get a more accurate result with fewer computations.
<!-- more -->

## Numerical Integration

Say we have a function {% katex() %} f(x) = x^3 {% end %} which we wish to integrate. Recall that the integral of {% katex() %} f {% end %} can, in some sense, be thought of as the (signed) area under the graph of {% katex() %} f {% end %}:

<center><div id="vis0"></div></center>

Suppose we're interested in integrating this function between {% katex() %} 0 {% end %} and {% katex() %} 1 {% end %}. We can do this analytically:

{% katex(block=true) %}
\begin{aligned}
    \int_0^1 f(x)\mathrm{d}x &= \int_0^1 x^3\mathrm{d}x \\
    &= \left.\frac{x^{4}}{4} \right\vert_{0}^{1} \\
    &= \frac{1}{4}
\end{aligned}
{% end %}

### Quadrature Rules

One simple way of approximating the integral would be to:

* Partition the domain {% katex() %} [0, 1] {% end %} into {% katex() %} N {% end %} subintervals: {% katex() %} \{ [x_0, x_1], [x_1, x_2], \ldots, [x_{n-2}, x_{n-1}] \} {% end %}
* Define {% katex() %} \hat{I} = \sum_{n=1}^{n=N} (x_i - x_{i - 1})f(x_i) \approx \int_0^1 f(x)\mathrm{d}x {% end %}

Using partitions of equal size {% katex() %} \Delta x = \frac{1}{N}, x_i = i\Delta x {% end %} gives the following Javascript code:

```js
function integrate_zero_one(f, n) {
    var sum = 0.0;
    var delta_x = 1/n;

    for (var i = 0; i < n; i++) {
        sum += delta_x * f(i * delta_x);
    }

    return sum;
}
```

Visually, this can be thought of as summing the area of {% katex() %} N {% end %} rectangles of equal width, with height determined by the integrand in question:

<img src="https://upload.wikimedia.org/wikipedia/commons/c/c9/LeftRiemann2.svg"></img>

There are other methods for numerical approximation that work similarly to this basic method (known as a left [Reimann sum](https://en.wikipedia.org/wiki/Riemann_sum)), such as *Newton-Cotes* rules (including the [Trapezoidal rule](https://en.wikipedia.org/wiki/Trapezoidal_rule) and [Simpson's rule](https://en.wikipedia.org/wiki/Simpson%27s_rule)), or [*Gauss-Legendre* rules](https://en.wikipedia.org/wiki/Gaussian_quadrature). Collectively these types of methods are called 'quadrature rules'.

### Problems with Quadrature Rules

Quadrature rules are typically excellen for integrating functions of one variable. However they all suffer from a problem known as the 'curse of dimensionality', meaning that the approximations they give converge extremely slowly to the desired integral in multiple dimensions. See [Veach's thesis](http://graphics.stanford.edu/papers/veach_thesis/thesis.pdf) (section 2.2) for a more rigorous analysis of quadrature rules.

## Monte Carlo Integration

For integration functions of multiple variables, we instead prefer to use a technique called *Monte Carlo Integration*.

### Probability Review

Recall that a continuous random variable {% katex() %} X {% end %} is said to be sampled according to some *probability density function* {% katex() %} p_X(x) {% end %} which gives the relative liklihood of a given value of {% katex() %} X {% end %} being chosen (higher values of {% katex() %} p_X(x) {% end %} means higher liklihood).

The expected value of X is then given by:

{% katex(block=true) %} \mathbb{E}[X] = \int_{-\infty}^{\infty} xp_X(x)\mathrm{d}x {% end %}

The variance of X is given by:

{% katex(block=true) %}
\begin{aligned}
\mathrm{Var}[X] &= \mathbb{E}[(X - \mathbb{E}[X])^2] \\
&= \mathbb{E}[X^2] - (\mathbb{E}[X])^2
\end{aligned}
{% end %}

### Monte Carlo Estimator

Let's say that we want to compute the value of an integral {% katex() %}\int_a^b f(x)\mathrm{d}x {% end %}. Given a set of {% katex() %} N {% end %} random variables {% katex() %} X_i \sim \mathrm{Uniform}(a, b) {% end %}, we know that the quantity

{% katex(block=true) %}
\hat{I}_N = \frac{b-a}{N}\sum_{i=1}^{N}f(X_i)
{% end %}

can be used to approximate the integral. Algebraic manipulation shows that

{% katex(block=true) %}
\begin{aligned}
\mathbb{E}[\hat{I}_N] &= \mathbb{E}\left[\frac{b-a}{N}\sum_{i=1}^{N}f(X_i)\right] \\
&= \frac{b-a}{N}\sum_{i=1}^{N}\mathbb{E}[f(X_i)] \\
&= \frac{b-a}{N}\sum_{i=1}^{N}\int_a^b f(x)p_X(x)\mathrm{d}x \\
&= \frac{b-a}{N}\sum_{i=1}^{N}\int_a^b \frac{f(x)}{b-a}\mathrm{d}x \\
&= \frac{1}{N}\sum_{i=1}^{N}\int_a^b f(x)\mathrm{d}x \\
&= \int_a^b f(x)\mathrm{d}x \\
\end{aligned}
{% end %}

which is exactly what we want! 
In fact, we can sample random variables from *any* distribution - we just need to change our {% katex() %} \hat{I}_N {% end %} slightly. Suppose now that the {% katex() %} X_i {% end %} random variables are sampled from some distribution with a probability density function {% katex() %} p_X(x) {% end %}. Then define the **Monte Carlo estimator** of {% katex() %} f {% end %} with {% katex() %} N {% end %} samples to be:

{% katex(block=true) %}
\hat{I}_N = \frac{1}{N}\sum_{i=1}^{N}\frac{f(X_i)}{p_X(X_i)}
{% end %}

Again, algebraic manipulation shows that:
{% katex(block=true) %}
\begin{aligned}
\mathbb{E}[\hat{I}_N] &= \mathbb{E}\left[\frac{1}{N}\sum_{i=1}^{N}\frac{f(X_i)}{p_X(X_i)}\right] \\
&= \frac{1}{N}\sum_{i=1}^{N}\mathbb{E}\left[\frac{f(X_i)}{p_X(X_i)}\right] \\
&= \frac{1}{N}\sum_{i=1}^{N}\int_a^b \frac{f(x)}{p_X(x)}p_X(x)\mathrm{d}x \\
&= \frac{1}{N}\sum_{i=1}^{N}\int_a^b f(x)\mathrm{d}x \\
&= \int_a^b f(x)\mathrm{d}x \\
\end{aligned}
{% end %}

It will become clear later why it is useful to be able to pick the {% katex() %} X_i {% end %} from any distribution.

Returning to the previous case setting {% katex() %} X_i \sim \mathrm{Uniform}(a, b) {% end %}, we can write some code which computes the value of the Monte Carlo estimator of {% katex() %} f {% end %} with {% katex() %} N {% end %} samples:

```js
function integrate_monte_carlo(f, a, b, n) {
    var sum = 0.0;

    for (var i = 0; i < n; i++) {
        // Obtain x_i ~ Unif(a, b) from Unif(0, 1)
        var x_i = Math.random() * (b - a) + a;
        sum += f(x_i);
    }

    return (b - a) / n * sum;
}

// e.g integrate_monte_carlo(x => (x * x * x), 0.0, 1.0, 100)
```

The Monte Carlo estimator extends easily to multiple dimensions. For example, in three dimensions: let {% katex() %} X_i = (x_i, y_i, z_i) {% end %} be drawn according to a joint probability density function {% katex() %} p_X(x, y, z) {% end %}. Then the Monte Carlo estimator of a function {% katex() %} f(x, y, z) {% end %} is again

{% katex(block=true) %}
\hat{I}_N = \frac{1}{N}\sum_{i=1}^{N}\frac{f(X_i)}{p_X(X_i)}\mathrm{.}
{% end %}

Keep in mind that the Monte Carlo estimator is a function of {% katex() %} N {% end %} - for low values of {% katex() %} N {% end %}, the estimate will be quite inaccurate, but the law of large numbers tells us that as {% katex() %} N {% end %} increases, the estimate gets better and better. It is possible to show analytically that the error of the Monte Carlo estimator (defined by {% katex() %} \hat{I}_N - \mathbb{E}[\hat{I}_N] {% end %}) is {% katex() %} O(N^{-1/2}) {% end %} in any number of dimensions, whereas no quadrature rule has an error better than {% katex() %} O(N^{-1/s}) {% end %} in {% katex() %} s {% end %} dimensions. Again, see [Veach's thesis](http://graphics.stanford.edu/papers/veach_thesis/thesis.pdf) for a more rigorous exploration of these results.

## Variance Reduction Techniques

Ideally we want our estimator {% katex() %} \hat{I}_N {% end %} to give us an accurate result with as small a value of {% katex() %} N {% end %} as possible (since this implies fewer computations). Mathematically speaking, this means we would like {% katex() %} \mathrm{Var}[\hat{I}_N] {% end %} to be as low as possible.

### Importance Sampling

One extremely clever way of reducing the variance of a Monte Carlo estimator is to strategically sample the {% katex() %} X_i {% end %} according to some probability distribution {% katex() %} p_X(x) {% end %} that closely approximates the integrand {% katex() %} f {% end %}. To see why this works, consider picking {% katex() %} p_X(x) = cf(x){% end %} (where {% katex() %} c {% end %} ensures that {% katex() %} p_X {% end %} integrates to 1). Then

{% katex(block=true) %}
\begin{aligned}
\mathrm{Var}[\hat{I}_N] &= \mathrm{Var}\left[\frac{1}{N}\sum_{i=1}^{N}\frac{f(X_i)}{p_X(X_i)}\right] \\
&= \mathrm{Var}\left[\frac{1}{N}\sum_{i=1}^{N}\frac{1}{c}\right] \\
&= \mathrm{Var}\left[\frac{1}{c}\right] \\
&= 0\mathrm{.}
\end{aligned}
{% end %}

This would be the perfect estimator! For all values of {% katex() %} N {% end %}, our estimator gives us the exact value of the integral. However unfortunately, it is not possible to choose such a {% katex() %} p_X {% end %} in the first place, because computing the normalization constant {% katex() %} c {% end %} involves computing the integral of {% katex() %} f {% end %}, which is exactly the thing we're trying to calculate. Also, it may be difficult to sample the {% katex() %} X_i {% end %} from the probability density function if we cannot find an analytic formula for the cumulative distribution function (which prevents [CDF inversion sampling](https://en.wikipedia.org/wiki/Inverse_transform_sampling)).

Therefore we usually have to settle for picking samples from probability density functions which merely approximate the integrand. To think about why this is an improvement over picking from a uniform distribution, imagine that we're trying to integrate a bell curve over a very wide domain. The bell curve is zero *almost everywhere* except near the mean, so most of the uniform samples will result in low values of {% katex() %} f {% end %}.

Alternatively, if we pick more of the samples near the middle of the bell curve, then the large numerator in {% katex() %} \frac{f(X_i)}{p_X(X_i)} {% end %} will be cancelled out by the large denominator (as picking more samples here means a higher pdf). Similarly at the tails, when {% katex() %} f {% end %} is low, this will be cancelled out by a small denominator. So {% katex() %} \frac{f(X_i)}{p_X(X_i)} {% end %} is roughly constant across the domain, giving us a low variance!

Importance sampling is absolutely crucial for reducing the amount of computational work. For example in computer graphics we are often trying to calculate the color of a point on a surface by (very loosely speaking) integrating the energy of light rays arriving at the point in a hemisphere of directions. We know that incoming light rays arriving perpendicular to the surface have a greater effect on its color than light rays arriving parallel to the surface, so we can sample *more* light rays close to the normal of the surface and get a faster converging result!

In another article, I will talk about a technique called *[Multiple Importance Sampling](https://en.wikipedia.org/wiki/Importance_sampling#Multiple_and_adaptive_importance_sampling)* which allows us to combine several different guesses of the shape of our integrand into one single probability density function usable for importance sampling as above.

### Low Discrepancy Sampling

One final technique I will talk about for reducing the variance of our estimator is uniform sample placement.

In addition to importance sampling, it is also intuitive that we would like to explore the domain of our function {% katex() %} f {% end %} as evenly as possible. For example, think back to the case of the Monte Carlo estimator where we have a uniform PDF. It should be clear that we would like our samples to be evenly spaced: that is, they should not be clumped up together (as they would be retrieving values of {% katex() %} f {% end %} that are nearly the same) and similarly they should not be far apart. Relaxing the restriction of having a uniform PDF, the same holds true, though the connection is a little harder to see intuitively.

We can mathematically quantify how 'evenly spaced' the points in a sequence {% katex() %} (x_n) {% end %} are using a measurement called the [Star Discrepancy](https://mathworld.wolfram.com/StarDiscrepancy.html). Using a low discrepancy sequence (LDS) gives us a slightly lower variance (especially for small sample counts) than naive pseudorandom sampling.

The image below uses pseudorandom samples to generate the coordinates of each point.

<center><img src="https://upload.wikimedia.org/wikipedia/commons/thumb/a/a4/Pseudorandom_sequence_2D.svg/300px-Pseudorandom_sequence_2D.svg.png"></img></center>

As you can see, there are areas of higher point density and lower point density. This results in high variance when doing Monte Carlo integration. The image below instead uses a low discrepancy sequence called a [Sobol sequence](https://en.wikipedia.org/wiki/Sobol_sequence) for each coordinate:

<center><img src="https://upload.wikimedia.org/wikipedia/commons/thumb/f/f1/Sobol_sequence_2D.svg/300px-Sobol_sequence_2D.svg.png"></img></center>

The points are much more evenly distributed throughout the image which is what we want.

One simplest such low discrepancy sequence is called the [van der Corput sequence](https://en.wikipedia.org/wiki/Van_der_Corput_sequence). The base-{% katex() %} b {% end %} van der Corput sequence is defined by:

{% katex() %}
x_n = \sum_{k=0}^{\infty} d_k(n)b^{-k-1}
{% end %}

where {% katex() %}d_k(n){% end %} is the {% katex() %} k {% end %}th digit of the expansion of {% katex() %} n {% end %} in base {% katex() %} b {% end %}. With {% katex() %} b = 2 {% end %}, the sequence begins {% katex() %} 0.5, 0.25, 0.75, 0.126 \ldots {% end %}

## Further Reading

* [Eric Veach's thesis](http://graphics.stanford.edu/papers/veach_thesis/thesis.pdf) is a truly excellent resource and covers everything here and more in greater detail (mainly section 2)
* [Physically Based Rendering: From Theory to Implementation](http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration.html) chapter 10 covers Monte Carlo Integration. [Chapter 7](http://www.pbr-book.org/3ed-2018/Sampling_and_Reconstruction.html) describes the theory and implementation behind a number of low discrepancy sequences
