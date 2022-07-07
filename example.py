# First build stan/multi/multi with
# > cd <stan-model-server>
# make -j4 stan/multi/multi

import StanModelClient as smc
import MCMC as mcmc
import numpy as np

server = "stan/multi/multi"
data = "stan/multi/multi.data.json"

model = smc.StanClient(server, data=data, seed=1234)
D = model.dims()

stepsize = 0.25
steps = 10
metric_diag = [1] * D
sampler = mcmc.HMCDiag(model, stepsize=stepsize, steps=steps, metric_diag=metric_diag)

# works, but excruciatingly slow
M = 1000
theta = np.empty([M, D])
for m in range(M):
    theta[m, :], _ = sampler.sample()

print(theta.mean(0))
print(theta.std(0))

# proposal_rng = lambda: np.random.normal(size=model.dims())
# sampler = mcmc.Metropolis(model, proposal_rng)
