# Stan Model Server

A lightweight server interface to Stan model methods.

#### Installation Instructions

For installation instructions, see

* Installing Stan Model Server [Install Documentation](doc/INSTALL.md)

#### Read-Eval-Print Loop

For full documentation on the REPL, see:

* Stan Model Server [REPL Documentation](doc/REPL.md)


## Hello, Shell!

The package is distributed with a a Stan program in `stan/bernoulli/bernoulli.hpp` and matching data in `stan/bernoulli/bernoulli.data.json`.

To compile the server executable,

```
$ cd stan-model-server
$ make CMDSTAN=/Users/carp/github/stan-dev/cmdstan stan/bernoulli/bernoulli
```

Then we run from the command line with data and an RNG seed.

```
$ stan/bernoulli/bernoulli -s 1234 -d stan/bernoulli/bernoulli.data.json
```

The lines marked with `<` indicate input from the user and the unmarked lines are responses from the server.

```
< name
bernoulli_model

< param_num 1 1
3

< param_unc_num
1

< param_names 1 1
theta,logit_theta,y_sim

< param_unc_names
theta

< param_constrain 1 1 -2.3
0.0911229610148561,-2.3,0

< param_unconstrain {"theta":0.09112}
-2.30003575312272

< log_density 1 1 1 1 -1.5
-6.91695933579303,0.810893714323724,-1.78975742485563
```


## Motivation and Inspiration

We want to be able to access Stan model methods from within R or Python in order to do algorithm development.  The first system that did this is HTTPStan, an official Stan project:

* GitHub stan-dev: [httpstan](https://github.com/stan-dev/httpstan)

The direct inspiration for this project came from the simple I/O structure of ReddingStan:

* Dan Muck and Daniel Lee. 2022. [Smuggling log probability and gradients out of Stan programs — ReddingStan](https://blog.mc-stan.org/2022/03/24/smuggling-log-probability-and-gradients-out-of-stan-programs-reddingstan/). *The Stan Blog*.
* Github: [dmuck/redding-stan](https://github.com/dmuck/redding-stan) 

## Python Client

The Python client is feature complete. It can be invoked this way after the Bernoulli example is compiled.

```python
> import StanModelClient as smc
> sc = smc.StanClient("./stan/bernoulli/bernoulli",
                      data = "stan/bernoulli/bernoulli.data.json",
                      seed = 1234)
```

Here are some example calls.

```python
> sc.name()

> sc.param_names()

> sc.log_density_gradient([-2.3])

> sc.param_constrain([-2.3])
```

Documentation is available as docstrings in the [source code](StanModelClient.py).

### Python-based Samplers

The plan is to build samplers out in Python using this interface.  The samplers are even more a work-in-progress than the client interface. For now, there is a [worked example of Metropolis with a Stan model](example.py).

## R Client

The R Client is feature complete. It can be invoked this way after the Bernoulli example is compiled.

```R
# Load source file into environment
source('StanModelClient.R')

# Create the Stan Client 
sc <- create_stan_client(exe_file = "./stan/bernoulli/bernoulli", 
                         data_file = "stan/bernoulli/bernoulli.data.json", 
                         seed = 1234)

```

Here are some example calls.

```R
sc$name()

sc$param_names()

sc$log_density_gradient(-2.3)

sc$param_constrain(-2.3)
```

For more information on the R Client, see the [source code](StanModelClient.R) and the [documentation](./doc/R-CLIENT.md) for a complete list of commands and their usage.

### R-based Samplers

In time, the samplers available in Python will also be available through the R interface. For now, however, see the [worked example of gradient descent on the Bernoulli Stan model](example.R). 

## License

* Code released under the [BSD-3 license](LICENSE).
* Documentation released under the
  [CC BY 4.0 license](https://creativecommons.org/licenses/by/4.0/).
