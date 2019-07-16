+++
title = "Why we need alternatives to Actix"
date = 2019-07-16

[taxonomies]
categories = ["Rust"]
+++

*Warning: unapologetic rant ahead.*

Don't get me wrong - I actually really like [`actix-web`](https://github.com/actix/actix-web). It's got a simple and innovative API, a reasonably sized ecosystem of [crates](https://crates.io/search?q=actix) and [examples](https://github.com/actix/examples) (at least compared to other Rust web frameworks), [real world usage](https://www.reddit.com/r/rust/comments/cdg5b4/rust_in_the_on_of_the_biggest_music_festival/) - and notably - it's fast. [Very fast](https://www.techempower.com/benchmarks/#section=data-r18&hw=ph&test=fortune). Despite these things, I'm going to try and spell out why I don't think it can be *the* framework of choice for the Rust community moving forward.
<!-- more -->

## Belly of the Beast

Does anyone remember when actix used to have [over 100 instances of unsafe](https://github.com/actix/actix-web/issues/289), largely used willy-nilly? Most of the egregious usages have been fixed a while ago, but there's still about 25 instances of it. Looking through the code now, there's still some worrying bits. Here's [one example](https://github.com/actix/actix-web/blob/d286ccb4f5a86eca12c65b1632506a8bd8b37d19/actix-http/src/helpers.rs#L116):

```rust
pub(crate) fn convert_usize(mut n: usize, bytes: &mut BytesMut) {
    let mut curr: isize = 39;
    let mut buf: [u8; 41] = unsafe { mem::uninitialized() };
    buf[39] = b'\r';
    buf[40] = b'\n';
    let buf_ptr = buf.as_mut_ptr();
    let lut_ptr = DEC_DIGITS_LUT.as_ptr();

    // eagerly decode 4 characters at a time
    while n >= 10_000 {
        let rem = (n % 10_000) as isize;

	// ...snip

        unsafe {
            ptr::copy_nonoverlapping(lut_ptr.offset(d1), buf_ptr.offset(curr), 2);
            ptr::copy_nonoverlapping(lut_ptr.offset(d2), buf_ptr.offset(curr + 2), 2);
        }
    }

    // another 5 similar uses of unsafe below...
}
```

There's a couple of reasons that code like this worries me, and probably should worry you too. First is the use of [`std::mem::uninitialized()`](https://doc.rust-lang.org/std/mem/fn.uninitialized.html), which is now deprecated in favour of [`std::mem::MaybeUninit`](https://doc.rust-lang.org/std/mem/union.MaybeUninit.html) instead. The short of it is that it was never entirely clear that `mem::uninitialized()` could be used without UB, even for 'all bit-patterns are valid' types like `u8`. See [Gankro's post](https://gankro.github.io/blah/initialize-me-maybe/) and [Ralf J's post](https://www.ralfj.de/blog/2019/07/14/uninit.html) on uninitialised[^1] memory for more details.

Secondly (and probably the main reason for disliking this code) is that there's absolutely *no justification given* for why this is done. Why is `Display` not simply being used? Is this function even performance-critical? Is there any real benefit to using `ptr::copy_nonoverlapping` for **2 bytes** instead of just (safely) assigning the bytes like `buf[curr] = lut[d1]`? With `ptr::copy_nonoverlapping` there's a number of variants you need to uphold, including making sure that the arguments don't alias (i.e overlap), and making sure the buffers are large enough for the read/write - but neither of these are mentioned.

In [other places](https://github.com/actix/actix-web/blob/f27beab016de18577de0818a5802829b31da96b1/actix-http/src/h1/encoder.rs#L135), there's bits of code like this, which are probably enough to instill fear in people who don't know any Rust at all:

```rust
let mut buf = unsafe { &mut *(dst.bytes_mut() as *mut [u8]) };
```

You might be thinking 'somebody used unsafe a bit too much, what's the big deal here? Just fix it and move on.'. The problem I'm trying to highlight here is that there's a fundamental issue with the author's attitude which we don't seem to be able to change.

For example, you'd think that after the previous [debacle](https://github.com/actix/actix-web/issues/289), Nikolay (the author) would have learnt his lesson about `unsafe`. But apparently this is not the case. Recently, GitHub user [Aaron101](https://github.com/aaron101) opened a PR on actix [removing several instances of `unsafe`](https://github.com/actix/actix-web/pull/968). The changes basically boiled down to replacing a use of `UnsafeCell` with `Cell` / `RefCell` - changes which should have absolutely no observable impact on performance. Aaron101 explained that, as it currently stands, the current interface (though keep in mind this is **not** part of the public API) makes it possible to trigger UB in only a few lines of code.

The reasonable and responsible thing to do here is to merge. But instead, Nikolay [responded with](https://github.com/actix/actix-web/pull/968#issuecomment-509894555):

>"I guess everybody should switch to interpreted language otherwise we will die in ub"
>
>*PR closed by @fafhrd91*

Not exactly the kind of attitude you'd want from the maintainer of a top web framework with 425k downloads.

Another example of (in my opinion) poor attitude was the release of `actix-web 1.0.2`, which was actually a breaking change (some code was refactored into a separate crate). At least one person has been caught off-guard by this, and [when asked to yank and add a re-export to maintain compatability](https://github.com/actix/actix-web/issues/953), Nikolay responded with simply "i dont see reason" and closed the issue. Again, it's not a huge problem in itself, but I'd consider it mildly irresponsible and a demonstration of poor attitude.

## Flying Solo

Let me give a little bit of backstory here. I was working on a project and decided to go with the asynchronous `tokio-postgres` driver (which is fantastic, by the way). Unfortunately there was a little issue: connecting to the database is *asynchronous* (it returns a future), yet actix only supports *synchronous* creation of application state. I asked on gitter and was given a little bit of (admittedly confusing) advice on how to go about creating state asynchronously, along with: "it is complicated, I'll try to implement it soon".

As I didn't really have any better ideas, I opened up the repo to see if I could implement it myself. I was pointed to [this](https://github.com/actix/actix-net/blob/da302d4b7a3faaeb5d041f8480d4437113d2e0d5/actix-server/src/services.rs#L26) code:
```rust
pub trait ServiceFactory: Send + Clone + 'static {
	type NewService: NewService<Config = ServerConfig, Request = Io<TcpStream>>;

	fn create(&self) -> Self::NewService;
}

pub(crate) trait InternalServiceFactory: Send {
	fn name(&self, token: Token) -> &str;

	fn clone_factory(&self) -> Box<InternalServiceFactory>;

	fn create(&self) -> Box<Future<Item = Vec<(Token, BoxedServerService)>, Error = ()>>;
}
```

Right off the bat I'm a bit confused. Why are there two traits here? They don't even appear connected in any meaningful way. Ignoring that, it looks like this `ServiceFactory` trait allows you to construct a `NewService`. Let's open that up next. We can look at the `docs.rs` for this one, since it's part of the public API. Oh wait, we can't look it on `docs.rs`, because it's part of the `actix-net` crate, and `docs.rs` [doesn't believe that it's a library](https://docs.rs/crate/actix-net/0.3.0). Not to worry, let's open up the copy of the documentation on `actix.rs` (which doesn't even seem to be linked from the main `actix.rs` site - there's a link to it on the GitHub page):

> **Trait `actix_net::service::NewService`**
>
> Creates new Service values. Acts as a service factory. This is useful for cases where new Service values must be produced.

Hang on a second... so the `ServiceFactory::create()` method returns an item implementing `NewService`, and `NewService` supposedly acts as a 'service factory'? So `NewService` should really be `ServiceFactory`, and `ServiceFactory` should really be `ServiceFactoryFactory`. Got it. There's also [`IntoNewService`](https://actix.rs/actix-net/actix_net/service/trait.IntoNewService.html), which presumably does a similar thing to `ServiceFactory` except it gets consumed. Then there's [`NewServiceExt`](https://actix.rs/actix-net/actix_net/service/trait.NewServiceExt.html) (I have absolutely no idea why you'd need an `Ext` trait here). It gets even better - there's *another* internal trait called `ServiceFactory`, this time inside the `actix-web` crate itself, with a single mysterious method that doesn't on the face of it appear to 'create' anything (as a factory should): `fn register(&mut self, config: &mut AppService);`. We've also got two different `HttpServiceFactory` traits, one in the `actix-web` crate and one in the `actix-framed` crate, both internal and completely unrelated...

I don't think I need to explore this rabbit hole much further to demonstrate what kind of mess we're dealing with here. I genuinely spent a good few hours looking at these parts of the code, trying to figure out how in hell this was going to help me connect to my database driver asynchronously before the server starts up. Even looking at it starting from the public API was incredibly confusing - there's about four or five different `NewService` / `IntoNewService` / `Service` layers that the user's config goes through before requests start getting handled.

Ultimately, I found a way to sidestep the problem entirely by running the future to completion before starting the server, using a function in `actix_rt`, which seems to be actix's wrapper around a tokio executor runtime. Now, granted, I'm sure a more experienced Rust developer would have been able to find their way around the codebase much quicker than me, but I don't think it's unfair to call actix a mess. I should add that Nikolay recently implemented the feature in question about a month later, with apparent ease.

The overall point I'm trying to make here is this: actix isn't something that you can easily contribute to. Ultimately, this makes us reliant on Nikolay for new features which is a *very* undesireable trait for a large software project.

## Blazingly fast... or not?

Actix is doing very well in the latest round of the [TechEmpower Web Framework Benchmarks](https://www.techempower.com/benchmarks/#section=data-r18). It's leading in four of the six benchmarks, and among the top 5 in the remaining two benchmarks. While clearly an excellent result, there's some questionable behaviour going on behind the scenes in a couple of them.

Specifically, in the 'plaintext' and 'JSON' benchmarks we can see that actix is doing some manual HTTP parsing, hardcoding header values (while ignoring the HTTP method). It doesn't look even *close* to the kind of code you'd write in real life ([see for yourselves](https://github.com/TechEmpower/FrameworkBenchmarks/blob/3129ff5f0cd36794a5bf0cca6e8d4b01eeb841f4/frameworks/Rust/actix/src/main_raw.rs#L26)):

```rust
impl<T: AsyncRead + AsyncWrite> Future for App<T> {
    type Item = ();
    type Error = ();

    fn poll(&mut self) -> Poll<Self::Item, Self::Error> {
        loop {
            if self.read_buf.remaining_mut() < 4096 {
                self.read_buf.reserve(32_768);
            }
            let read = unsafe { self.io.read(self.read_buf.bytes_mut()) };
            match read {
                Ok(0) => return Ok(Async::Ready(())),
                Ok(n) => unsafe { self.read_buf.advance_mut(n) },
                Err(e) => {
                    if e.kind() == io::ErrorKind::WouldBlock {
                        break;
                    } else {
                        return Err(());
                    }
                }
            }
        }
	// ...snip
    }
}
```

Another problem is that actix doesn't seem to check the HTTP method at all, it just looks at the path and assumes (based on the benchmark specification) what it needs to do.

In fairness, it seems other benchmarks are doing these things (hyper even does this - though [Sean acknowledges](https://github.com/TechEmpower/FrameworkBenchmarks/blob/master/frameworks/Rust/hyper/src/main.rs#L28) it's a bit of a scummy tactic). The authors of Gotham and Thruster [make a point](https://www.reddit.com/r/rust/comments/cd9asm/announcing_gotham_v04/etteye7/) not to do this, for what it's worth. More commentary on the benchmarks from Gotham author can be found [here](https://www.reddit.com/r/rust/comments/cd9asm/announcing_gotham_v04/ett80qm/). I would strongly urge someone to take a proper look at these benchmarks and notify TechEmpower that they're being deceived - there's a special 'stripped' classification for framework applications which have been unrealistically crafted for maximum benchmark score - actix, hyper and many others probably deserve this. 

Please don't misconstrue this as me arguing that actix is slow - it's definitely not - I'm just trying to point out that the TechEmpower benchmarks don't tell the whole story!

## Anything else, while we're at it?

Actix has 221 dependencies in its tree at the time of writing. This leads to pretty horrible compile times, to be brutally honest. In one project which uses actix, my compile times are up to around 23 seconds for an incremental change in debug mode, which I find nearly unbearable. Thank God for `cargo check` (`rls` also seems to shit the bed a little bit in this project). I would really like to see more effort spent reducing compile times. (Side rant: I reckon if Rust used `lld` then I'd save around 5-7 seconds per compile on that project, but I haven't been able to get it working on my machine).

Last minor gripe: documentation. The API documentation is actually reasonably good, although there's still some rough edges. For example, there's a trait called [`ResponseError`](https://docs.rs/actix-web/1.0.3/actix_web/trait.ResponseError.html) which allows you to convert from your custom error type into a `HttpResponse` that gets sent to the user (useful if you want to have your routes return something like `Result<HttpResponse, MyError>`). There's two functions on it: `response_error` and `error_response`. Feel free to figure out what the difference is between them, because the documentation and naming is entirely unhelpful! Turns out that if you use the `error_response` function then the body of whatever you return is totally ignored when sending to the user, and I have absolutely no idea why.

Aside from the fact that the other parts of `actix-web`'s web of crates (like `actix-net`) are barely documented at all, the API documentation is fairly solid. There's a fair few examples, though most use outdated mechanisms and some don't even compile. A bigger issue is that it's taken over a month for the [actix user guide](https://actix.rs/docs/getting-started/) to be updated to 1.0, and it's not even done yet! Why Nikolay would push a 1.0 release without updating the user guide I do not know. GitHub user [cldershem](https://github.com/cldershem) has put in a [monumental effort](https://github.com/actix/actix-website/pull/90) updating the documentation by themselves thus far.

## Conclusion

TL;DR: actix isn't really a sustainable framework for the Rust community to rally around (as much as I'd like it to be). There's a number of red flags; mainly the author's mentality and internal code quality. As such, I won't be using actix in the future, and I think it's time we looked elsewhere.

## What are the alternatives?

Sorry for the downbeat tone - don't despair! Here are some promising alternatives, all of which are currently being updated and maintained:
- [Rocket](https://rocket.rs/) - Probably the most mature Rust web framework of all. Dead simple API, but lacking in performance, which will hopefully improve when the framework moves over to Futures and async/await
- [Gotham](https://gotham.rs/) - Still fairly new, but rapidly improving! Good support for async, recently added TLS and diesel middleware support in 0.4.
- [Thruster](https://github.com/trezm/Thruster) - Fast middleware-based framework (think express, koa). Still very new but author is friendly and willing to help. Totally unsafe-free!
- [Warp](https://github.com/seanmonstar/warp) - Lightweight composable framework using middleware 'filters', built on top of hyper
- Please leave a comment (here or on reddit) if you feel I've left something off this list

<hr>

UPDATE 1: Replaced some bits from the benchmarks section. I was a little too harsh with my initial assessment.

[^1]: Yes, I'm a Brit, and I'm bloody-well going to switch to spelling uninitialised with an 's' when you're not looking.
