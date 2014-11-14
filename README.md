shake
=====
Shake is a defragmenter that runs in userspace, without the need of patching the kernel and while the system is used (for now, on GNU/Linux only).

There is nothing magic in that : it just works by rewriting fragmented files. But it has some heuristics that could make it more efficient than other tools, including `defrag` and, maybe, `xfs_fsr`.

As an example, it allows you to write:
```shell
find -iname '*.mp3' | sort | shake
```
to defrag all mp3 in a directory, puting together on the disk those close in lexical order.

