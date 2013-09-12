import os
import pexpect
import socket
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestGivesErrorOnLongRequests(unittest.TestCase):

    def test_gives_error_on_long_requests(self):
        backend = create_and_start_backend(5)

        _, config_file = mkstemp(suffix=".lua")
        with open(config_file, "w") as f:
            f.writelines([
                'listenPort = "8000"'
                'backendAddress = "127.0.0.1"'
                'backendPort = "8888"'
            ])
        server = pexpect.spawn(RSP_BINARY, [config_file])
        server.expect("Started.  Listening on port 8000.")

        sock = socket.socket()
        sock.connect(("localhost", 8000))
        sock.send("GET / HTTP/1.1\r")
        while True:
            try:
                sent = sock.send("A-Header: some values\r")
            except: 
                break
            if sent == 0:
                break
        return_value = sock.recv(10000)
        self.assertTrue(return_value.startswith("HTTP/1.0 414 "))
