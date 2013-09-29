from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import ThreadingMixIn
from threading import Thread
import time



class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    pass



def create_and_start_backend(request_speed, response=None):
    if response is None:
       response = "Hello from the mock server\n"


    class MockServerRequestHandler(BaseHTTPRequestHandler):
        def do_GET(self):
            time.sleep(request_speed)
            self.send_response(200)
            self.end_headers()
            self.wfile.write(response)

    httpd = ThreadedHTTPServer(('127.0.0.1', 8888), MockServerRequestHandler)
    server_thread = Thread(target=httpd.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    return httpd

