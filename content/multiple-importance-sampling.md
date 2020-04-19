+++
title = "Multiple Importance Sampling"
date = 2020-04-19

[taxonomies]
categories = ["Graphics", "Maths"]
+++

In the previous article I described [Importance Sampling](@/monte-carlo.md), a method for reducing the variance of a Monte Carlo estimator. In this article I will discuss a technique called *Multiple Importance Sampling* which allows us to combine samples from multiple different probability distributions that we think match the shape of the integrand, reducing variance without introducing bias.
<!-- more -->

Let's say we're trying to estimate the integral of a product of two functions, {% katex() %} I = \int g(x)h(x)\mathrm{d}x {% end %}. Suppose we have two probability density functions, {% katex() %} p_g(x) {% end %} and {% katex() %} p_h(x) {% end %} which have a similar shape to {% katex() %} g {% end %} and {% katex() %} h {% end %} respectively. It is not obvious how we should sample and weight the random variables to construct an unbiased estimator of {% katex() %} I {% end %} given this information.

## Trying Some Stuff

One way to do this would be to importance sample strictly according to {% katex() %} p_g {% end %}, completely ignoring {% katex() %} p_h {% end %}:

{% katex(block=true) %}
\begin{aligned}
\tag{1} \hat{I}_N = \frac{1}{N}\sum_{i=1}^{N}\frac{g(X_i)h(X_i)}{p_{g}(X_i)}
\end{aligned}
{% end %}


This works well if {% katex() %} h {% end %} is approximately constant over the domain, as the shape of the integrand {% katex() %} g(x)h(x) {% end %} will match the shape of {% katex() %} g(x) {% end %} closely. However if this assumption does not hold, then the scheme will be poor.

Another idea would be to construct a probability density function {% katex() %} p_{gh}(x) \propto p_g(x)p_h(x) {% end %}, and use importance sampling as before, picking the {% katex() %} X_i {% end %} according to {% katex() %} p_{gh} {% end %}:

{% katex(block=true) %}
\begin{aligned}
\tag{2} \hat{I}_N = \frac{1}{N}\sum_{i=1}^{N}\frac{g(X_i)h(X_i)}{p_{gh}(X_i)}
\end{aligned}
{% end %}

