import os
import pexpect
from tempfile import mkstemp
import unittest


RSP_BINARY = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "rsp"))



class TestGivesAppropriateErrorMessagesOnNonExistentOrBuggyConfigFiles(unittest.TestCase):

    def test_gives_appropriate_error_message_on_nonexistent_config_file(self):
        server = pexpect.spawn(RSP_BINARY, ["/definitely_does_not_exist"])
        server.expect("Error parsing config file: cannot open /definitely_does_not_exist: No such file or directory")


    def test_gives_appropriate_error_message_on_buggy_config_file(self):
        _, config_file = mkstemp(suffix=".lua")
        with open(config_file, "w") as f:
            f.write("dfs'df';f'#as'\n")
        server = pexpect.spawn(RSP_BINARY, [config_file])
        server.expect("Error parsing config file:")

