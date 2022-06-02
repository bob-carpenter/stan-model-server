import numpy as np


class Metropolis:
    def __init__(self, model, proposal_rng, init=[]):
        self._model = model
        self._dim = self._model.dims()
        self._q_rng = proposal_rng
        self._theta = init or np.random.normal(size=self._dim)
        self._log_p_theta = self._model.log_density(self._theta)

    def __iter__(self):
        return self

    def __next__(self):
        return self.sample()

    def sample(self):
        # does not include initial value as first draw
        theta_star = self._theta + self._q_rng()
        log_p_theta_star = self._model.log_density(theta_star)
        if np.log(np.random.uniform()) < log_p_theta_star - self._log_p_theta:
            self._theta = theta_star
            self._log_p_theta = log_p_theta_star
        return self._theta, self._log_p_theta


class HMCDiag:
    def __init__(
        self, model, proposal_rng, stepsize, steps, metric_diag=None, init=None
    ):
        self._model = model
        self._dim = self._model.dims()
        self._q_rng = proposal_rng
        self._theta = init or np.random.normal(size=self._dim)
        self._stepsize = stepsize
        self._steps = steps
        self._metric = metric_diag or np.ones(self._dim)

    def joint_logp(self, theta, rho):
        return self._model.log_density(theta) - 0.5 * np.dot(
            rho, np.multiply(this._metric, rho)
        )

    def leapfrog(theta, rho):
        # TODO(bob-carpenter): refactor to share non-initial and non-final updates
        for n in range(self._steps):
            lp, grad = self._model.log_prob_gradient(theta)
            rho_mid = rho - 0.5 * stepsize * multiply(this._metric, grad)
            theta = theta + stepsize * rho_mid
            lp, grad = self._model.log_prob_gradient(theta)
            rho = rho_mid - 0.5 * stepsize * multiply(this._metric, grad)
        return (theta, rho)

    def sample(self):
        theta = self._theta
        rho = np.random.normal(size=self._dim)
        theta_prop, rho_prop = self.leapfrog(theta, rho)
        current_logp = self.joint_logp(theta, rho)
        proposal_logp = self.joint_logp(theta_prop, rho_prop)
        if np.log(np.random.uniform()) < proposal_log_prob - current_log_prob:
            self._theta = theta_star
        return self._theta, self._log_p_theta
