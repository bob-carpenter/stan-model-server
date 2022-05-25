# Stan Model Server

A lightweight server interface to Stan model methods:


## Example

Example Stan programs are organized into directories under the `stan`
directory.  The subdirectory `stan/bernoulli` contains both the Stan
model file and input data.  You must compile all Stan models from the
directory in which the makefile resides (i.e., from the
`stan-model-server` directory).

> Before trying this, check out the [dependencies](#Dependencies) section below.  You will at least need to install the stanc parser binary for your platform, as well as the source for Stan (`stan-dev:stan` repo) and for the Stan Math library (`stan-dev:math` repo).

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

#### GNU make

GNU make must be available on your path so that `make` on the
command-line resolves.

#### C++ toolchain

A C++ toolchain that supports C++1y (between C++14 and C++17);  here 
are
[full details on C++ requirements](https://mc-stan.org/docs/2_29/cmdstan-guide/cmdstan-installation.html#source-installation). 

The makefile defaults to `clang`, but this can be changed by setting the
`CXX` environment variable for the makefile.

```
CXX ?= clang++
```

#### stanc

Stanc is the Stan compiler (it's technically a transpiler from Stan to
C++). The environment variable `STANC` must point to this program.
The easiest way to do this is to move a `stanc` instance for your
platform to the local `bin` directory (e.g., from a `cmdstan`
install). 

```
STANC ?= bin/stanc 
```

#### stan and math

We need the source code for

* Stan [GitHub repo `stan-dev:stan`](https://github.com/stan-dev/stan), and 
* Stan math library: [GitHub repo `stan-dev:math`](https://github.com/stan-dev/math)

These are specified by the variables `STAN` and `MATH`, with the following
defaults. 

```
	STAN ?= $(HOME)/github/stan-dev/cmdstan/stan/
	MATH ?= $(STAN)lib/stan_math/
```

#### Rapid JSON

The Stan model server requires [rapidJSON](https://rapidjson.org) for
parsing data files. A version is distributed with the model server,
but a different installation can be used by setting the relevant
environment variable.

```
RAPIDJSON ?= lib/rapidjson_1.1.0/
```

#### Command line interface for C++ 11

We also distribute [CLI11](https://github.com/CLIUtils/CLI11), a
command line interface library for C++11.  An alternative version can
be used by setting this make variable.

```
CLI11 ?= lib/CLI11-1.9.1/
```

## License

* Code released under the [BSD-3 license](LICENSE).
* Documentation released under the
  [CC BY 4.0 license](https://creativecommons.org/licenses/by/4.0/).
