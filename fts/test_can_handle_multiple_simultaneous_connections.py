import os
import pexpect
import requests
from tempfile import mkstemp
from threading import Thread
import time
import unittest

from mock_backend import create_and_start_backend

RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))


class TestCanHandleMultipleSimultaneousConnections(unittest.TestCase):

    def test_multiple_roughly_simultaneous_requests_complete_in_time_roughly_equivalent_to_one_request(self):
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
        try:
            responses = []
            errors = []
            def make_request():
                try:
                    responses.append(requests.get("http://127.0.0.1:8000"))
                except Exception, e:
                    errors.append(e)

            start = time.time()
            client_threads = []
            for i in range(4):
                t = Thread(target=make_request)
                t.daemon = True
                t.start()
                client_threads.append(t)

            for client_thread in client_threads:
                client_thread.join()

            self.assertTrue(time.time() - start < 6)

            if errors != []:
                raise errors[0]

            for response in responses:
                self.assertEqual(response.status_code, 200)
                self.assertEqual(response.text, "Hello from the mock server\n")
        finally:
            server.kill(9)
            os.remove(config_file)

            backend.shutdown()
