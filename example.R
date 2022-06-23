# example.R
#
# This script contains an example of how to use the R Client 
# for the Stan Model Server. 
#
# Though a bit contrived, this example demonstrates how to 
# conduct gradient descent for the single-parameter Bernoulli model.
#
#-----------------------------------------------------------------#


#--------------#
# Setup client #
#--------------#

# Load in the R Client
source("StanModelClient.R")

# Store path to model executable and data file
server <- "./stan/bernoulli/bernoulli"
data <- "stan/bernoulli/bernoulli.data.json"

# Create Stan Client
model <- create_stan_client(server, data, seed=1234)

# Bernoulli Model 
writeLines(readLines('stan/bernoulli/bernoulli.stan'))



#----------------------------------------------------#
# Gradient Descent Algorithm (w/ Jacobian adjustment)
#----------------------------------------------------#

# generate N initial values
theta_init <- runif(model$dims())      

# transform to logit scale
t0 <- boot::logit(theta_init)          

# step size
alpha <- 0.1                          

# starting theta value
t <- t0

# iteration counter
iter <- 0

# maximum number of iterations
max_iter <- 1000



# Conduct gradient descent until some conditions are met...
while(TRUE) {
  
  # Extract gradient from model server
  grad = model$log_density_gradient(t)[['gradient']]
  
  t_plus_1 = t + alpha*grad
  
  if (iter > max_iter | abs(t - t_plus_1) < 1e-08) {
    break
  }
  
  t = t_plus_1
  iter = iter + 1
}


# store final value
theta_w_jacobian <- t

# number of iterations to converge
iter

# check value
c(boot::logit(0.25), theta_w_jacobian)

# difference from target
abs(boot::logit(0.25) - theta_w_jacobian)



#---------------------------------------------------------#
# Gradient Descent Algorithm (without Jacobian adjustment)
#---------------------------------------------------------#

# generate N initial values
theta_init <- runif(model$dims())      

# transform to logit scale
t0 <- boot::logit(theta_init)          

# step size
alpha <- 0.1                          

# starting theta value
t <- t0

# iteration counter
iter <- 0

# maximum number of iterations
max_iter <- 1000



# Conduct gradient descent until some conditions are met...
while(TRUE) {
  
  # Extract gradient from model server; note: jacobian argument set to FALSE
  grad = model$log_density_gradient(t, jacobian=FALSE)[['gradient']]
  
  t_plus_1 = t + alpha*grad
  
  if (iter > max_iter | abs(t - t_plus_1) < 1e-08) {
    break
  }
  
  t = t_plus_1
  iter = iter + 1
}

# store final value
theta_without_jacobian <- t

# number of iterations to converge
iter

# check value
c(boot::logit(0.2), theta_without_jacobian)

# difference from target
abs(boot::logit(0.2) - theta_without_jacobian)


# Theta parameter with and without Jacobian adjustment
c(theta_w_jacobian, theta_without_jacobian)
c(boot::logit(0.25), boot::logit(0.2))


