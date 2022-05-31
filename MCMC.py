import numpy as np

class Metropolis:
    _model = []
    _dim = []
    _q_rng = []
    _theta = []
    _log_p_theta = []

    def __iter__(self):
        return self

    def __next__(self):
        return self.sample()

    def __init__(self, model, proposal_rng, init = []):
        self._model = model
        self._dim = self._model.dims()
        self._q_rng = proposal_rng
        self._theta = init or np.random.normal(size = self._dim)
        self._log_p_theta = self._model.log_density(self._theta)

    def sample(self):
        # could include logic to include initialization as first draw
        theta_star = self._theta + self._q_rng()
        log_p_theta_star = self._model.log_density(theta_star)
        accept = 0
        if np.log(np.random.uniform()) < log_p_theta_star - self._log_p_theta:
            self._theta = theta_star
            self._log_p_theta = log_p_theta_star
            accept = 1
        return self._theta, self._log_p_theta, [accept]
