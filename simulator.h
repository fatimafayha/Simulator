from collections import deque
import sys
import os
import math

DEBUG = False
OPT = False
DEFAULT_CAP = 1024

class Replacement:
    LRU, FIFO, OPTIMAL = range(3)

class Inclusion:
    NONINCLUSIVE, INCLUSIVE = range(2)

class Block:
    def __init__(self, addr=0, tag=0, valid=False, dirty=False, replacement_count=0):
        self.addr = addr
        self.tag = tag
        self.valid = valid
        self.dirty = dirty
        self.replacement_count = replacement_count

class ArrayList:
    def __init__(self):
        self.ar = []
        self.size = 0
        self.cap = DEFAULT_CAP

    def append(self, addr):
        if len(self.ar) >= self.cap:
            self.resize()
        self.ar.append(addr)
        self.size += 1

    def delete(self, index):
        if 0 <= index < len(self.ar):
            del self.ar[index]
            self.size -= 1

    def trim(self):
        self.ar = self.ar[:self.size]

    def resize(self):
        self.cap *= 2
        self.ar.extend([None] * (self.cap - len(self.ar)))

    def clear(self):
        self.ar = []
        self.size = 0

class Vector:
    def __init__(self):
        self.list = ArrayList()
        self.push = self.list.append
        self.delete = self.list.delete
        self.trim = self.list.trim
        self.resize = self.list.resize
        self.clear = self.list.clear

class Address:
    def __init__(self, addr, index, tag):
        self.addr = addr
        self.index = index
        self.tag = tag

def gen_mask(bits):
    return (1 << bits) - 1

def print_input():
    print("Debug Input")

def print_file(file):
    print("File contents")

def usage():
    print("Usage details")

def init():
    print("Initialize")

def free_everything():
    print("Free resources")

def init_vectors():
    print("Initialize vectors")

def calc_addressing(addr, bits):
    mask = gen_mask(bits)
    index = (addr >> bits) & mask
    tag = addr >> (bits + int(math.log2(mask)))
    return Address(addr, index, tag)


