import os
import pexpect
import requests
import socket
import struct
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestCanHandleClientDisconnect(unittest.TestCase):

    def test_can_handle_client_disconnect(self):
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
            #l_onoff = 1
            #l_linger = 0
            #sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', l_onoff, l_linger))
            sock.send("GET / HTTP/1.1\r\n")
            sock.send("\r\n")
            sock.recv(1024)
            sock.close()

            # Check it didn't crash
            print "Making second request"
            response = requests.get("http://127.0.0.1:8000")
            self.assertEqual(response.status_code, 200)
            # self.assertEqual(response.text, "Hello from the mock server\n")
            print "Done"
        finally:
            server.kill(9)
            os.remove(config_file)

            backend.shutdown()
