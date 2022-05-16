# stan-model-server
lightweight server interface to Stan model methods


## dependencies

- The stanc compiler; expected path:  bin/stanc
- The core Stan and Stan math libraries, specified by GNU-Make variables `STAN` and `MATH`.
- The JSON parser library, included as `src/cmdstan/io/json`

## example

To compile the example model `bernoulli.stan` in this directory:
```
make STAN=path/to/stan/distribution/ bernoulli
```

The compiled executable (proof of concept) reads in the data file ("foo.json") and prints the model name on the console.






