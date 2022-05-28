import csv
import numpy as np
import json
import subprocess

def readCsv(x):
    return next(csv.reader([x]))

class StanClient:
    server = []
    def __init__(self, modelExe, data, seed=1234):
        cmd = [modelExe, '-d', data, '-s', str(seed)]
        self.server = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    def __del__(self):
        self.server.stdin.close()
        self.server.terminate()
        self.server.wait(timeout=0.5)

    # I/O functions
    def read(self):
        return self.server.stdout.readline().decode("utf-8").strip()
    def write(self, msg):
        self.server.stdin.write(msg.encode("utf-8"))
    def writeNum(self, x):
        self.write(str(x))
    def writeNums(self, xs):
        for x in xs:
            self.writeNum(x)
        # np.savetxt(self.server.stdin, xs, newline=" ", delimiter=" ")
    def getReturn(self):
        self.write("\n")
        self.server.stdin.flush()
        return self.read()
    def getReturnFloats(self):
        ys = self.getReturn().split(',')
        return np.asfarray(ys, dtype=np.float64)
    def request(self, msg):
        self.write(msg)
        return self.getReturn()

    # REPL functions
    def name(self):
        return self.request("name")
    def paramNum(self, tp, gq):
        return int(self.request("param_num" + " " + str(tp) + " " + str(gq)))
    def paramUncNum(self):
        return int(self.request("param_unc_num"))
    def paramNames(self, tp, gq):
        return self.request("param_names" + " " + str(tp) + " " + str(gq)).split(',')
    def paramUncNames(self):
        return self.request("param_unc_names").split(',')
    def paramConstrain(self, tp, gq, params_unc):
        self.write("param_constrain" + " " + str(tp) + " " + str(gq) + " ")
        self.writeNums(params_unc)
        return self.getReturnFloats()
    def paramUnconstrain(self, param_dict):
        self.write("param_unconstrain ")
        self.write(json.dumps(param_dict))
        return self.getReturnFloats()
    def logDensity(self, propto, jacobian, params_unc):
        self.write("log_density" + " " + str(propto) + " " + str(jacobian) + " 0 0 ")
        self.writeNums(params_unc)
        return float(self.getReturn())
    def logDensityGradient(self, propto, jacobian, params_unc):
        self.write("log_density" + " " + str(propto) + " " + str(jacobian) + " 1 0 ")
        self.writeNums(params_unc)
        zs = self.getReturnFloats()
        return zs[0], zs[1:]
    def logDensityHessian(self, propto, jacobian, params_unc):
        self.write("log_density" + " " + str(propto) + " " + str(jacobian) + " 0 1 ")
        self.writeNums(params_unc)
        zs = self.getReturnFloats()
        return zs[0], zs[1:]
