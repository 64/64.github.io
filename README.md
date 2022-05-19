# Blog

First, run `git submodule update --init` to grab the `even` theme.

To build, you will need [my fork of Zola](https://github.com/64/zola) which adds KaTeX rendering on the server side. Compile that somewhere (say, in `../zola`), then run:

```
../zola/target/debug/zola serve
```
