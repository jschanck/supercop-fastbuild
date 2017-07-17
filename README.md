supercop-fastbuild
==================

This is supercop-20170716 with a "do-one" script a few other changes
to make testing new primitives easier.

DO NOT use this package for benchmarking.

The "do-one" script lets you build just the packages you need to test
the submission you're working on. For example, the RSA-2048 KEM example
requires GMP, an RNG, and SHA256. So to build crypto_kem rsa2048 run

    $ ./do-one gmp
    $ ./do-one crypto_stream chacha20
    $ ./do-one crypto_rng chacha20
    $ ./do-one crypto_hash sha256
    $ ./do-one crypto_kem rsa2048

You may want to read ./bench/hostname/log and ./bench/hostname/work/errors
after each call to do-one.

The do-one script reads compiler configurations from okcompilers-short/c{,pp}.
You may need to add platform specific configurations to properly test your
submission.

Original source: http://bench.cr.yp.to/supercop/supercop-20170716.tar.xz

Supercop: http://bench.cr.yp.to/supercop.html
