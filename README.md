# stan-model-server
lightweight server interface to Stan model methods


## dependencies

- The stanc compiler; expected path:  bin/stanc
- The core Stan and Stan math libraries, specified by GNU-Make variables `STAN` and `MATH`.
- The [rapidJSON](https://rapidjson.org) parser library and JSON input parser, `lib/rapidjson_1.1.0` and `src/cmdstan/io/json`, respectively.
- The [CLI11](https://github.com/CLIUtils/CLI11) command line parser library `lib/CLI11-1.9.1`.

## example

To compile the example model `bernoulli.stan` in the directory in
which the makefile resides (`stan-model-server`):

```
$ cd stan-model-server
$ make STAN=<stan> bernoulli
```

where `<stan>` is the path to the top-level directory of the
source code tree for the `stan` module and which contains
the Stan math library source code as subdirectory `math`.
For example, `<stan>` might be `~/github/stan-dev/stan/` *(note trailing slash)*.

This will produce an executable `stan-model-server/bernoulli` which
takes one command line argument - the name of the JSON file containing
the definitions for all variables declared in the Stan program's `data` block:

```
$ ./bernoulli bernoulli.data.json
```

This exe file prints the model name on the console (i.e., "bernoulli_model") and then exits.