This is totally reasonable, but in many cases it is not computationally feasible to sample random variables from {% katex() %} p_{gh} {% end %}. Typically, sampling random variables from a given PDF involves [inverse transform sampling](https://en.wikipedia.org/wiki/Inverse_transform_sampling), which is only possible if you can compute the inverse CDF.

Instead of trying to construct a single PDF, we can sample from each distribution, then average the samples:

{% katex(block=true) %}
\begin{aligned}
\tag{3} \hat{I}_N = \frac{1}{2N}\sum_{i=1}^{N}\frac{g(X_{g,i})h(X_{g,i})}{p_g(X_{g,i})} + \frac{1}{2N}\sum_{i=1}^{N}\frac{g(X_{h,i})h(X_{h,i})}{p_h(X_{h,i})}
\end{aligned}
{% end %}

where the {% katex() %} X_{g,i} {% end %} are picked according to {% katex() %} p_g {% end %}, and the {% katex() %} X_{h,i} {% end %} according to {% katex() %} p_h {% end %}. Since each term of this sum is a Monte Carlo estimator with expected value {% katex() %} I/2 {% end %}, we sum these to obtain an unbiased estimator. Note that this means we are now evaluating the integrand {% katex() %} 2N {% end %} times, with {% katex() %} N {% end %} samples being taken according to each of {% katex() %} p_g {% end %} and {% katex() %} p_h {% end %}.

The problem with this method is that it can lead to unnecessarily high variance. To see this, consider the variance of the estimator:

{% katex(block=true) %}
\begin{aligned}
\mathrm{Var}[\hat{I}_N] &= \frac{1}{4N^2}\sum_{i=1}^{N}\mathrm{Var}\left[\frac{g(X_{g,i})h(X_{g,i})}{p_g(X_{g,i})} + \frac{g(X_{h,i})h(X_{h,i})}{p_h(X_{h,i})}\right] \\
&= \frac{1}{4N}\left(\mathrm{Var}\left[\frac{g(X_{g,i})h(X_{g,i})}{p_g(X_{g,i})}\right] + \mathrm{Var}\left[\frac{g(X_{h,i})h(X_{h,i})}{p_h(X_{h,i})}\right]\right) \\
\end{aligned}
{% end %}

Since the variance is additive, if *either* {% katex() %} p_g {% end %} or {% katex() %} p_h {% end %} is a bad match for the integrand, we will get high variance - even if the other PDF is a perfect match.

## Generalizing

Let's try and think about the problem a bit more generally. The last technique was definitely along the right lines. 

### Weights

It seems reasonable to explore estimators of the form

{% katex(block=true) %}
\begin{aligned}
\tag{4} \hat{I}_N = \frac{1}{N}\sum_{i=1}^{N}w_g(X_{g,i})\frac{g(X_{g,i})h(X_{g,i})}{p_g(X_{g,i})} + \frac{1}{N}\sum_{i=1}^{N}w_h(X_{h,i})\frac{g(X_{h,i})h(X_{h,i})}{p_h(X_{h,i})}
\end{aligned}
{% end %}

where {% katex() %} w_g(x), w_h(x) {% end %} are some 'weighting' functions. In strategy {% katex() %} (1) {% end %}, we tried {% katex() %} w_g(x) = 1, w_h(x) = 0 {% end %}, and in strategy {% katex() %} (3) {% end %} we tried {% katex() %} w_g(x) = w_h(x) = 1/2 {% end %}.

The notation at this point is starting to get a bit heavy, so try to keep in mind that we're looking for some expression {% katex() %} \hat{I}_N {% end %} such that {% katex() %} \mathbb{E}[\hat{I}_N] = \int g(x)h(x)\mathrm{d}x {% end %}. Clearly not all choices of weighting functions leave us with an unbiased estimator. To see what constraints are required on the weighting functions to keep the estimator unbiased, consider the expected value of {% katex() %} (4) {% end %}:

{% katex(block=true) %}
\begin{aligned}
\mathbb{E}[\hat{I}_N] &= \frac{1}{N}\sum_{i=1}^{N}\mathbb{E}\left[w_g(X_{g,i})\frac{g(X_{g,i})h(X_{g,i})}{p_g(X_{g,i})} + w_h(X_{h,i})\frac{g(X_{h,i})h(X_{h,i})}{p_h(X_{h,i})}\right] \\
&= \int w_g(x)\frac{g(x)h(x)}{p_g(x)}p_g(x) + w_h(x)\frac{g(x)h(x)}{p_h(x)}p_h(x)\mathrm{d}x \\
&= \int \left(w_g(x) + w_h(x)\right)g(x)h(x)\mathrm{d}x
\end{aligned}
{% end %}

Therefore we can see that we require {% katex() %} \forall x, w_g(x) + w_h(x) = 1 {% end %} for the estimator to remain unbiased. It is also required that {% katex() %} w_g(x) = 0 {% end %} whenever {% katex() %} p_g(x) = 0 {% end %}, so as to avoid a division by zero when evaluating the first term of {% katex() %} \hat{I}_N {% end %} (and likewise for {% katex() %} w_h(x) {% end %} and the second term).

### Sampling Techniques

Currently we've been sampling from 2 PDFs (let's call each of these a 'sampling technique'), but we can generalise this. Let {% katex() %} K {% end %} be the number of sampling techniques, with PDFs {% katex() %} p_1, p_2, \dots, p_K {% end %} that each match the shape of the integrand {% katex() %} g(x)h(x) {% end %} in some way. Let {% katex() %} X_{j, i} {% end %} be sampled according to {% katex() %} p_j {% end %}. Given some set of weighting functions {% katex() %} w_j {% end %}, we can then rewrite {% katex() %} (4) {% end %} as:

