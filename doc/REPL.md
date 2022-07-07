# Stan Model Server

The Stan Model Server links to a compiled Stan program and provides a
read-evaluate-print loop (REPL) for accessing methods of the Stan
model class. Specifically, this enables accessing parameters names,
transforms, log densities, gradients, and generated quantities.

## Step 1: Compile Executable

The compilation steps are run from the shell.

1. Transpile Stan program to C++
* Inputs: .stan file
* Outputs: .hpp file
* Configuration: TBB (threads), OpenCL (GPU), MPI (multi-process)

2. Compile transpiled C++ to object code and link to server to
   generate executable
* Input: .hpp file
* Output: model server executable
* Configuration: compiler options, make options (e.g., j8)

For example, for the model supplied in
`stan/bernoulli/bernoulli.stan`, the build procedure is

```
> cd <stan-model-server>
> make stan/bernoulli/bernoulli
```

The final executable is built in `stan/bernoulli/bernoulli`.


## Step 2: Run Server

1. Run executable for model
* Configuration: data file path (.json), random seed (unsigned int)

Continuing the running example, we fire it up given a JSON data file
`stan/bernoulli/bernoulli.data.json` as

```
> stan/bernoulli/bernoulli -d stan/bernoulli/bernoulli.data.json -s 1234
```

This reads the data and creates the model object in the server.  The
seed is used to create a random number generator which is used
throughout the calls to the server.

Any messages printed by the Stan program on data load will be directed
to `stderr`.


## Step 3: Read-Evaluate-Print-Loop (REPL)

The REPL supports the following operations through standard input and
output, with all messages going to standard error.  The operations are
listed following the general formatting standards.


#### Standard streams: stdin, stdout, and stderr

The three standard streams are used: stdin, stdout, stderr.  Input is
read from stdin and output is written to stdout.  Any messages written
by Stan programs are redirected to stderr.  Any exceptions thrown by
models due to errors are written to stderr.  Any error messages from
the server are written stderr.

#### Line-based I/O

I/O is line based.  All input must be provided on a single line with a
terminating newline.  All output is returned as a single-line with a
terminating newline.  Newlines are UNIX style (C++ '\n').

#### Floating point precision and notation

Floating point numbers are written to double precision, using
scientific notation if necessary.  Input may be provided to any
precision using fixed or scientific notation, but only
double-precision is retained.

#### String formats

All messages from the server are encoded in ASCII.  Print statements
within a Stan program may include UTF-8 encoded characters.

#### CSV format for sequences

All sequences are read and returned in comma-separated value notation.
Output is not padded, but the input may be.

#### Prompt free

There is no prompt from the REPL---it blocks until a line of input
is available to read.

#### Container entry names

Names of container entries are written using periods as separators
(e.g., matrix entry `a[1, 2]` has name `a.1.2` and complex scalar `z`
has parameter names `z.re` and `z.im`)


### REPL Commands

The following is a complete list of commands and their format.


#### quit

```
quit
```

Writes an exit report, terminates REPL loop, frees
resources, shuts down server, and returns an exit code back to the
operating system (zero for success, nonzero otherwise).


#### name

```
name
```

Writes name of model.


#### param_num

```
param_num <tp>(int) <gq>(int)
```

Write number of contrained parameters, including transformed
parameters if `tp` is 1, and including generated quantities if `gq`
is 1.


#### param_unc_num

```
param_unc_num
```

Write number of uncontrained parameters, excluding transformed
parameters and generated quantities.


#### param_names

```
param_names <tp>(int) <gq>(int)
```

Write names of constrained parameters, including transformed
parameters if `tp` is 1, and including generated quantities if `gq`
is 1.


#### param_unc_names

```
param_unc_names
```

Write names of unconstrained parameters, excluding transformed
parameters and generated quantities.


#### param_constrain

```
param_constrain  <tp>(int) <gq>(int) <param_unc>(float(,float)*)
```

Write constrained parameters corresponding to unconstrained parameters
`param_unc`, including transformed parameters if `tp` is 1 and including
generated quantities if `gq` is 1.


#### param_unconstrain

```
param_unconstrain <param>(json)
```

Write the unconstrained parameters corresponding to the constrained
paramters encoded in JSON, excluding transformed parameters and
generated quantities.


#### log_density

```
log_density <propto>(int) <jacobian>(int) <grad>(int) <hess>(int) <param_unc>(float(,float)*)
```

Return the log density of the unconstrained parameters
`param_unc`, dropping constant terms that do not depend on parameters
if `propto` is 1 and including the change-of-variables adjustment for
constrained parameters if `jacobian` is 1.  The gradient is returned
if `grad` is 1; gradients are calculated by automatic differentiation.
The Hessian is returned in column-major order if `hess` is 1; Hessians
are calculated with central finite differences using the automatic
differentiation gradients.
