# stan-model-server
lightweight server interface to Stan model methods


## dependencies

- The stanc compiler; expected path:  bin/stanc
- The core Stan and Stan math libraries, specified by GNU-Make variables `STAN` and `MATH`.
- The JSON parser library, included as `src/cmdstan/io/json`

## example

To compile the example model `bernoulli.stan` in the directory in
which the makefile resides (`stan-model-server`):

```
$ cd stan-model-server
$ make STAN=<stan> bernoulli
```

where `<stan>` is the path to the top-level directory of the
repository `stan` from `stan-dev`.  For example, `<stan>` might be
`~/github/stan-dev/stan`.

This will produce an executable `stan-model-server/bernoulli` which
can be invoked with:

```
$ ./bernoulli
```

It will print the model name on the console (i.e., "bernoulli_model").
It reads in a data file `foo.json`, but that's hard coded in the executable.
