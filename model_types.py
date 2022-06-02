from typing import Protocol, Tuple
from numpy.typing import ArrayLike, NDArray
import numpy as np


class LogDensityOnly(Protocol):
    def dims(self) -> int:
        ...

    def log_density(self, params_unc: ArrayLike) -> float:
        ...


class LogDensityGrad(LogDensityOnly, Protocol):
    def log_density_gradient(
        self, params_unc: ArrayLike
    ) -> Tuple[float, NDArray[np.float64]]:
        ...


class LogDensityHessian(LogDensityGrad, Protocol):
    def log_density_hessian(
        self, params_unc: ArrayLike
    ) -> Tuple[float, NDArray[np.float64], NDArray[np.float64]]:
        ...


class ModelGradFiniteDiff(LogDensityGrad):
    def __init__(self, m: LogDensityOnly) -> None:
        # might need to define this function at the class level and store m, not sure
        self.model = m

    def log_density_gradient(
        self, params_unc: ArrayLike
    ) -> Tuple[float, NDArray[np.float64]]:
        log_p = self.log_density(params_unc)
        # TODO finite difference things
        return log_p, np.array([])

    # boilerplate
    def dims(self) -> int:
        return self.model.dims()

    def log_density(self, params_unc: ArrayLike) -> float:
        return self.model.log_density(params_unc)


class ModelHessianFiniteDiff(LogDensityHessian):
    def __init__(self, m: LogDensityGrad) -> None:
        self.model = m

    def log_density_hessian(
        self, params_unc: ArrayLike
    ) -> Tuple[float, NDArray[np.float64], NDArray[np.float64]]:
        log_p, grad = self.log_density_gradient(params_unc)
        # TODO finite difference things
        return log_p, grad, np.array([])

    # boilerplate
    def dims(self) -> int:
        return self.model.dims()

    def log_density(self, params_unc: ArrayLike) -> float:
        return self.model.log_density(params_unc)

    def log_density_gradient(
        self, params_unc: ArrayLike
    ) -> Tuple[float, NDArray[np.float64]]:
        return self.model.log_density_gradient(params_unc)
