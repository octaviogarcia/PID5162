reljmp
======

A simple module that can replace kernel functions on a live system. It follows
the kpropbe approach and replaces the first few bytes of the original (to be
replaced) function with a relative jump to the replacement function.

I don't have to tell you how dangerous this is and how easily things can blow
up in your face. If you don't believe me, you probably shouldn't try this :-)

The module in this repo is a simple example that replaces the kernel's
printk() function with our own version. It's based on a 2.6.38 kernel simply
because that's what was running on the system where I had to do this. If you
need this for a different kernel, you might want to check the kprobes code to
verify that the approach is still valid.

In general, for every kernel function that you need to replace, you want to
create a separate file reljmp_\<function name\>.c (use reljmp_printk.c as a
starting point), add the the name of the reljmp struct (rj_printk in the
example) to the reljmp_func[] array in reljmp_main.c and update the Makefile.
That's it. Easy, ain't it? :-)
