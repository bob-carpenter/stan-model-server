# R Client for Stan Model Server

The Stan Model Server links to a compiled Stan program and provides a read-evaluate-print loop (REPL) for accessing methods of the Stan model class. Specifically, this enables accessing parameter names, transforms, log densities, gradients, and generated quantities.

The R Client provides an interface to Stan Model Server, making it easy to access methods of the Stan model class within an R environment. For more information on how the client connects to the server, see the last section [below](#comms). 

## [Loading the R Client](#load)

To run the client in an R environment, load the source file from the [stan-model-server](https://github.com/bob-carpenter/stan-model-server) repository. 

```R
source("StanModelClient.R")
```


Then initialize the process with the path to the model executable, the data (in JSON format), and the seed. There are two ways to do this. First, one can call the object directly.

```R
sc <- StanClient$new('path/to/exe_file', 'path/to/data.json', seed)
```

Or, similarly, using the function wrapper.

```R
sc <- create_stan_client('path/to/exe_file', 'path/to/data.json', seed)
```

After the object has been created, check the status of the connection to the model server.

```R
sc$status()
```

This will indicate whether the model has been initialized and if the process is running. 

## [Using the R Client](#use)

Once the channel of communication has been established between the R Client and the Stan Model Server, the client is straightforward to use. For instance, using the Bernoulli model, here are some key commands.

```R
# Load source file into environment
source('StanModelClient.R')

server <- "stan/bernoulli/bernoulli"          # path to executable
data <- "stan/bernoulli/bernoulli.data.json"  # data in JSON format

# Create client 
sc <- create_stan_client(server, data, seed=1234)

# Check status of the process
sc$status()

# Kill process
sc$end_process()

# Reconnect to the model server
sc$init_process()

# Model methods
sc$name()

sc$param_num()

sc$param_names()

sc$param_unc_num()

sc$param_constrain(-2.3)

sc$log_density(-2.3)

```

### [R Client Commands](#commands)


The following is a complete list of commands.

#### create_stan_client()

```R
sc <- create_stan_client(exe_file, data_file, seed)
```

Function call to create a StanClient object (R6) with three parameters: 

* `exe_file` (string): file path to the compiled executable
* `data_file` (string): file path to the data file (in JSON format)
* `seed` (numeric): set seed for random number generation

#### init_process()

```R
sc$init_process()
```

Initializes the StanClient object.

#### end_process()

```R
sc$end_process()
```

Closes the connection to the Stan Model Server.

#### status()

```R
sc$status()
```

Returns the connection status with the Stan Model Server. 

#### name()

```R
sc$name()
```

Returns the name of the model.

#### dims()

```R
sc$dims()
```

Returns the number of parameters. This is the dimensionality of the log density function. It does not include transformed parameters or generated quantities. It is equivalent to calling `param_num(FALSE, FALSE)`.

#### param_num()

```R
sc$param_num(tp = TRUE, gq = TRUE)
```

Returns the number of model parameters, including the number of transformed parameters (`tp`) and generated quantities (`gq`).

#### param_unc_num()

```R
sc$param_num()
```

Returns the number of unconstrained parameters. Does not include transformed parameters or generated quantities as these do not have unconstrained forms.

#### param_names()

```R
sc$param_names(tp = TRUE, gq = TRUE)
```

Return the encoded name(s) of the constrained parameters, including the transformed parameters (`tp`) and the generated quantities (`gq`). 

#### param_unc_names()

```R
sc$param_unc_names()
```

Returns the encoded name(s) of the unconstrained parameters. Does not include transformed parameters or generated quantities as these do not have unconstrained forms.

#### param_constrain()

```R
sc$param_constrain(params_unc, tp = TRUE, gq = TRUE)

# e.g.,
sc$param_constrain(-2.3)
```

Return the constrained parameters for the specified unconstrained parameters (`params_unc`). Option to include the transformed parameters (`tp`) and generate the the generated quantities (`gq`) using the pseudo-RNG built into the server.

#### param_unconstrain()

```R
sc$param_unconstrain(param_list)

# e.g.,
list_of_params <- list(theta = 0.09112)
sc$param_unconstrain(param_list = list_of_params)
```

Return the unconstrained parameter(s) for the given list of parameter value(s). Passing the parameters as a list is a requirement. Does not include transformed parameters or generated quantities. 

#### log_density()

```R
sc$log_density(params_unc, propto = TRUE, jacobian = TRUE, grad = FALSE, hess = FALSE)

# e.g.,
sc$log_density(-2.3)
```

Return the log density for the unconstrained parameters (`params_unc`). The `propto` and `jacobian` arguments (TRUE by default) indicate whether to include the constant terms and the change-of-variables adjustment in the result. The `grad` and `hess` arguments (FALSE by default) indicate whether the gradient and Hessian information, respectively, should also be returned. 

#### log_density_gradient()

```R
sc$log_density_gradient(params_unc, propto = TRUE, jacobian = TRUE, grad = TRUE, hess = FALSE)

# e.g., 
sc$log_density_gradient(-2.3)
```

Returns the log density and gradient for the unconstrained parameters (`params_unc`). The `propto` and `jacobian` arguments (TRUE by default) indicate whether to include the constant terms and the change-of-variables adjustment in the result. The `grad` argument (TRUE by default) indicates whether to return the gradient. The `hess` argument (FALSE by default)  indicates whether the Hessian should be returned. 

#### log_density_hessian()

```R
sc$log_density_hessian(params_unc, propto = TRUE, jacobian = TRUE, grad = TRUE, hess = TRUE)

# e.g, 
sc$log_density_hessian(-2.3)
```

Returns the log density, gradient, and Hessian for the unconstrained parameters (`params_unc`). The `propto` and `jacobian` arguments (TRUE by default) indicate whether to include the constant terms and the change-of-variables adjustment in the result. The `grad` argument (TRUE by default) indicates whether to return the gradient. The `hess` argument (TRUE by default)  indicates whether the Hessian should be returned. 



## [How the R Client Communicates with the Stan Model Server](#comms)

Since the Stan Model Server interface runs from the command line, one must start a background process from R that allows the user to run the model executable, and in turn read and write to the command line, within an R environment. The [`processx` library](https://processx.r-lib.org/) makes it easy to do so. 

The opening of this connection happens automatically when one initializes a `StanClient` object in R. Technically, the R Client creates an `StanClient` object using the `R6` library. The [`StanClient` object](https://github.com/bob-carpenter/stan-model-server/blob/feature/r-client/StanModelClient.R#L25) holds the methods and attributes of the Stan model. Crucially, upon initialization, this object starts a background process using the `processx` library, thereby opening a connection to the Stan Model Server. 

In the `processx` library, this connection is called a subprocess. The subprocess is stored within the `StanClient` object's `proc` attribute, which makes it easy to read the standard output and error as well as write the standard input of the background process (that is, the model server). The subprocess is initialized as follows:

```R
      self$proc <- processx::process$new(
        paste0("./", self$exe_path), 
        self$proc_args,
        stdout = "|", stdin = "|", stderr = "|"
      )
```

The `process$new()` function starts a process and stores the resulting environment in the `proc` variable. The arguments are (1) the path to the executable, (2) the corresponding arguments, and (3) the options for the standard output, input, and error, respectively, which in this case are all piped. The arguments provided to the subprocess are those that are required [to start the server, namely, the data and pseudo-RNG seed](https://github.com/bob-carpenter/stan-model-server/blob/main/doc/REPL.md#step-2-run-server). By default, these arguments are stored in the `proc_args` attribute of the `StanClient` object; they consist of two flags and the arguments for the seed and data file. 

```R
self$proc_args <- c("-s", seed, "-d", data_file)
```

The status of the subprocess, and hence the connection to the model server, can be checked using the [`status()` function in the `StanClient` object](https://github.com/bob-carpenter/stan-model-server/blob/feature/r-client/StanModelClient.R#L75), which is effectively a wrapper around the `processx` function `is_alive()`. 

When the process is running, it is possible to write and read to the subprocess using the[ `StanClient` object's `write()` and `read()` functions](https://github.com/bob-carpenter/stan-model-server/blob/feature/r-client/StanModelClient.R#L92), which are wrappers around `processx`'s `write_input()` and `read_output_lines()` functions. 

```R
# Writes to Stan model server
write = function(input) {
    self$proc$write_input(paste0(input, '\n'))
}

# Reads output from Stan model server
read = function() {
	self$proc$read_output_lines()
}
```

Note that input is piped by line, so to execute a command the `'\n'` is required; for more information about Stan Model Server's input and output, [see the REPL page for an explanation and a complete list of commands](https://github.com/bob-carpenter/stan-model-server/blob/main/doc/REPL.md#step-3-read-evaluate-print-loop-repl). 

The `read()` and `write()` functions are the building blocks for the R Client's communication with the Stan Model Server. However, the user should never have to invoke these commands as they are [embedded within the `StanClient`'s methods, which mirror the REPL ones](https://github.com/bob-carpenter/stan-model-server/blob/feature/r-client/StanModelClient.R#L107).