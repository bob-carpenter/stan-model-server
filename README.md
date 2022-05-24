# Stan Model Server

A lightweight server interface to Stan model methods:


## Example

Example Stan programs are in the `stan` directory.
The subdirectory `stan/bernoulli` contains both the Stan model file and input data.
You must compile all Stan models from the directory in which the makefile resides,
i.e. from the `stan-model-server` directory.

```
$ cd stan-model-server
$ make src/bernoulli/bernoulli
```

This will produce an executable `stan-model-server/stan/bernoulli/bernoulli` which
takes one command line argument - the name of the JSON file containing
the definitions for all variables declared in the Stan program's `data` block:

```
$ cd stan/bernoulli
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

## Python Client

You can run the Python client from this directory as follows.

```python
import StanModelClient
s = StanModelClient.StanClient("./bernoulli", data = "bernoulli.data.json", seed = "1234")
s.name()
del s
```

## Dependencies

- A modern C++ toolchain, including the GNU-Make utility, [details here](https://mc-stan.org/docs/2_29/cmdstan-guide/cmdstan-installation.html#source-installation)
- The stanc compiler; expected path:  bin/stanc
- The core Stan and Stan math libraries, specified by GNU-Make variables `STAN` and `MATH` with the following defaults
```
	STAN ?= $(HOME)/github/stan-dev/cmdstan/stan/
	MATH ?= $(STAN)lib/stan_math/
```
- The [rapidJSON](https://rapidjson.org) parser library and JSON input parser, `lib/rapidjson_1.1.0` and `src/cmdstan/io/json`, respectively.
- The [CLI11](https://github.com/CLIUtils/CLI11) command line parser library `lib/CLI11-1.9.1`.

## License

* Code released under the [BSD-3 license](LICENSE).
* Documentation released under the
  [CC BY 4.0 license](https://creativecommons.org/licenses/by/4.0/).
