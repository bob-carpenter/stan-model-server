# First build stan/multi/multi with
# > cd <stan-model-server>
# make -j4 stan/multi/multi

import StanModelClient as smc
import MCMC as mcmc
import numpy as np

server = "stan/multi/multi"
data = "stan/multi/multi.data.json"

model = smc.StanClient(server, data=data, seed=1234)

stepsize = 0.1
steps = 10
sampler = mcmc.HMCDiag(model, stepsize=stepsize, steps=steps)
sampler.sample()

print(model.param_names(0, 0), ", log density, accept\n")
for n in range(10):
    theta = sampler.sample()
    print(theta, "\n")

# proposal_rng = lambda: np.random.normal(size=model.dims())
# sampler = mcmc.Metropolis(model, proposal_rng)

stepsize = 0.1
steps = 10
metric_diag = [1, 1]
sampler = mcmc.HMCDiag(model, proposal_rng, stepsize, steps, metric_diag)
print(model.param_names(0, 0), ", log density, accept\n")
for n in range(10):
    theta = sampler.next()
    print(theta, "\n")
