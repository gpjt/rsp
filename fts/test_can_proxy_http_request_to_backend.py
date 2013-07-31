from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from threading import Thread
import unittest


class MockServerRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write("Hello from the mock server\n")



class TestCanProxyHTTPRequestToBackend(unittest.TestCase):

    def test_simple_proxying(self):
        httpd = HTTPServer(('127.0.0.1', 8888), MockServerRequestHandler)
        server_thread = Thread(target=httpd.serve_forever)
        server_thread.start()

        # Do the test here.

        httpd.shutdown()
