import argparse
parser = argparse.ArgumentParser(prog = "knitserver", description = "Brother KH-930 knitting server", add_help = False)
parser.add_argument("-f", "--force", action = "store_true", help = "Erase the socket if it already exists.")
parser.add_argument("--no-hardware", action = "store_true", help = "Do not initialize actual hardware. Used for debugging purposes only.")
parser.add_argument("-v", "--verbose", action = "count", default = 0, help = "Increase verbosity. Can be specified multiple times.")
parser.add_argument("unix_socket", metavar = "socket", type = str, help = "UNIX socket that the KnitPi knitting server listens on.")
