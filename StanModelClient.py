"""Stan Model Client

This module supplies a client for the Stan Model Server.

Example:
"""
import numpy as np

import csv
import json
import subprocess


def readCsv(csv):
    """Return list of strings parsed from specified CSV string using.

    Args:
        csv: CSV-encoded string from which to parse array
    Returns:
        array of strings parsed from argument
    """
    return next(csv.reader([x]))

class StanClient:
    """Stan client class holding all resources.

    Attributes:
        server: Subprocess for Stan model server
    """
    server = []

    def __init__(self, modelExe, data, seed=1234):
        """Construct a Stan client with open subprocess to server.

        Args:
            modelExe: Path to Stan model server executable
            data: Path to JSON data file
            seed: Pseudo-random number generator seed; Defaults to 1234
        """
        cmd = [modelExe, '-d', data, '-s', str(seed)]
        self.server = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def __del__(self):
        """Close the server process, terminate it, and wait for shutdown."""
        self.server.stdin.close()
        self.server.terminate()
        self.server.wait(timeout=0.5)

    # I/O functions
    def read(self):
        return self.server.stdout.readline().decode("utf-8").strip()

    def write(self, msg):
        self.server.stdin.write(msg.encode("utf-8"))

    def write_num(self, x):
        self.write(' ')
        self.write(str(x))

    def write_nums(self, xs):
        for x in xs:
            self.write_num(x)
        # np.savetxt(self.server.stdin, xs, newline=" ", delimiter=" ")

    def get_return(self):
        self.write("\n")
        self.server.stdin.flush()
        return self.read()

    def get_return_float(self):
        return float(self.get_return())

    def get_return_floats(self):
        ys = self.get_return().split(',')
        return np.asfarray(ys, dtype=np.float64)

    def request(self, msg):
        self.write(msg)
        return self.get_return()

    # REPL functions
    def name(self):
        """Return name of model being served.

        Returns:
            Name of model being served.
        """
        return self.request("name")

    def param_num(self, tp = 1, gq = 1):
        """Return the number of constrained parameters.

        Optionally includes counts of transformed parameters and
        generated quantities.

        Args:
            tp: int, default = 1
                1 to include transformed parameters, 0 to exclude
            gq: int, default = 1
                1 to include generated quantitites, 0 to exclude
        Returns:
            number of parameters
        """
        return int(self.request("param_num" + " " + str(tp) + " " + str(gq)))

    def dims(self):
        """Return number of parameters.

        This is the dimensionality of the log density function.  It does
        not include transformed parameters or generated quantities.
        Equivalent to calling `param_num(0, 0)`

        Returns:
        number of parameters
        """
        return self.param_num(0, 0)


    def param_unc_num(self):
        """Return the number of unconstrained parameters.

        Does not include transformed parameters or generated
        quantities as these do not have unconstrained forms.

        Returns:
            number of unconstrained parameters
        """
        return int(self.request("param_unc_num"))

    def param_names(self, tp = 1, gq = 1):
        """Return the encoded constrained parameter names.

        Optionally includes names of transformed parameters and
        generated quantities as indicated.  Parameter names are included
        for each scalar and container indexes are separated by periods.
        For example, `a.2` is the second entry in a one-dimensional
        array `a` and `b.1.2` might be the value at the first row and
        second column of matrix `b`.

        Args:
            tp: int, default = 1
                1 to include transformed parameters, 0 to exclude
            gq: int, default = 1
                1 to include generated quantitites, 0 to exclude
        Returns:
            array of parameter names
        """
        return self.request("param_names" + " " + str(tp) + " " + str(gq)).split(',')

    def param_unc_names(self):
        """Return the encoded unconstrained parameter names.

        Does not include transformed parameters or generated quantities
        as these do not have unconstrained forms.  Parameter names are
        included for each scalar and container indexes are separated by
        periods.  For example, `a.2` is the second entry in a
        one-dimensional array `a` and `b.1.2` might be the value at the
        first row and second column of matrix `b`.

        Returns:
            array of parameter names
        """
        return self.request("param_unc_names").split(',')

    def param_constrain(self, params_unc, tp = 1, gq = 1):
        """Return the constrained parameters for the specified unconstrained parameters.

        Optionally include the transformed parameters and generate the
        generated quantities using the pseudo-RNG built into the server.

        Args:
            params_unc: array
                        unconstrained parameters
            tp: int, default = 1
                1 to include transformed parameters, 0 to exclude
            gq: int, default = 1
                1 to include generated quantitites, 0 to exclude
        Returns:
            array of constrained parameters in double precision
        """
        self.write("param_constrain" + " " + str(tp) + " " + str(gq))
        self.write_nums(params_unc)
        return self.get_return_floats()


    def param_unconstrain(self, param_dict):
        """Return unconstrained parameters for parameters.

        The parameters are provided in dictionary form with the shapes
        expected by the Stan program.  Does not include transformed
        parameters or generated quantities in input or output as they
        are not defined on the unconstrained scale.

        Args:
            params_dict: dictionary
                         parameter values
        Returns:
            array of constrained parameters in double precision
        """
        self.write("param_unconstrain ")
        self.write(json.dumps(param_dict))
        return self.get_return_floats()

    def log_density(self, params_unc, propto = 1, jacobian = 1):
        """Return log density for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: array
                        unconstrained parameter values
            propto: int, default = 1
                    1 to exclude constant terms, 0 to include
            jacobian: int, default = 1
                      1 to include change-of-variables adjustment, 0 to exclude

        Return:
            log density of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 0 0")
        self.write_nums(params_unc)
        return self.get_return_float()

    def log_density_gradient(self, params_unc, propto = 1, jacobian = 1):
        """Return a pair of log density and gradient for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: array
                        unconstrained parameter values
            propto: int, default = 1
                    1 to exclude constant terms, 0 to include
            jacobian: int, default = 1
                      1 to include change-of-variables adjustment, 0 to exclude

        Return:
            pair of log density and gradient of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 1 0")
        self.write_nums(params_unc)
        zs = self.get_return_floats()
        return zs[0], zs[1:]

    def log_density_hessian(self, params_unc, propto = 1, jacobian = 1):
        """Return a triple of log density, gradient, and Hessian for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            params_unc: array
                        unconstrained parameter values
            propto: int, default = 1
                    1 to exclude constant terms, 0 to include
            jacobian: int, default = 1
                      1 to include change-of-variables adjustment, 0 to exclude

        Return:
            triple of log density, gradient, and Hessian of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 1 1")
        self.write_nums(params_unc)
        zs = self.get_return_floats()
        N = int(np.sqrt(len(zs) - 1))
        return zs[0], zs[1:(N + 1)], np.reshape(zs[(N + 1):], (N, N))
