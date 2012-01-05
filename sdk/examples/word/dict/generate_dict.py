#!/usr/bin/env python
import time, random
from itertools import *
import os, sys
import fileinput
import sys

seed_size = 5

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

def generate_word_list_file():
    fi = open("words0.txt", "r")
    #print "second file " + fi.filename()
    
    word_list = {}
    for line in fi:
        word = line.strip()
        if len(word) == seed_size and word.find("'") == -1:
            print "word list: " + line
            word_list[word] = True        
    fi.close()

    fi = open("word_list.txt", "w")
    for word in word_list:
        fi.write(word + "\n")
    fi.close()
    
def generate_dict():
    """ Reads in a dictionary and a word list, then writes out a (hopefully smaller) dictionary that is composed of that word list and all possible anagram of each word"""
    
    dictionary = {}
    fi = open("dictionary.txt", "r")
    for line in fi:
        dictionary[line.strip()] = True
    fi.close()

    print "words in dict: " + str(len(dictionary))
    
    # uncomment to regenerate: generate_word_list_file()
    
    fi = open("word_list.txt", "r")
    #print "second file " + fi.filename()
    
    word_list = {}
    for line in fi:
        word = line.strip()
        if len(word) == seed_size and word.find("'") == -1:
            print "word list: " + line
            word_list[word] = True        
    fi.close()
    
    print "filtered word list to: " + str(len(word_list))
    
    output_dictionary = {}
    num_seed_words = 0
    max_anagrams = 0
    for word in word_list:
        anagrams = find_anagrams(word, dictionary)
        if len(anagrams) > 25:
            num_seed_repeats = 0
            # filter equivalent anagrams
            for w in anagrams:
                if len(w) == seed_size and w in output_dictionary:
                    num_seed_repeats += 1
            if num_seed_repeats <= 1:
                print word + ": " + str(len(anagrams))
                num_seed_words += 1
                num_anagrams = len(anagrams)
                if max_anagrams < num_anagrams:
                    max_anagrams = num_anagrams
                output_dictionary[word.upper()] = True
                for w in anagrams:
                    output_dictionary[w.upper()] = True
            
    # write dict to file
    print "word list filtered to  " + str(num_seed_words)
    print "max anagrams: " + str(num_anagrams)
    sorted_output_dict = output_dictionary.keys()
    sorted_output_dict.sort()
    print "output dict will have " + str(len(sorted_output_dict))
    
    fi = open("dict.cpp", "w")
    for word in sorted_output_dict:
        fi.write("\"" + word + "\",\n")
    fi.close()
    
    fi = open("word_list_used.txt", "w")
    for word in word_list:
        fi.write(word + "\n")
    fi.close()    

def main():
    print sys.argv[1:]
    generate_dict()

if __name__ == '__main__':
    main()