{% katex(block=true) %}
\begin{aligned}
\tag{5} \hat{I}_N = \sum_{j=1}^{K}\frac{1}{N}\sum_{i=1}^{N}w_j(X_{j,i})\frac{g(X_{j,i})h(X_{j,i})}{p_i(X_{j,i})} 
\end{aligned}
{% end %}

Note that the order of summation is not important here. We still require the two properties from before: {% katex() %} \forall x, \sum_{j=1}^{K} w_j(x) = 1  {% end %}, and {% katex() %} p_j(x) = 0 \implies w_j(x) = 0 {% end %}.

### Sample Counts

So far we've been taking exactly {% katex() %} N {% end %} samples from each sampling technique, resulting in {% katex() %} KN {% end %} samples in total. This can be generalised to allow greater or fewer samples for each sampling technique.

Let {% katex() %} n_j {% end %} the number of samples taken for sampling technique {% katex() %} j {% end %} (so that the **total** number of samples used by the estimator is {% katex() %} \sum_j n_j = N {% end %}). Then:

{% katex(block=true) %}
\begin{aligned}
\tag{6} \hat{I}_N = \sum_{j=1}^{K}\frac{1}{n_j}\sum_{i=1}^{n_j}w_j(X_{j,i})\frac{g(X_{j,i})h(X_{j,i})}{p_i(X_{j,i})} 
\end{aligned}
{% end %}

The proof that this is results in an unbiased estimator is trivial and left as an exercise to the reader.

## Weighting Heuristics

Now that I've introduced the general MIS formulation, we can discuss some different choices of weighting functions.

### Balance Heuristic

Veach determined that the following choice of weights is reasonable:

{% katex(block=true) %}
\begin{aligned}
w_j(x) = \frac{n_jp_j(x)}{\sum_{k=1}^{K} n_kp_k(x)}
\end{aligned}
{% end %}

When inserted into the MIS definition {% katex() %} (6) {% end %}, this gives:

{% katex(block=true) %}
\begin{aligned}
\hat{I}_N &= \sum_{j=1}^{K}\frac{1}{n_j}\sum_{i=1}^{n_j}\left(\frac{n_jp_j(X_{j,i})}{\sum_{k=1}^{K} n_kp_k(X_{j,i})}\frac{g(X_{j,i})h(X_{j,i})}{p_i(X_{j,i})}\right) \\
\tag{7} &= \sum_{j=1}^{K}\sum_{i=1}^{n_j}\frac{g(X_{j,i})h(X_{j,i})}{\sum_{k=1}^{K} n_kp_k(X_{j,i})}
\end{aligned}
{% end %}

Veach shows that (assuming positive weights), the balance heuristic is close to optimal. Specifically, with the balance heuristic:

{% katex(block=true) %}
\begin{aligned}
\mathrm{Var}[\hat{I}_N] - \mathrm{Var}[I] \le \left(\frac{1}{\mathrm{min}_j n_j} - \frac{1}{\sum_j n_j}\right)\mathbb{E}[I]^2
\end{aligned}
{% end %}

