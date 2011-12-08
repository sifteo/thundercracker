#!/usr/bin/env python
import time, random
from itertools import *
import os, sys
import fileinput

def count_anagrams(string, dictionary):

    words = {}
    for k in range(len(string) + 1):
        if k >= 2:
            for subword in permutations(string, k):
                if ''.join(subword) in dictionary and not''.join(subword) in words:
                    # Check if it's a max lengthword and a bad word
                    words[''.join(subword)] = True
    #print string
    #print words
    return len(words)

def find_max_anagrams():
    """ Returns the maximum count of anagrams for any word in the
    one word per line file given by the command argument"""
    dictionary = {}
    for line in fileinput.input():
        dictionary[line.strip()] = True

    max_anagrams = 0
    for line in dictionary:
        num_anagrams = count_anagrams(line, dictionary)
        print line + ": " + str(num_anagrams)
        if num_anagrams > max_anagrams:
            max_anagrams = num_anagrams
    return max_anagrams


def main():
    print find_max_anagrams()

if __name__ == '__main__':
    main()

