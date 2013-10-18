import os
import pexpect
import requests
import socket
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestCanHandleClientDisconnect(unittest.TestCase):

    def DONTtest_can_handle_client_disconnect(self):
        backend = create_and_start_backend(0, "0123456789\n" * 1024 * 1024)

        _, config_file = mkstemp(suffix=".lua")
        with open(config_file, "w") as f:
            f.writelines([
                'listenPort = "8000"'
                'backendAddress = "127.0.0.1"'
                'backendPort = "8888"'
            ])
        server = pexpect.spawn(RSP_BINARY, [config_file])
        server.expect("Started.  Listening on port 8000.")

        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            sock.connect(("localhost", 8000))
            sock.send("GET / HTTP/1.1\r\n")
            sock.send("\r\n")
            sock.recv(1024)
            sock.close()

            # Check it didn't crash
            response = requests.get("http://127.0.0.1:8000")
            self.assertEqual(response.status_code, 200)
        finally:
            server.kill(9)
            os.remove(config_file)

            backend.shutdown()
