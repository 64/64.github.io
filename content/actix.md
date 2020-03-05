+++
title = "Why we need alternatives to Actix"
date = 2019-07-16

[taxonomies]
categories = ["Rust"]
+++

Don't get me wrong - I actually really like [`actix-web`](https://github.com/actix/actix-web). It's got a simple and innovative API, a reasonably sized ecosystem of crates and [examples](https://github.com/actix/examples) (at least compared to other Rust web frameworks), [real world usage](https://www.reddit.com/r/rust/comments/cdg5b4/rust_in_the_on_of_the_biggest_music_festival/) - and notably - it's fast. [Very fast](https://www.techempower.com/benchmarks/). Despite these things, I'm going to try and spell out why I don't think it can be *the* framework of choice for the Rust community moving forward.
<!-- more -->

---

UPDATE 1: Replaced some bits from the benchmarks section.

UPDATE 2 \[2019-08-01\]: Wow, this [blew up](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/) a lot more than I expected. A lot of things have been said (mostly [on reddit](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/)), so I'm going to take a moment to respond and clarify, in a sort of Q&A format:

**Q:** There is no need to be a dick. Why the mocking tone?<sup>[1](https://twitter.com/mgattozzi/status/1151982658173513728), [2](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etxlwtr/), [3](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/eu13bog/)</sup>\
**A:** This is certainly my biggest regret of the whole situation. It was absolutely not my intention to be mocking or insulting, but on the other hand I can totally see how this post may come across as such. In particular, the [Flying Solo](#Flying_Solo) section had a poor tone, and hence I have removed it. For what it's worth, the point of this section was **not** to randomly mock the framework or maintainer, but was to demonstrate the barriers that new contributors face, which is a bad thing for the longevity of any software project.

**Q:** Criticising code is one thing, but why the personal angle? That clearly crosses a line.<sup>[1](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/eu3bkgq/), [2](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etxqj6n/)</sup>\
**A:** I think [BurntSushi](https://github.com/BurntSushi) answered this in a much better way than I could:
> Yes, I agree the situation is unfortunate. It's no fun being at the bad end of a mob. I think most people are being pretty polite relative to how the rest of the Internet behaves, but this is a thorny issue. I've said in the past (outside the context of actix) that the people behind a project are fair game for evaluating whether to bring in a dependency or not. There's trust, reputation and good judgment that are hard to quantify, but are nevertheless important qualitative metrics. You hear the positive side of this a lot, e.g., "burntsushi's crates are always good." But the negative side is... ugly, because it's really hard to straddle that line between being rude/unfair and lodging legitimate concerns with one's qualitative confidence in a maintainer. And then when you throw in the fact that this is a volunteer effort... It's tough. And unfortunately, that's exactly what's happening here.
> -- [/u/BurntSushi on reddit](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/ety4hkz/)

(Please do not misconstrue this as BurntSushi endorsing everything I or anyone else has said on the issue.)

**Q:** None of the code shown actually contains any UB so what is the fuss about? <sup>[1](https://twitter.com/withoutboats/status/1151539484414136321), [2](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etz92ze/)</sup>\
**A:** [Instances](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etxlqot/) of UB which can be triggered through the public API have since been pointed out elsewhere. But my main problem is with the somewhat careless nature towards `unsafe` code. In the standard library, and many others, you are always encouraged to add a comment documenting which invariants the `unsafe` block requires, and how exactly they are upheld. Even the extra effort required for this should be enough to discourage you from using `unsafe` unnecessarily. Personally I find it quite worrying that this sort of attitude is prominent in such a popular and security-critical crate.

**Q:** Do you believe that nobody should ever use `unsafe`? <sup>[1](https://twitter.com/withoutboats/status/1151537402265198605), [2](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etxkx4q/)</sup>\
**A:** No, of course not. `unsafe` serves a purpose and is often needed for performance reasons or just to shorten the amount of code required. Both of these use cases are totally valid. However, many of the uses I am seeing in actix are entirely unnecessary, such as the first block I showed which appears to be able to be rewritten using the normal `Display` implementation while *gaining speed*. Further, the security critical nature of this crate is another reason why it's bad to have unnecessary unsafe lying about. For things such as CLI tools or games, there is less of a need to be so acutely aware of unsafety.

**Q:** If you dislike actix so much, why not just use something else instead of bashing it? <sup>1 dead, [2](https://www.reddit.com/r/rust/comments/ce09id/why_we_need_alternatives_to_actix/etxlwtr/)</sup>\
**A:** That is pretty much what I am recommending to people. I think it is totally fair to notify people of these concerns and let them decide if they want to keep using the framework. I know a lot of people have read this post and decided to keep using actix anyway, which I have no problem with.

Anyway, onwards with the rest.

---

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

There's a couple of reasons that code like this worries me, and probably should worry you too. First is the use of [`std::mem::uninitialized()`](https://doc.rust-lang.org/std/mem/fn.uninitialized.html), which is now deprecated in favour of [`std::mem::MaybeUninit`](https://doc.rust-lang.org/std/mem/union.MaybeUninit.html) instead. The short of it is that it was never entirely clear that `mem::uninitialized()` could be used without UB, even for 'all bit-patterns are valid' types like `u8`. See [Gankro's post](https://gankra.github.io/blah/initialize-me-maybe/) and [Ralf J's post](https://www.ralfj.de/blog/2019/07/14/uninit.html) on uninitialised[^1] memory for more details.

Secondly (and probably the main reason for disliking this code) is that there's absolutely *no justification given* for why this is done. Why is `Display` not simply being used? Is this function even performance-critical? Is there any real benefit to using `ptr::copy_nonoverlapping` for **2 bytes** instead of just (safely) assigning the bytes like `buf[curr] = lut[d1]`? With `ptr::copy_nonoverlapping` there's a number of variants you need to uphold, including making sure that the arguments don't alias (i.e overlap), and making sure the buffers are large enough for the read/write - but neither of these are mentioned.

In [other places](https://github.com/actix/actix-web/blob/f27beab016de18577de0818a5802829b31da96b1/actix-http/src/h1/encoder.rs#L135), there's bits of code like this, which are probably enough to instill fear in people who don't know any Rust at all:

```rust
let mut buf = unsafe { &mut *(dst.bytes_mut() as *mut [u8]) };
```

You might be thinking 'somebody used unsafe a bit too much, what's the big deal here? Just fix it and move on.'. The problem I'm trying to highlight here is that there's a fundamental issue with the author's attitude which we don't seem to be able to change.

For example, you'd think that after the previous [debacle](https://github.com/actix/actix-web/issues/289), Nikolay (the author) would have learnt his lesson about `unsafe`. But apparently this is not the case. Recently, GitHub user [Aaron1011](https://github.com/Aaron1011) opened a PR on actix [removing several instances of `unsafe`](https://github.com/actix/actix-web/pull/968). The changes basically boiled down to replacing a use of `UnsafeCell` with `Cell` / `RefCell` - changes which should have absolutely no observable impact on performance. Aaron101 explained that, as it currently stands, the current interface (though keep in mind this is **not** part of the public API) makes it possible to trigger UB in only a few lines of code. (**UPDATE: This has since been re-opened and merged.**)

The reasonable and responsible thing to do here is to merge. But instead, Nikolay [responded with](https://github.com/actix/actix-web/pull/968#issuecomment-509894555):

>"I guess everybody should switch to interpreted language otherwise we will die in ub"
>
>*PR closed by @fafhrd91*

Not exactly the kind of attitude you'd want from the maintainer of a top web framework with 425k downloads.

Another example of (in my opinion) poor attitude was the release of `actix-web 1.0.2`, which was actually a breaking change (some code was refactored into a separate crate). At least one person has been caught off-guard by this, and [when asked to yank and add a re-export to maintain compatability](https://github.com/actix/actix-web/issues/953), Nikolay responded with simply "i dont see reason" and closed the issue. Again, it's not a huge problem in itself, but I'd consider it mildly irresponsible and a demonstration of poor attitude.

## Flying Solo

\[deleted\]

## Blazingly fast... or not?

Actix is doing very well in the latest round (18) of the [TechEmpower Web Framework Benchmarks](https://www.techempower.com/benchmarks/). It's leading in four of the six benchmarks, and among the top 5 in the remaining two benchmarks. While clearly an excellent result, there's some questionable behaviour going on behind the scenes in a couple of them.

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

In fairness, it seems other benchmarks are doing these things (hyper even does this - though [Sean acknowledges](https://github.com/TechEmpower/FrameworkBenchmarks/blob/master/frameworks/Rust/hyper/src/main.rs#L28) it's a bit of a scummy tactic). Gotham and Thruster [make a point](https://www.reddit.com/r/rust/comments/cd9asm/announcing_gotham_v04/etteye7/) not to do this, for what it's worth. More commentary on the benchmarks from one of the Gotham maintainers can be found [here](https://www.reddit.com/r/rust/comments/cd9asm/announcing_gotham_v04/ett80qm/). I would strongly urge someone to take a proper look at these benchmarks and notify TechEmpower that they're being deceived - there's a special 'stripped' classification for framework applications which have been unrealistically crafted for maximum benchmark score - actix, hyper and many others probably deserve this. 

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
- [Tide](https://github.com/rustasync/tide) - Modular framework built by the Rust Async Ecosystem working group, naturally quite focused on `async`/`await` support but still a work in progress
- Please leave a comment (here or on reddit) if you feel I've left something off this list

<hr>

[^1]: Yes, I'm a Brit, and I'm bloody-well going to switch to spelling uninitialised with an 's' when you're not looking.
