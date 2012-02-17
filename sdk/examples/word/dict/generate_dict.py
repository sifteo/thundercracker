#!/usr/bin/env python
import math, random
from itertools import *
import os, sys
import fileinput
import sys
from ctypes import *
import operator

seed_word_lens = [3, 4, 5, 6, 8, 9]
min_nonbonus_anagrams = 2
word_list_leading_spaces = {}

def find_anagrams(string, dictionary, letters_per_cube):
    words = {}
    if letters_per_cube == 1:
        for k in range(len(string) + 1):
            if k > 1:
                for subword in permutations(string, k):            
                    sw = ''.join(subword).upper()
                    if sw in dictionary and not sw in words:
                        # Check if it's a max lengthword and a bad word
                        words[sw] = True
    else:
        #print string
        # for all the seed_word_lens, up to the length of this string
        padded_strings = []
        spaces = ((letters_per_cube - (len(string) % letters_per_cube)) % letters_per_cube)
        if spaces > 0:
            padded_strings.append(string + (" " * spaces))
            padded_strings.append((" " * spaces) + string)
            #print str(spaces) + " " + str(padded_strings)
        else:
            padded_strings.append(string)
        padded_string_anagrams_dict = {}
        for padded_string in padded_strings:
            padded_string_anagrams_dict[padded_string] = {}
            #print "'" + padded_string + "'" + " " + str(spaces) + " " + str(len(string)) + " " + str(letters_per_cube)
            for k in range(letters_per_cube * 2, len(padded_string) + 1, letters_per_cube):
                if k in seed_word_lens:        
                    #print k
                    # count up a number to figure out how to shift each cube set of letters
                    for sub_perm_index in range(int(math.pow(letters_per_cube, len(padded_string)/letters_per_cube))):
                        #print sub_perm_index
                        s = padded_string
                        cube_ltrs = []
                        cube_index = 0
                        # break the padded_string into cube sets of letters, and shift
                        while len(s) > 0:
                            st = s[:letters_per_cube]
                            shift = (sub_perm_index / math.pow(letters_per_cube, cube_index)) % letters_per_cube
                            shift = int(shift)
                            #print shift
                            #print "bef: " + st
                            st = st[shift:] + st[:shift]
                            #print "af: " + st
                            cube_ltrs.append(st)
                            #print "'" +st+"'"
                            s = s[letters_per_cube:]
                            cube_index += 1
                            #print s
                        #print cube_ltrs
                        # for all the permuations of cube sets of letters
                        #print "ceil " + str(math.ceil(k/letters_per_cube))
                        for cube_ltr_set in permutations(cube_ltrs, math.ceil(k/letters_per_cube)):            
                            #if k/letters_per_cube == 3 and len(string) == 9:
                            #print cube_ltr_set
                            # form the padded_string
                            sw = ''
                            for cltrs in cube_ltr_set:
                                #print cltrs
                                sw += cltrs
                            # if it's in the dictionary, save it
                            trimmed_sw = sw.strip()
                            if trimmed_sw in dictionary and not trimmed_sw in padded_string_anagrams_dict[padded_string].keys():
                                #if len(string) == 9:
                                 #   print sw
                                #print trimmed_sw + "\n"
                                # Check if it's a max lengthword and a bad word
                                padded_string_anagrams_dict[padded_string][trimmed_sw] = True
        # pick the padded string (on left or right) that has more anagrams
        max_anagrams = 0
        max_anagram_padded_string = ""
        for padded_string, padded_string_anagrams in padded_string_anagrams_dict.iteritems():
            #print "padded string '" + padded_string + "' dict: " + str(padded_string_anagrams)
            if len(padded_string_anagrams.keys()) > max_anagrams:
                max_anagrams = len(padded_string_anagrams.keys())
                max_anagram_padded_string = padded_string
        #print "max padded: '" + max_anagram_padded_string + "'"
        if len(max_anagram_padded_string) > 0:
            if max_anagram_padded_string[0] == " ":
                word_list_leading_spaces[string] = True
                #print word_list_leading_spaces
            words = padded_string_anagrams_dict[max_anagram_padded_string]
                                
    #print string
    #if len(string) == 9:
     #   print string +": " + str(words)
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
    letters_per_cube = [1, 1, 1, 2, 2, 2, 3, 3, 3]
    min_anagrams = [999, 999, 1, 999, 2, 2, 2, 2, 2]    

    
    # test code
    #word = "DANCE"
    #find_anagrams(word, dictionary, letters_per_cube[len(word) - 1])
    #return

    puzzles = {}
    fi = open("puzzles.txt", "r")
    for line in fi:
        puzzles[line.strip().upper()] = True
    fi.close()

    puzzles_to_use = word_list#dictionary.keys(): #word_list:#
    skip_tests = False
    if len(puzzles.keys()) > 0:
        puzzles_to_use = puzzles.keys()
        skip_tests = True
        print puzzles.keys()
		
    
    for word in puzzles_to_use:
        if skip_tests or len(word) in seed_word_lens:
            anagrams = find_anagrams(word, dictionary, letters_per_cube[len(word) - 1])
            #min_anagrams = [999, 999, 4, 15, 25, 25]
            #print "checking word " + word
            if skip_tests or len(anagrams) >= min_anagrams[len(word) - 1]:
                #print word + " " + str(anagrams)
                num_seed_repeats = 0
                # skip it if a pre-existing seed word has the same anagram set 
                bad = False
                num_nonbonus_anagrams = 0
                for w in anagrams:
                    if not skip_tests and len(w) == len(word) and w.upper() in word_list_used.keys():
                        num_seed_repeats += 1
                        break
                    if not skip_tests and w in bad_words.keys():
                        bad = True
                        break
                    if w in word_list:
                        num_nonbonus_anagrams += 1
                if skip_tests or (num_seed_repeats == 0 and not bad and num_nonbonus_anagrams >= min_nonbonus_anagrams):
                    #print word + ": " + str(len(anagrams))
                    word_list_used[word.upper()] = len(anagrams) - num_nonbonus_anagrams
                    num_anagrams = len(anagrams)
                    if max_anagrams < num_anagrams:
                        max_anagrams = num_anagrams
                    output_dictionary[word.upper()] = True
                    for w in anagrams:
                        output_dictionary[w.upper()] = True
            
    # write dict to file
    num_words = [0, 0, 0, 0, 0, 0, 0, 0, 0]
    for w in word_list_used:
        num_words[len(w) - 1] += 1
    print "seed words with enough anagrams  " + str(num_words)
    print "max anagrams: " + str(max_anagrams)
    sorted_output_dict = output_dictionary.keys()
    sorted_output_dict.sort()
    print "output dict will have " + str(len(sorted_output_dict))
    
    fi = open("../PrototypeWordListData.h", "w")
    fi.write("#include <sifteo.h>\n\n")
    fi.write("// This file is autogenerated from dict/generate_dict.py\n\n")
    
    fi.write("const unsigned PROTO_WORD_LIST_LENGTH = " + str(len(sorted_output_dict)) + ";\n\n")
    fi.write("const static uint64_t protoWordList[] =\n")
    fi.write("{\n")
    
    for word in sorted_output_dict:
        # write out compressed version of word (uses fact that demo only has up to 5 letter words)
        bits = 0
        letter_index = 0
        letter_bits = 5
        for letter in word:
            bits |= ((1 + ord(letter) - ord('A')) << (letter_index * letter_bits))
            letter_index += 1
        if word in word_list.keys():
            bits |= (1 << 63)
            #print "533D: " + word
            fi.write("    " + hex(bits) + ",\t\t// " + word + ", length: " + str(len(word)) + ")\n")
        else:
            fi.write("    " + hex(bits) + ",\t\t// " + word + ", (bonus), length: " + str(len(word)) + ")\n")
    fi.write("};\n")
    fi.close()
    
    # sort word list used by value numeric (keys by values in dict)
    sorted_word_list_used = sorted(word_list_used.iteritems(), key=operator.itemgetter(1), reverse=False)    
    #print sorted_word_list_used
    # TODO pack puzzles somehow
    fi = open("../DictionaryData.h", "w")
    fi.write("// This file is autogenerated from dict/generate_dict.py\n\n")
    num_puzzles = 0
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            num_puzzles += 1    
    fi.write("const unsigned NUM_PUZZLES = " + str(num_puzzles) + ";\n\n")
    
    fi.write("const static char* puzzles[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        ltrs_p_c = letters_per_cube[len(word) - 1]
        if  ltrs_p_c > 1:
            anagrams = find_anagrams(word, dictionary, ltrs_p_c).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []
            for a in anagrams:
                if a in word_list.keys():
                    anagrams_nonbonus.append(a)
                else:
                    anagrams_bonus.append(a)
            #print word + " anagrams:" + str(anagrams)
            #print " c. anagrams:" + str(anagrams_nonbonus) 
            #print " u. anagrams:" + str(anagrams_bonus)
            num_anagrams = len(anagrams)
            cube_ltrs = []
            padded_string = ""
            cube_index = 0
            # break the padded_string into cube sets of letters, and shift
            spaces = ((ltrs_p_c - (len(word) % ltrs_p_c)) % ltrs_p_c)
            if spaces > 0:
                if word in word_list_leading_spaces:
                    padded_string = (" " * spaces) + word
                else:
                    padded_string = word + (" " * spaces)
                #print str(spaces) + " " + str(padded_strings)
            else:
                padded_string = word
            s = padded_string
            while len(s) > 0:
                st = s[:ltrs_p_c]                
                #print "af: " + st
                cube_ltrs.append(st)
                #print "'" +st+"'"
                s = s[ltrs_p_c:]
                #print s
            
            fi.write("    \"" + word + "\",    // pieces: " + str(cube_ltrs) + ", solutions: " + str(anagrams) + "\n")
    fi.write("};\n\n")

    fi.write("const static unsigned char puzzlesNumGoalAnagrams[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            anagrams = find_anagrams(word, dictionary, letters_per_cube[len(word) - 1]).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []
            for a in anagrams:
                if a in word_list.keys():
                    anagrams_nonbonus.append(a)
                else:
                    anagrams_bonus.append(a)
            fi.write("    " + str(len(anagrams_nonbonus)) + ",\t// " + word + ", nonbonus anagrams: " + str(anagrams_nonbonus) + "\n")
    fi.write("};\n\n")

    fi.write("const static unsigned char puzzlesNumBonusAnagrams[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            anagrams = find_anagrams(word, dictionary, letters_per_cube[len(word) - 1]).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []
            for a in anagrams:
                if a in word_list.keys():
                    anagrams_nonbonus.append(a)
                else:
                    anagrams_bonus.append(a)
            fi.write("    " + str(len(anagrams_bonus)) + ",\t// " + word + ", bonus anagrams: " + str(anagrams_bonus) + "\n")
    fi.write("};\n\n")

    fi.write("const static bool puzzlesUseLeadingSpaces[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            fi.write("    " + str(word in word_list_leading_spaces.keys()).lower() + ",\t// " + word + "\n")
    fi.write("};\n\n")
    

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