That is to say, even using the most optimal choice of weights, the variance differs by at most the term on the right hand side. A proof of this is given in the [Chapter 9 Appendix](https://graphics.stanford.edu/courses/cs348b-03/papers/veach-chapter9.pdf) of Veach's thesis.

We can interpret the balance heuristic {% katex() %} (7) {% end %} in a more natural way by rewriting it slightly. Let {% katex() %} c_j = n_j/N {% end %}, i.e the fraction of samples given to technique {% katex() %} j {% end %}. Then:

{% katex(block=true) %}
\begin{aligned}
\hat{I}_N &= \sum_{j=1}^{K}\sum_{i=1}^{n_j}\frac{g(X_{j,i})h(X_{j,i})}{\sum_{k=1}^{K} n_kp_k(X_{j,i})} \\
&= \frac{1}{N}\sum_{i=1}^{n_j}\sum_{j=1}^{K}\frac{g(X_{j,i})h(X_{j,i})}{\sum_{k=1}^{K} c_kp_k(X_{j,i})}
\end{aligned}
{% end %}

Note that I have swapped the order of summation on the second line. One may notice that this resembles a standard Monte Carlo estimator with probability density function {% katex() %} \hat{p}(x) = \sum_j c_jp_j(x) {% end %} which Veach calls the 'combined sample density'. This represents the distribution of a random variable that is equal to each {% katex() %} X_{j,i} {% end %} with probability {% katex() %} 1/N {% end %}.

### Cutoff Heuristic

The balance heuristic can introduce unnecessary variance in the case that one of the sampling techniques is a very close match to the integrand (this is closer to what typically happens in computer graphics). The reason for this is explored in more detail in [Veach's thesis](http://graphics.stanford.edu/papers/veach_thesis/thesis.pdf) on pages 270-273.

The way to address this is to 'sharpen' the weighting functions - i.e increase the value of high weights while decreasing the value of low weights. For brevity,we will drop the argument {% katex() %} x {% end %} and write {% katex() %} q_j = n_jp_j {% end %}. Veach defines the cutoff heuristic with parameter {% katex() %} 0 \le \alpha \le 1 {% end %} by:

{% katex(block=true) %}
w_j(x) = 
\begin{cases}
\frac{q_j}{\sum_{k=1}^{N}\{q_k | q_k \ge \alpha q_{\text{max}}\}} &\text{if } q_j \ge \alpha q_{\text{max}} \\
0 &\text{otherwise}
\end{cases}
{% end %}

This has the effect of 'cutting off' any samples that are generated with a PDF less than {% katex() %} \alpha {% end %} (and therefore increasing the value of the other weights so as to sum to 1). In the case of {% katex() %} \alpha = 0 {% end %} this reduces to the balance heuristic.

### Power Heuristic

The power heuristic with parameter {% katex() %} \beta \ge 1 {% end %} is defined as:

{% katex(block=true) %}
\begin{aligned}
w_j(x) = \frac{q_j^\beta}{\sum_{k=1}^{K} q_k^\beta}
\end{aligned}
{% end %}

In the case of {% katex() %} \beta = 1 {% end %}, this also reduces to the balance heuristic. For larger values of {% katex() %} \beta {% end %}, more sharpening occurs. Veach determined empirically that the value {% katex() %} \beta = 2 {% end %} gives good results in typical scenes.

## Conclusion

Although I have not directly linked multiple importance sampling to computer graphics (see [this PBRT section](http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/Importance_Sampling.html#MultipleImportanceSampling) for a brief summary), MIS is such a crucial technique for reducing variance in offline rendering that it was mentioned as a reason for Eric Veach's technical Oscar. You might be amused to [watch the video](https://www.youtube.com/watch?v=e3ss_Ozb9Yg) of him going up on stage and receiving the award from Michael B. Jordan and Kristen Bell, who clearly have no understanding of the words they are reading off the teleprompter.

### Further Reading

There is a reasonable amount of literature surrounding MIS that is worthy of note. [Cornuet et al. (2012)](https://www.jstor.org/stable/23357226) describe a method for *adaptive multiple importance sampling* that effectively modify the weights according to samples obtained at runtime. [Kondapaneni et al. (2019)](https://graphics.cg.uni-saarland.de/papers/konda-2019-siggraph-optimal-mis.pdf) revisit some of Veach's derivations, showing that the variance of the balance estimator can be improved upon further than the bound given by Veach if negative weights are allowed. They also describe a technique for estimating the most optimal MIS weights, since they cannot be computed directly.

[Sbert et al. (2016)](https://onlinelibrary.wiley.com/doi/epdf/10.1111/cgf.13042) define an estimator based on the 'one sample model' of MIS that Veach describes but I have omitted. This involves randomly selecting one of the proposal distributions to sample from, rather than sampling from them all at the same time and combining their contributions. Their method is provably better than the balance heuristic for the same number of samples.

### References

* [Eric Veach's thesis, Chapter 9](http://graphics.stanford.edu/papers/veach_thesis/thesis.pdf)
* [Physically Based Rendering: From Theory to Implementation, Section 13.10](http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/Importance_Sampling.html#MultipleImportanceSampling)
