# Stan Model Server

A lightweight server interface to Stan model methods:


## Example

To compile the example model `bernoulli.stan` in the directory in
which the makefile resides (`stan-model-server`):

```
$ cd stan-model-server
$ make STAN=<stan> bernoulli
```

where `<stan>` is the path to the top-level directory of the
source code tree for the `stan` module and which contains
the Stan math library source code as subdirectory `math`.
For example, `<stan>` might be `~/github/stan-dev/stan/` *(note the
required trailing slash)*.

This will produce an executable `stan-model-server/bernoulli` which
takes one command line argument - the name of the JSON file containing
the definitions for all variables declared in the Stan program's `data` block:

```
$ ./bernoulli -s 1234 -d bernoulli.data.json
```

The `-s` flag indicates a random seed and the `-d` flag the path to
the data in JSON.  You will then enter the REPL loop.


## Design: Read-eval-print loop (REPL)

The basic design follows the standard read-eval-print loop design
pattern. See the [design document](design.txt) for specifics.


## Inspiration

The overall idea to do this came from

* Dan Muck and Daniel
  Lee. 2022. [Smuggling log probability and gradients out of Stan programs — ReddingStan](https://blog.mc-stan.org/2022/03/24/smuggling-log-probability-and-gradients-out-of-stan-programs-reddingstan/). *The
  Stan Blog*.
* dmuck. [redding-stan](https://github.com/dmuck/redding-stan). GitHub.

Redding Stan is released under BSD-3, like Stan itself.

## Dependencies

- The stanc compiler; expected path:  bin/stanc
- The core Stan and Stan math libraries, specified by GNU-Make variables `STAN` and `MATH`.
- The [rapidJSON](https://rapidjson.org) parser library and JSON input parser, `lib/rapidjson_1.1.0` and `src/cmdstan/io/json`, respectively.
- The [CLI11](https://github.com/CLIUtils/CLI11) command line parser library `lib/CLI11-1.9.1`.

## License

* Code released under the [BSD-3 license](LICENSE).
* Documentation released under the
  [CC BY 4.0 license](https://creativecommons.org/licenses/by/4.0/).
