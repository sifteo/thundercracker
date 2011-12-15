#!/usr/bin/env python
import time, random
from itertools import *
import os, sys
import fileinput
import sys

def find_anagrams(string, dictionary):

    words = {}
    for k in range(len(string) + 1):
        if k >= 2:
            for subword in permutations(string, k):
                if ''.join(subword) in dictionary and not''.join(subword) in words:
                    # Check if it's a max lengthword and a bad word
                    words[''.join(subword)] = True
    #print string
    #print words
    return words

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

def generate_dict():
    """ Reads in a dictionary and a word list, then writes out a (hopefully smaller) dictionary that is composed of that word list and all possible anagram of each word"""
    
    dictionary = {}
    fi = open("dictionary.txt", "r")
    for line in fi:
        dictionary[line.strip()] = True

    print "words in dict: " + str(len(dictionary))
    
    fi = open("words0.txt", "r")
    #print "second file " + fi.filename()
    
    word_list = {}
    for line in fi:
        print "word list: " + line
        word_list[line.strip()] = True
        
    print "words in dict: " + str(len(word_list))
    output_dictionary = {}
    for word in word_list:
        anagrams = find_anagrams(word, dictionary)
        print word + ": " + str(len(anagrams))
        for w in anagrams:
            output_dictionary[w] = True
            
    # TODO write dict to file
    print "output dict would have " + str(len(output_dictionary.keys()))
    
def main():
    print sys.argv[1:]
    generate_dict()

if __name__ == '__main__':
    main()

