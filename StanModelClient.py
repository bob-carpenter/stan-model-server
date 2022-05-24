import subprocess

class StanClient:
    server = []
    def __init__(self, modelExe, data, seed):
        cmd = [modelExe, '-d', data, '-s', seed]
        self.server = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    def __del__(self):
        self.server.stdin.close()
        self.server.terminate()
        self.server.wait(timeout=0.5)
    def request(self, msg):
        self.server.stdin.write(msg.encode("utf-8"))
        self.server.stdin.write("\n".encode("utf-8"))
        self.server.stdin.flush()
        return self.server.stdout.readline().decode("utf-8").strip()
    def name(self):
        return self.request("name")
