import os
import requests
import pexpect
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestCanHandleLargeResponse(unittest.TestCase):

    def test_large_response(self):
        # We start up a backend
        expected_response = "0123456789\n" * 1024 * 1024
        backend = create_and_start_backend(0, expected_response)

        # ...and an rsp option that proxies everything to it.
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
            # Make a request and check it works.
            response = requests.get("http://127.0.0.1:8000")
            self.assertEqual(response.status_code, 200)
            self.assertEqual(len(response.text), len(expected_response))
            self.assertEqual(response.text, expected_response, "Response incorrect")
        finally:
            server.kill(9)
            for line in server.readlines():
                print line
            os.remove(config_file)

            backend.shutdown()
