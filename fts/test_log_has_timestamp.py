from datetime import datetime
import os
import pexpect
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestLogHasTimestamp(unittest.TestCase):

    def test_simple_proxying(self):
        # We start up a backend
        backend = create_and_start_backend(0)

        # ...and an rsp option that proxies everything to it.
        _, config_file = mkstemp(suffix=".lua")
        with open(config_file, "w") as f:
            f.writelines([
                'listenPort = "8000"'
                'backendAddress = "127.0.0.1"'
                'backendPort = "8888"'
            ])
        server = pexpect.spawn(RSP_BINARY, [config_file])
        try:
            now = datetime.now()
	    expected_timestamp = now.strftime("%Y-%m-%d %H:%M")
	    server.expect("\\[%s:.*\\] Started.  Listening on port 8000." % (expected_timestamp,), timeout=5)
        finally:
            server.kill(9)
            os.remove(config_file)

            backend.shutdown()
