+++
title = "C with hindsight: What I'd change about C"
date = 2019-08-06

[taxonomies]
categories = ["C"]
+++

ASD

---

- remove VLA
- remove narrowing / sign-changing implicit conversions (also ptrs)
- forbid 5[foo]
- make void foo(); == void foo(void);
- allow function use without a declaration, as long as it's defined below
- remove inline keyword or make it useful
- make signed/unsigned overflow not braindead
- remove postincrement / preincrement
- make string literals const
- fix fucking errno
- fix tbaa
- move mem* and malloc into memory.h
- remove scanf %s
- if expressions / statement expressions (e.g ({ foo }) gcc extension)
- standard compiler hints for likely/unlikely/unreachable
- make bitfields not braindead
- volatile? non-tearing?
- make malloc guaranteed to return a valid pointer; opt out with a variant like emalloc
- disallow sizeof expr

