# StanModelClient.R
#
# Stan Model Client
# 
# This script contains an R-based client for the Stan Model Server.
#
# Compile the model executable before using the client. See the 
# `README.md` file in the `stan-model-server` repo for instructions.
#
# Also, if necessary install the following packages:
# install.packages(c("processx", "R6", "rjson"))
#------------------------------------------------------------------------------#


create_stan_client <- function(exe_path = NULL, data_file = NULL, seed = NULL) {
  # Function call to create a StanClient object (R6) with three parameters: 
  #        - exe_path (string): file path to the compiled model executable
  #        - data_file (string): file path to the data file (in JSON format)
  #        - seed (numeric): set seed for random number generation
  
  StanClient$new(exe_path = exe_path, data_file = data_file, seed = seed)
}


StanClient <- R6::R6Class(
  # Stan client class holding all resources.
  #
  # Attributes:
  #     - exe_path (string): path to Stan model executable
  #     - data_file (string): file path to the data file (in JSON format)
  #     - seed (numeric): set seed for random number generation
  #     - proc (environment): subprocess to Stan Model Server, using `processx`
  #     - proc_args (character): arguments to launch the executable
  
  
  classname = "StanClient",
  
  public = list(
    exe_path = NULL, 
    data_file = NULL,
    seed = NULL,
    proc = NULL,
    proc_args = NULL,
    
    
    initialize = function(exe_path = NA, data_file = NA, seed = NA) {
      # Construct a Stan client with open process to server:
      
      self$exe_path <- exe_path
      self$data_file <- data_file
      self$seed <- seed
      self$proc_args <- c("-s", seed, "-d", data_file)
      self$init_process()
    },
    
    
    init_process = function() {
      # Starts a background process using `processx`, opening a connection
      # to the Stan Model Server; this function is called when the 
      # object is created.
      
      self$proc <- processx::process$new(
        paste0("./", self$exe_path), 
        self$proc_args,
        stdout = "|", stdin = "|", stderr = "|"
      )
    },
    
    end_process = function() {
      # Closes the connection with the Stan Model Server
      
      invisible(self$proc$kill())
    }, 
    
    status = function() {
      # Returns the status of the Stan Model Server
      
      if (is.null(self$proc) == TRUE) {
        return("Process uninitialized; connect to the Stan Model Server with init_process()")
      }
      
      if (self$proc$is_alive() == FALSE) {
        return("Process stopped; reconnect to the Stan Model Server with init_process()")
      }
      
      if (self$proc$is_alive() == TRUE) {
        return("Process running; connected to Stan Model Server")
      }
    },
    
    
    ## I/O Functions
    
    write = function(input) {
      # Writes to Stan model server 
      
      self$proc$write_input(paste0(input, '\n'))
    },
    
    read = function() {
      # Reads output from Stan model server
      
      self$proc$read_output_lines()
    },
    
    
    ## REPL Functions
    
    name = function() {
      # Returns the name of the Stan model on the server
      
      self$write('name')
      return(self$read())
    },
    
    param_num = function(tp = TRUE, gq = TRUE) {
      # Returns the number of constrained parameters 
      #
      # The function takes two optional arguments:
      #     - tp (logical): `TRUE` to include transformed parameters (default); `FALSE` to exclude
      #     - gq (logical): `TRUE`  to include generated quantities (default); `FALSE` to exclude
      
      self$write(paste('param_num', as.numeric(tp), as.numeric(gq), sep = " "))
      self$proc$poll_io(50)
      return(as.numeric(self$read()))
    }, 
    
    dims = function() {
      # Return number of parameters
      #
      # This is the dimensionality of the log density function. It does
      # not include transformed parameters or generated quantities. It is
      # equivalent to calling `param_num(0, 0)`
      
      return(as.numeric(self$param_num(FALSE, FALSE)))
    },
    
    param_unc_num = function() {
      # Returns the number of unconstrained parameters
      #
      # Does not include transformed or generated quantities
      # as these do not have unconstrained forms.
      
      
      self$write('param_unc_num')
      self$proc$poll_io(50)
      return(as.numeric(self$read()))
    },
    
    param_names = function(tp = TRUE, gq = TRUE) {
      # Return the encoded constrained parameter names.
      # 
      # Optionally includes names of transformed parameters and
      # generated quantities as indicated.  Parameter names are included
      # for each scalar. Container indexes are separated by periods.
      # For example, `a.2` is the second entry in a one-dimensional
      # array `a` and `b.1.2` might be the value at the first row and
      # second column of matrix `b`.
      # 
      #  The function takes two optional arguments:
      #     - tp (logical): `TRUE` to include transformed parameters (default); `FALSE` to exclude
      #     - gq (logical): `TRUE`  to include generated quantities (defautl); `FALSE` to exclude
      
      self$write(paste('param_names', as.numeric(tp), as.numeric(gq), sep = " "))
      self$proc$poll_io(50)
      return(strsplit(self$read(), split=",")[[1]])
    },
    
    param_unc_names = function() {
      # Return the encoded unconstrained parameter names.
      #
      # Does not include transformed parameters or generated
      # quantities as these do not have unconstrained forms. 
      # Parameter names are included for each scalar. Container 
      # indexes are separated by periods. For example, `a.2` is 
      # the second entry in a one-dimensional array `a` 
      # and `b.1.2` might be the value at the first row and
      # second column of matrix `b`.
      
      
      self$write('param_unc_names')
      self$proc$poll_io(50)
      return(strsplit(self$read(), split=",")[[1]])
    },
    
    param_constrain = function(params_unc, tp=TRUE, gq=TRUE) {
      # Return the constrained parameters for the specified 
      # unconstrained parameters.
      # 
      # Optionally include the transformed parameters and generate the
      # generated quantities using the pseudo-RNG built into the server.
      #
      # The function takes three arguments:
      #     - params_unc: unconstrained parameters
      #     - tp (logical): `TRUE` to include transformed parameters (default); `FALSE` to exclude
      #     - gq (logical): `TRUE`  to include generated quantities (defautl); `FALSE` to exclude
      
      
      params_unc=paste(params_unc, collapse=' ')
      self$write(paste('param_constrain', 
                       as.numeric(tp), as.numeric(gq),
                       params_unc, 
                       sep = " "))
      self$proc$poll_io(50)
      return(as.numeric(strsplit(self$read(), split=",")[[1]]))
    },
    
    param_unconstrain = function(param_list) {
      # Return unconstrained parameters for parameters.
      # The parameters are passed as a list and converted to
      # JSON format for the Stan Model Server.
      #
      # Does not include transformed parameters or generated
      # quantities in input or output as they are not defined 
      # on the unconstrained scale.
      # 
      # The function takes one argument: 
      #     - params_list (list): values for each parameter, e.g., list("theta" = -2.32)
      
      if (is.list(param_list) == FALSE) {
        return("Error: unconstrained parameters must be passed as a list")
      }
      
      self$write(paste('param_unconstrain', 
                       rjson::toJSON(param_list), 
                       sep = " "))
      self$proc$poll_io(50)
      return(as.numeric(strsplit(self$read(), split=",")[[1]]))
    },
    
    log_density = function(params_unc, propto=TRUE, jacobian=TRUE, grad=FALSE, hess=FALSE) {
      # Return log density for unconstrained parameters
      #
      # The `propto` and `jacobian` arguments indicate whether to include
      # the constant terms and the change-of-variables adjustment in the result.
      # The `grad` and `hess` arguments indicate whether the gradient and
      # Hessian information, respectively, should be returned.
      # 
      # The function requires one argument and four optional arguments set by default:
      #       - `param_unc`: unconstrained parameter values
      #       - `propto` (logical): `TRUE` to exclude constant terms (default); `FALSE` to include
      #       - `jacobian` (logical): `TRUE` to include adjustment (default); `FALSE` to exclude
      #       - `grad` (logical): `TRUE` to include gradient; `FALSE` to exclude (default)
      #       - `hess` (logical): `TRUE` to include Hessian; `FALSE` to exclude (default)
      
      params_unc=paste(params_unc, collapse=' ')
      self$write(paste('log_density', 
                       as.numeric(propto), as.numeric(jacobian), 
                       as.numeric(grad), as.numeric(hess), 
                       params_unc, 
                       sep = " "))
      self$proc$poll_io(50)
      return( as.numeric(unlist( strsplit(self$read(), split=","))[1] ) )
      
      
    },
    
    log_density_gradient = function(params_unc, propto=TRUE, jacobian=TRUE, grad=TRUE, hess=FALSE) {
      # Return log density and gradient for unconstrained parameters
      #
      # The `propto` and `jacobian` arguments indicate whether to include
      # the constant terms and the change-of-variables adjustment in the result.
      # The `grad` and `hess` arguments indicate whether the gradient and
      # Hessian information, respectively, should be returned.
      #
      # The function requires one argument and four optional arguments set by default:
      #       - `param_unc`: unconstrained parameter values
      #       - `propto` (logical): `TRUE` to exclude constant terms (default); `FALSE` to include
      #       - `jacobian` (logical): `TRUE` to include adjustment (default); `FALSE` to exclude
      #       - `grad` (logical): `TRUE` to include gradient (default); `FALSE` to exclude
      #       - `hess` (logical): `TRUE` to include Hessian; `FALSE` to exclude (default)
      
      params_unc=paste(params_unc, collapse=' ')
      
      self$write(paste('log_density', 
                       as.numeric(propto), 
                       as.numeric(jacobian), 
                       as.numeric(grad), 
                       as.numeric(hess), 
                       params_unc, 
                       sep=" "))
      
      self$proc$poll_io(50)
      
      out = as.numeric(strsplit(self$read(), split=",")[[1]])
      
      lst_dens = c("density" = out[1], 
                   "gradient" = as.list(out[2:length(out)]))
      
      return(lst_dens)
    },
    
    log_density_hessian = function(params_unc, propto=TRUE, jacobian=TRUE, grad=TRUE, hess=TRUE) {
      # Return log density, gradient, and Hessian for unconstrained parameters
      #
      # The `propto` and `jacobian` arguments indicate whether to include
      # the constant terms and the change-of-variables adjustment in the result.
      # The `grad` and `hess` arguments indicate whether the gradient and
      # Hessian information, respectively, should be returned.
      #
      # The function requires one argument and four optional arguments set by default:
      #       - `param_unc`: unconstrained parameter values
      #       - `propto` (logical): `TRUE` to exclude constant terms (default); `FALSE` to include
      #       - `jacobian` (logical): `TRUE` to include adjustment (default); `FALSE` to exclude
      #       - `grad` (logical): `TRUE` to include gradient (default); `FALSE` to exclude
      #       - `hess` (logical): `TRUE` to include Hessian (default); `FALSE` to exclude
      
      params_unc=paste(params_unc, collapse=' ')
      
      self$write(paste('log_density',
                       as.numeric(propto), 
                       as.numeric(jacobian), 
                       as.numeric(grad), 
                       as.numeric(hess),
                       params_unc, 
                       sep = " "))
      
      self$proc$poll_io(50)
      
      out = as.numeric(strsplit(self$read(), split=",")[[1]])
      
      N = as.integer(sqrt(length(out) - 1))
      
      lst_dens = list("density" = out[1],
                      "gradient" = as.vector(out[2:(N+1)]),
                      "hessian" = matrix(out[(N+2):length(out)], 
                                         nrow = N, ncol = N))
      
      return(lst_dens)
    }
  )
)

  
  


