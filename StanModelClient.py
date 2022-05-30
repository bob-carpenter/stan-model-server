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

    def writeNum(self, x):
        self.write(' ')
        self.write(str(x))

    def writeNums(self, xs):
        for x in xs:
            self.writeNum(x)
        # np.savetxt(self.server.stdin, xs, newline=" ", delimiter=" ")

    def getReturn(self):
        self.write("\n")
        self.server.stdin.flush()
        return self.read()

    def getReturnFloat(self):
        return float(self.getReturn())

    def getReturnFloats(self):
        ys = self.getReturn().split(',')
        return np.asfarray(ys, dtype=np.float64)

    def request(self, msg):
        self.write(msg)
        return self.getReturn()

    # REPL functions
    def name(self):
        """Return name of model being served.

        Returns:
            Name of model being served.
        """
        return self.request("name")

    def paramNum(self, tp, gq):
        """Return the number of constrained parameters.

        Optionally includes counts of transformed parameters and
        generated quantities.

        Args:
            tp: 1 to include transformed parameters, 0 to exclude
            gq: 1 to include generated quantities, 0 to exclude
        Returns:
            number of parameters
        """
        return int(self.request("param_num" + " " + str(tp) + " " + str(gq)))

    def paramUncNum(self):
        """Return the number of unconstrained parameters.

        Does not include transformed parameters or generated
        quantities as these do not have unconstrained forms.

        Returns:
            number of unconstrained parameters
        """
        return int(self.request("param_unc_num"))

    def paramNames(self, tp, gq):
        """Return the encoded constrained parameter names.

        Optionally includes names of transformed parameters and
        generated quantities as indicated.  Parameter names are included
        for each scalar and container indexes are separated by periods.
        For example, `a.2` is the second entry in a one-dimensional
        array `a` and `b.1.2` might be the value at the first row and
        second column of matrix `b`.

        Args:
            tp: 1 to include transformed parameters, 0 to exclude
            gq: 1 to include generated quantities, 0 to exclude
        Returns:
            array of parameter names
        """
        return self.request("param_names" + " " + str(tp) + " " + str(gq)).split(',')

    def paramUncNames(self):
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

    def paramConstrain(self, tp, gq, params_unc):
        """Return the constrained parameters for the specified unconstrained parameters.

        Optionally include the transformed parameters and generate the
        generated quantities using the pseudo-RNG built into the server.

        Args:
            tp: 1 to include transformed parameters, 0 to exclude
            gq: 1 to include generated quantitites, 0 to exclude
            params_unc: array of unconstrained parameters
        Returns:
            array of constrained parameters in double precision
        """
        self.write("param_constrain" + " " + str(tp) + " " + str(gq))
        self.writeNums(params_unc)
        return self.getReturnFloats()


    def paramUnconstrain(self, param_dict):
        """Return the unconstrained parameters for the specified parameters.

        The parameters are provided in dictionary form with the shapes
        expected by the Stan program.  Does not include transformed
        parameters or generated quantities in input or output as they
        are not defined on the unconstrained scale.

        Args:
            params_dict: dictionary of constrained parameters
        Returns:
            array of constrained parameters in double precision
        """
        self.write("param_unconstrain ")
        self.write(json.dumps(param_dict))
        return self.getReturnFloats()

    def logDensity(self, propto, jacobian, params_unc):
        """Return the log density for the specified unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            propto: 1 to exclude constant terms, 1 to include
            jacobian: 1 to include change-of-variables adjustment, 0 to exclude
            params_unc: array of unconstrained parameters

        Return:
            log density of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 0 0")
        self.writeNums(params_unc)
        return self.getReturnFloat()

    def logDensityGradient(self, propto, jacobian, params_unc):
        """Return a pair of the log density and gradient for the unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            propto: 1 to exclude constant terms, 1 to include
            jacobian: 1 to include change-of-variables adjustment, 0 to exclude
            params_unc: array of unconstrained parameters

        Return:
            pair of log density and gradient of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 1 0")
        self.writeNums(params_unc)
        zs = self.getReturnFloats()
        return zs[0], zs[1:]

    def logDensityHessian(self, propto, jacobian, params_unc):
        """Return a pair of the log density and Hessian for the unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            propto: 1 to exclude constant terms, 1 to include
            jacobian: 1 to include change-of-variables adjustment, 0 to exclude
            params_unc: array of unconstrained parameters

        Return:
            pair of log density and Hessian of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 0 1")
        self.writeNums(params_unc)
        zs = self.getReturnFloats()
        zs_H = zs[1:]
        N = int(np.sqrt(len(zs_H)))
        return zs[0], np.reshape(zs[1:], (N, N))

    def logDensityGradientHessian(self, propto, jacobian, params_unc):
        """Return a triple of log density, gradient, and Hessian for unconstrained parameters.

        The `propto` and `jacobian` flags indicate whether to include
        the constant terms and the change-of-variables adjustment in the
        result.

        Args:
            propto: 1 to exclude constant terms, 1 to include
            jacobian: 1 to include change-of-variables adjustment, 0 to exclude
            params_unc: array of unconstrained parameters

        Return:
            triple of log density, gradient, and Hessian of unconstrained parameters
        """
        self.write("log_density " + str(propto) + " " + str(jacobian) + " 1 1")
        self.writeNums(params_unc)
        zs = self.getReturnFloats()
        N = int(np.sqrt(len(zs) - 1))
        return zs[0], zs[1:(N + 1)], np.reshape(zs[(N + 1):], (N, N))
