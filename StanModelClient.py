"""Stan Model Client

This module supplies a client for the Stan Model Server.
"""
import numpy as np
import numpy.typing as npt
import json
import subprocess
from typing import Any, Iterable, List, Mapping, Tuple, Union


class StanClient:
    """Stan client class holding all resources.

    Attributes:
        server: Subprocess for Stan model server
    """

    def __init__(self, modelExe: str, data: str, seed: int = 1234) -> None:
        """Construct a Stan client with open subprocess to server.

        Args:
            modelExe: Path to Stan model server executable
            data: Path to JSON data file
            seed: Pseudo-random number generator seed; Defaults to 1234
        """
        cmd = [modelExe, "-d", data, "-s", str(seed)]
        self.server: subprocess.Popen = subprocess.Popen(
            cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

    def __del__(self) -> None:
        """Close the server process, terminate it, and wait for shutdown."""
        self.request("quit")
        self.server.stdin.close()
        # TODO(carpenter): check to see if quit already closes
        self.server.terminate()
        self.server.wait(timeout=0.5)

    # I/O functions
    def _read(self) -> str:
        return self.server.stdout.readline().decode("utf-8").strip()

    def _write(self, msg: str) -> None:
        self.server.stdin.write(msg.encode("utf-8"))

    def _write_num(self, x: float) -> None:
        self._write(" ")
        self._write(str(x))

    def _write_nums(self, xs: Iterable[float]) -> None:
        for x in xs:
            self._write_num(x)
        # np.savetxt(self.server.stdin, xs, newline=" ", delimiter=" ")

    def _get_return(self) -> str:
        self._write("\n")
        self.server.stdin.flush()
        return self._read()

    def _get_return_float(self) -> float:
        return float(self._get_return())

    def _get_return_floats(self) -> npt.NDArray[np.float64]:
        return np.fromstring(self._get_return(), sep=",", dtype=np.float64)

    def _request(self, msg: str) -> str:
        self._write(msg)
        return self._get_return()

    # REPL functions
    def name(self) -> str:
        """Return name of model being served.

        Return:
            Name of model being served.
        """
        return self._request("name")

    def param_num(self, tp: bool = True, gq: bool = True) -> int:
        """Return the number of constrained parameters.

        Optionally includes counts of transformed parameters and
        generated quantities.

        Args:
            tp: `True` to include transformed parameters, `False` to exclude
            gq: `True` to include generated quantitites, `False` to exclude
        Return:
            number of parameters
        """
        return int(self._request(f"param_num {int(tp)} {int(gq)}"))

    def dims(self) -> int:
        """Return number of parameters.

        This is the dimensionality of the log density function.  It does
        not include transformed parameters or generated quantities.
        Equivalent to calling `param_num(0, 0)`

        Return:
        number of parameters
        """
        return self.param_num(False, False)

    def param_unc_num(self) -> int:
        """Return the number of unconstrained parameters.

        Does not include transformed parameters or generated
        quantities as these do not have unconstrained forms.

        Return:
            number of unconstrained parameters
        """
        return int(self._request("param_unc_num"))

    def param_names(self, tp: bool = True, gq: bool = True) -> List[str]:
        """Return the encoded constrained parameter names.

        Optionally includes names of transformed parameters and
        generated quantities as indicated.  Parameter names are included
        for each scalar and container indexes are separated by periods.
        For example, `a.2` is the second entry in a one-dimensional
        array `a` and `b.1.2` might be the value at the first row and
        second column of matrix `b`.

        Args:
            tp: `True` to include transformed parameters, `False` to exclude
            gq: `True` to include generated quantitites, `False` to exclude
        Return:
            array of parameter names
        """
        return self._request(f"param_names + {int(tp)} {int(tp)}").split(",")

    def param_unc_names(self) -> List[str]:
        """Return the encoded unconstrained parameter names.

        Does not include transformed parameters or generated quantities
        as these do not have unconstrained forms.  Parameter names are
        included for each scalar and container indexes are separated by
        periods.  For example, `a.2` is the second entry in a
        one-dimensional array `a` and `b.1.2` might be the value at the
        first row and second column of matrix `b`.

        Return:
            array of parameter names
        """
        return self._request("param_unc_names").split(",")

    def param_constrain(
        self, params_unc: Iterable[float], tp: bool = True, gq: bool = True
    ) -> npt.NDArray[np.float64]:
        """Return the constrained parameters for the specified unconstrained parameters.

        Optionally include the transformed parameters and generate the
        generated quantities using the pseudo-RNG built into the server.

        Args:
            params_unc: unconstrained parameters
            tp: `True` to include transformed parameters, `False` to exclude
            gq: `True` to include generated quantitites, `False` to exclude
        Return:
            array of constrained parameters in double precision
        """
        self._write(f"param_constrain {int(tp)} {int(gq)}")
        self._write_nums(params_unc)
        return self._get_return_floats()

    def param_unconstrain(
        self, param_dict: Mapping[str, Union[float, npt.ArrayLike]]
    ) -> npt.NDArray[np.float64]:
        """Return unconstrained parameters for parameters.

        The parameters are provided in dictionary form with the shapes
        expected by the Stan program.  Does not include transformed
        parameters or generated quantities in input or output as they
        are not defined on the unconstrained scale.

        Args:
            params_dict: parameter values
        Return:
            array of constrained parameters in double precision
        """
        self._write("param_unconstrain ")
        self._write(json.dumps(param_dict))
        return self._get_return_floats()

    def log_density(
        self, params_unc: Iterable[float], propto: bool = True, jacobian: bool = True
    ) -> float:
        """Return log density for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: unconstrained parameter values
            propto: `True` to exclude constant terms, `False` to include
            jacobian: `True` to include change-of-variables adjustment, `False` to exclude
        Return:
            log density of unconstrained parameters
        """
        self._write(f"log_density {int(propto)} {int(jacobian)} 0 0")
        self._write_nums(params_unc)
        return self._get_return_float()

    def log_density_gradient(
        self, params_unc: Iterable[float], propto: bool = True, jacobian: bool = True
    ) -> Tuple[float, npt.NDArray[np.float64]]:
        """Return a pair of log density and gradient for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: unconstrained parameter values
            propto: `True` to exclude constant terms, `False` to include
            jacobian: `True` to include change-of-variables adjustment, `False` to exclude
        Return:
            pair of log density and gradient of unconstrained parameters
        """
        self._write(f"log_density {int(propto)} {int(jacobian)} 1 0")
        self._write_nums(params_unc)
        zs = self._get_return_floats()
        return zs[0], zs[1:]

    def log_density_hessian(
        self, params_unc: Iterable[float], propto: bool = True, jacobian: bool = False
    ) -> Tuple[float, npt.NDArray[np.float64], npt.NDArray[np.float64]]:
        """Return a triple of log density, gradient, and Hessian for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: unconstrained parameter values
            propto: `True` to exclude constant terms, `False` to include
            jacobian: `True` to include change-of-variables adjustment, `False` to exclude
        Return:
            tuple of log density, gradient, and Hessian of unconstrained parameters
        """
        self._write(f"log_density {int(propto)} {int(propto)} 1 1")
        self._write_nums(params_unc)
        zs = self._get_return_floats()
        N = int(np.sqrt(len(zs) - 1))
        return zs[0], zs[1 : (N + 1)], np.reshape(zs[(N + 1) :], (N, N))
