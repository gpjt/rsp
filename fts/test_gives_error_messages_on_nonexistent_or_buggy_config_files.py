import os
import requests
import pexpect
import sys
from tempfile import mkstemp
import unittest

from mock_backend import create_and_start_backend


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestGivesAppropriateErrorMessagesOnNonExistentOrBuggyConfigFiles(unittest.TestCase):

    def test_gives_appropriate_error_message_on_nonexistent_config_file(self):
        self.fail("TODO")


    def test_gives_appropriate_error_message_on_buggy_config_file(self):
        self.fail("TODO")

