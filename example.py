# First build stan/multi/multi with
# > cd <stan-model-server>
# make -j4 stan/multi/multi

import StanModelClient as smc
import MCMC as mcmc
import numpy as np

server = "stan/multi/multi"
data = "stan/multi/multi.data.json"

model = smc.StanClient(server, data=data, seed=1234)

stepsize = 1/64
steps = 64
metric_diag = [1, 1]
sampler = mcmc.HMCDiag(model, stepsize=stepsize, steps=steps, metric_diag=metric_diag)

# works, but excruciatingly slow
M = 10000
theta = np.empty([M, 2])
for m in range(M):
    theta[m, :], _ = sampler.sample()



# proposal_rng = lambda: np.random.normal(size=model.dims())
# sampler = mcmc.Metropolis(model, proposal_rng)

