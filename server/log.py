from __future__ import print_function
from sys import stderr

def err(*objs):
    print("ERR: ", *objs, file=stderr)
def warn(*objs):
    print("WARN: ", *objs, file=stderr)
def info(*objs):
    print("INFO: ", *objs, file=stderr)

