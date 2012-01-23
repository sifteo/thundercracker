#!/usr/bin/env python
import math, random
from itertools import *
import os, sys
import fileinput
import sys
from ctypes import *
import operator

seed_word_lens = [4, 6]#, 9]
letters_per_cube = 2

def find_anagrams(string, dictionary):
    words = {}
    if letters_per_cube == 1:
        for k in range(len(string) + 1):
            if k in seed_word_lens:
                for subword in permutations(string, k):			
                    sw = ''.join(subword).upper()
                    if sw in dictionary and not sw in words:
                        # Check if it's a max lengthword and a bad word
                        words[sw] = True
    else:
        #print string
        # for all the seed_word_lens, up to the length of this string
        for k in range(len(string) + 1):
            if k in seed_word_lens:        
                # count up a number to figure out how to shift each cube set of letters
                for sub_perm_index in range(int(math.pow(letters_per_cube, len(string)/letters_per_cube))):
                    #print sub_perm_index
                    s = string
                    cube_ltrs = []
                    cube_index = 0
                    # break the string into cube sets of letters, and shift
                    while len(s) > 0:
                        st = s[:letters_per_cube]
                        shift = (sub_perm_index / math.pow(letters_per_cube, cube_index)) % letters_per_cube
                        shift = int(shift)
                        #print shift
                        #print "bef: " + st
                        st = st[shift:] + st[:shift]
                        #print "af: " + st
                        cube_ltrs.append(st)
                        s = s[letters_per_cube:]
                        cube_index += 1
                        #print s
                    #print cube_ltrs
                    # for all the permuations of cube sets of letters
                    for cube_ltr_set in permutations(cube_ltrs, k/letters_per_cube):			
                        #print cube_ltr_set
                        # form the string
                        sw = ''
                        for cltrs in cube_ltr_set:
                            #print cltrs
                            sw += cltrs
                        #print sw
                        # if it's in the dictionary, save it
                        if sw in dictionary and not sw in words:
                            #print sw + "\n"
                            # Check if it's a max lengthword and a bad word
                            words[sw] = True
                            
    #print string
    #print words
    return words

def generate_word_list_file():
    fi = open("words0.txt", "r")
    #print "second file " + fi.filename()
    
    word_list = {}
    for line in fi:
        word = line.strip()
        if len(word) in seed_word_lens and word.find("'") == -1 and word.find(".") == -1:
            #print "word list: " + line
            word_list[word.upper()] = True        
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
        dictionary[line.strip().upper()] = True
    fi.close()

    print "words in dict: " + str(len(dictionary))

    bad_words = {}
    fi = open("badwords.txt", "r")
    for line in fi:
        bad_words[line.strip().upper()] = True
    fi.close()
    
    # uncomment to regenerate: 
    #generate_word_list_file()
    
    fi = open("word_list.txt", "r")
    #print "second file " + fi.filename()
    
    word_list = {}
    for line in fi:
        word = line.strip()
        if len(word) in seed_word_lens and word.find("'") == -1 and word.find(".") == -1:
            #print "word list: " + line
            word_list[word.upper()] = True        
    fi.close()
    
    print "filtered word list to one size and no punctuation: " + str(len(word_list))
    
    output_dictionary = {}
    max_anagrams = 0
    word_list_used = {}
    #find_anagrams("LISTEN", dictionary)
    #return
    for word in word_list:
        anagrams = find_anagrams(word, dictionary)
        min_anagrams = [999, 999, 999, 999, 999, 1]
        #min_anagrams = [999, 999, 4, 15, 25, 25]
        #print "checking word " + word
        if len(anagrams) > min_anagrams[len(word) - 1]:
            #print word + " " + str(anagrams)
            num_seed_repeats = 0
            # skip it if a pre-existing seed word has the same anagram set 
            bad = False
            num_common_anagrams = 0
            for w in anagrams:
                if len(w) == len(word) and w.upper() in word_list_used.keys():
                    num_seed_repeats += 1
                    break
                if w in bad_words.keys():
                    bad = True
                    break
                if w in word_list:
                    num_common_anagrams += 1
            if num_seed_repeats == 0 and not bad:
                #print word + ": " + str(len(anagrams))
                word_list_used[word.upper()] = len(anagrams) - num_common_anagrams
                num_anagrams = len(anagrams)
                if max_anagrams < num_anagrams:
                    max_anagrams = num_anagrams
                output_dictionary[word.upper()] = True
                for w in anagrams:
                    output_dictionary[w.upper()] = True
            
    # write dict to file
    num_words = [0, 0, 0, 0, 0, 0]
    for w in word_list_used:
        num_words[len(w) - 1] += 1
    print "seed words with enough anagrams  " + str(num_words)
    print "max anagrams: " + str(max_anagrams)
    sorted_output_dict = output_dictionary.keys()
    sorted_output_dict.sort()
    print "output dict will have " + str(len(sorted_output_dict))
    
    fi = open("dict.cpp", "w")
    for word in sorted_output_dict:
        # write out compressed version of word (uses fact that demo only has up to 5 letter words)
        bits = 0
        letter_index = 0
        letter_bits = 5
        for letter in word:
            bits |= ((1 + ord(letter) - ord('A')) << (letter_index * letter_bits))
            letter_index += 1
        if word in word_list_used.keys():
            bits |= (1 << 31)
            #print "533D: " + word
            fi.write(hex(bits) + ",\t\t// " + word + ", seed word (533D: " + str(len(word)) + ")\n")
        else:
            fi.write(hex(bits) + ",\t\t// " + word + "\n")
    fi.close()
    
    # sort word list used by value numeric (keys by values in dict)
    sorted_word_list_used = sorted(word_list_used.iteritems(), key=operator.itemgetter(1), reverse=False)    
    #print sorted_word_list_used
    fi = open("word_list_used.cpp", "w")
    ficnt = open("word_list_used_anagram_count.cpp", "w")
    for word, value in sorted_word_list_used:
        fi.write("    \"" + word + "\",\n")
        ficnt.write("    " + str(len(find_anagrams(word, dictionary))) + ",\t// " + word + ", uncommon anagrams: " + str(word_list_used[word]) + "\n")
    fi.close()    
    ficnt.close()

    # skip the prototype code below, it just generates the word lists for the demo, if 
    # the seeds are set for each pick at run time
    return;
    fi = open("anagram_seeds.txt", "w")
    seed_inc = 88
    seed = 0
    max_picks = 34
    pick = 0
    pick_length = 3
    max_rounds = 5
    dict_len = len(output_dictionary.keys())
    print dict_len
    cdll.LoadLibrary("libc.dylib") # doctest: +LINUX	
    libc = CDLL("libc.dylib")     # doctest: +LINUX
    #libc = cdll.msvcrt # doctest: +WINDOWS
    while pick_length <= 5:
        round = 0
        fi.write("======== " + str(pick_length) + " cube puzzles ========\n")
        while round < max_rounds:
            pick = 0
            seed = max_picks * seed_inc * round
            fi.write("\n-------- Round " + str(round + 1) + " --------\n")
            while pick < max_picks:
                seed += seed_inc
                libc.srand(seed)
                seed += 1
                i = libc.rand() % dict_len
                while len(sorted_output_dict[i]) != pick_length or sorted_output_dict[i] not in word_list_used.keys():
                    #print sorted_output_dict[i]
                    i  = (i + 1) % dict_len
                fi.write(sorted_output_dict[i] + "\n")
                pick += 1
            round += 1
        pick_length += 1
        fi.write("\n")
    fi.close()
def main():
    #print sys.argv[1:]
    generate_dict()

if __name__ == '__main__':
    main()

