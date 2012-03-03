#!/usr/bin/env python
import math, random
from itertools import *
import os, sys
import fileinput
import sys
from ctypes import *
import operator
import csv
import unicodedata
import codecs

seed_word_lens = [3, 4, 5, 6, 8, 9] # FIXME 7 letter words. why not?
min_nonbonus_anagrams = 2
min_freq_bonus = 6
word_list_leading_spaces = {}
generate_examples = False

def strip_accents(s):
   #s = unicode(s1, 'utf-8').lstrip(unicode(codecs.BOM_UTF8, "utf8"))
   return ''.join((c for c in unicodedata.normalize('NFD', s) if unicodedata.category(c) != 'Mn'))

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
                            if trimmed_sw in dictionary and not trimmed_sw in padded_string_anagrams_dict[padded_string]:
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

def get_word_freq_slow(word, freq_dicts):
    freq = 0
    for fd in freq_dicts:
        if word in fd:
            return freq
        freq += 1
    if len(word) > 0:
        print "WARNING: "+word+" not in frequency dictionaries"
    return 999

def get_word_freq(word, dictionary):
    return dictionary[word]
    
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
    
    max_length_word = 0
    for l in seed_word_lens:
        if l > max_length_word:
            max_length_word = l
    dictionary = {}
    fi = open("dictionary.txt", "r")
    for word in fi:
        if len(word) <= max_length_word and word.find("'") == -1 and word.find(".") == -1:
            dictionary[word.strip().upper()] = -1
    fi.close()


    bad_words = {}
    fi = open("badwords.txt", "r")
    for line in fi:
        bad_words[line.strip().upper()] = True
    fi.close()
    
    # uncomment to regenerate: 
    #generate_word_list_file()
    freq_files = os.listdir("frequency")
    freq_files = sorted(freq_files, key=lambda name: name.split('.')[1])
    #print freq_files
    freq_dicts = []
    for fn in freq_files:
        d = {}
        fi = codecs.open("frequency/" + fn, "r", encoding='latin-1')
        for line in fi:
            word = strip_accents(line.strip()).upper()
            #if "CLAIR" in word:
             #   print word
                #return
            if word in dictionary:
                #print "word list: " + line
                d[word] = True        
        fi.close()        
        freq_dicts.append(d)

    i = 0
    for fd in freq_dicts:
        print "using " + str(len(fd.keys())) + " words from " + freq_files[i]
        i += 1
    print_time = len(dictionary.keys())/39
    i = 0
    for word in dictionary.keys():    
        if (i % print_time) == 0:
            print ".",
        i += 1
        dictionary[word] = get_word_freq_slow(word, freq_dicts)
        if dictionary[word] >= len(freq_dicts):
            del dictionary[word]
    print ""
    print "words in dict: " + str(len(dictionary))

    """
    fi = open("word_list.txt", "r")
    #print "second file " + fi.filename()
    word_list = freq_dicts[0]
    word_list = {}
    for line in fi:
        word = line.strip()
        if len(word) in seed_word_lens and word.find("'") == -1 and word.find(".") == -1:
            #print "word list: " + line
            word_list[word.upper()] = True        
    fi.close()
    
    print "filtered word list to one size and no punctuation: " + str(len(word_list))
    """
    
    output_dictionary = {}
    max_anagrams = 0
    word_list_used = {} # puzzles actually used
    letters_per_cube = [1, 1, 2, 2, 2, 2, 3, 3, 3]
    min_anagrams = [999, 999, 2, 2, 2, 2, 2, 2, 2]    
    
    # test code
    #word = "DANCE"
    #find_anagrams(word, dictionary, letters_per_cube[len(word) - 1])
    #return

    puzzles = []
    print "generate_examples flag: " + str(generate_examples)
    if not generate_examples:
        puzzle_file_name = "puzzles.txt"
        print "reading " + puzzle_file_name
        fi = open(puzzle_file_name, "r")
        for line in fi:
            puzzles.append(line.strip().upper())
        fi.close()
    #print "puzzles file: " + str(puzzles)

    puzzles_to_use = dictionary.keys()
    skip_tests = False
    sort_puzzles = True
    if len(puzzles) > 0:
        puzzles_to_use = puzzles
        skip_tests = True
        sort_puzzles = False
        #print puzzles

    iw = 0
    print_time = max(1, len(puzzles_to_use)/39)
    for word in puzzles_to_use:
        if (iw % print_time) == 0:
                print ".",
        iw += 1
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
                    if not skip_tests and len(w) == len(word) and w.upper() in word_list_used:
                        num_seed_repeats += 1
                        break
                    if not skip_tests and w in bad_words:
                        bad = True
                        break
                    if get_word_freq(w, dictionary) <= min_freq_bonus:
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
    print ""
    num_words = [0, 0, 0, 0, 0, 0, 0, 0, 0]
    for w in word_list_used:
        num_words[len(w) - 1] += 1
    print "seed words with enough anagrams  " + str(num_words)
    print "max anagrams: " + str(max_anagrams)
    sorted_output_dict = output_dictionary.keys()
    sorted_output_dict.sort()
    print "output dict will have " + str(len(sorted_output_dict))
    #print "******* wlu: " + str(word_list_used)
    
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
        if get_word_freq(word, dictionary) <= min_freq_bonus:
            bits |= (1 << 63)
            #print "533D: " + word
            fi.write("    " + hex(bits) + ",\t\t// " + word + ", length: " + str(len(word)) + ")\n")
        else:
            fi.write("    " + hex(bits) + ",\t\t// " + word + ", (bonus), length: " + str(len(word)) + ")\n")
    fi.write("};\n")
    fi.close()
    
    # sort word list used by value numeric (keys by values in dict)
    if sort_puzzles:
        sorted_word_list_used = sorted(word_list_used.iteritems(), key=operator.itemgetter(1), reverse=False)    
    else:
        sorted_word_list_used = []
        #print "******* wlu: " + str(word_list_used)
        for w in puzzles_to_use:
            #print "puzzle: " + w
            if w in word_list_used:
                sorted_word_list_used.append((w,word_list_used[w]))
        #sorted_word_list_used = list(word_list_used.iteritems())    
        #print "swlu: " + str(sorted_word_list_used)
        #print list(word_list_used.iteritems())    

    #print sorted_word_list_used
    # TODO pack puzzles somehow
    fi = open("../DictionaryData.h", "w")
    fi.write("// This file is autogenerated from dict/generate_dict.py\n\n")
    num_puzzles = 0
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            num_puzzles += 1    
    fi.write("const unsigned NUM_PUZZLES = " + str(num_puzzles) + ";\n\n")

    fieldnames = ('World', 'Level', 'Meta', 'Puzzle No.' ,'Puzzle', 'No. Possible', 'No. Required', 'ScrambleYN', 'Letter', 'Piece 1', 'Piece 2', 'Piece 3', 'Piece 4', 'Piece 5', 'Piece 6', 'Notes', 'Solution 1', 'Solution 2', 'Solution 3', 'Solution 4', 'Solution 5', 'Solution 6', 
'Solution 7', 'Solution 8', 'Solution 9', 'Solution 10', 'Solution 11', 'Solution 12',
'Solution 13', 'Solution 14', 'Solution 15', 'Solution 16', 'No. Letters', 'No. Blanks', 'No. Repeat Ltr. Cubes', 'Full-length Freq.', 'Min. Freq.', 'Max. Freq.',
'No. Freq. 1', 'No. Freq. 2', 'No. Freq. 3', 'No. Freq. 4', 'No. Freq. 5', 'No. Freq. 6', 'No. Freq. 7', 'No. Freq. 8', 'No. Freq. 9', 'No. Freq. 10')

    if generate_examples:
        examples_file_name = "example_puzzles.csv"
        print "saving example puzzles to " + examples_file_name
        fex = open(examples_file_name, 'wb')
        try:
            writer = csv.DictWriter(fex, fieldnames=fieldnames)
            headers = dict( (n,n) for n in fieldnames )
            writer.writerow(headers)
        except IOError:    
            print "failed to write example file: ", sys.exc_info()[0]
            fex.close()

    fi.write("const static char* puzzles[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        ltrs_p_c = letters_per_cube[len(word) - 1]
        if  ltrs_p_c > 1:
            anagrams = find_anagrams(word, dictionary, ltrs_p_c).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []            
            for a in anagrams:
                if get_word_freq(a, dictionary) <= min_freq_bonus:
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
            if generate_examples:
                try:
                    row_dict = {}
                    row_dict['Puzzle'] = word
                    row_dict['No. Possible'] = str(len(anagrams))
                    anagram_freqs = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
                    min_freq = 999
                    max_freq = -1
                    for a in anagrams:
                        freq = get_word_freq(a, dictionary)
                        if freq > max_freq:
                            max_freq = freq
                        if freq < min_freq:
                            min_freq = freq
                        anagram_freqs[freq] += 1
                    row_dict['Min. Freq.'] = min_freq
                    row_dict['Max. Freq.'] = max_freq
                    
                    for freq in range(0, 10):  
                        try:
                            row_dict['No. Freq. ' + str(freq+1)] = anagram_freqs[freq]
                        except IndexError:
                            print "bad index" + str(freq) + " for " + str(anagram_freqs)
                    row_dict['No. Letters'] = str(len(word))
                    # FIXME more cubes
                    blanks = 6 - len(word)
                    if blanks < 0:
                        blanks = 9 - len(word)
                    num_repeat_ltr_cubes = 0
                    for cltrs in cube_ltrs:
                        prev_l = cltrs[0]
                        for l in cltrs:
                            #if l == ' ':
                             #   blanks += 1
                             # FIXME cube_ltrs doesn't include fully blank cubes
                            if l != prev_l:
                                prev_l = ''
                        if prev_l != '':
                            num_repeat_ltr_cubes += 1
                    row_dict['No. Blanks'] = blanks
                    row_dict['No. Repeat Ltr. Cubes'] = num_repeat_ltr_cubes
                    for i in range(0,6):
                        if i >= len(cube_ltrs):
                            break
                        row_dict['Piece ' + str(i+1)] = cube_ltrs[i]
                        
                    for i in range(0,16):
                        if i >= len(anagrams):
                            break
                        row_dict['Solution ' + str(i+1)] = anagrams[i]
                    full_length_freq = 999
                    for a in anagrams:
                        if len(a) == len(word):
                            freq = get_word_freq(a, dictionary)
                            if freq < full_length_freq:
                                full_length_freq = freq
                    row_dict['Full-length Freq.'] = full_length_freq
                    writer.writerow(row_dict)
                except IOError:
                    print "failed to write example file: ", sys.exc_info()[0]
                    fex.close()
    if generate_examples:
        fex.close()
    fi.write("};\n\n")

    fi.write("const static unsigned char puzzlesNumGoalAnagrams[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:            
            anagrams = find_anagrams(word, dictionary, ltrs_p_c).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []            
            for a in anagrams:
                if get_word_freq(a, dictionary) <= min_freq_bonus:
                    anagrams_nonbonus.append(a)
                else:
                    anagrams_bonus.append(a)
            fi.write("    " + str(len(anagrams_nonbonus)) + ",\t// " + word + ", nonbonus anagrams: " + str(anagrams_nonbonus) + "\n")
    fi.write("};\n\n")

    fi.write("const static unsigned char puzzlesNumBonusAnagrams[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            anagrams = find_anagrams(word, dictionary, ltrs_p_c).keys()
            anagrams_nonbonus = []
            anagrams_bonus = []            
            for a in anagrams:
                if get_word_freq(a, dictionary) <= min_freq_bonus:
                    anagrams_nonbonus.append(a)
                else:
                    anagrams_bonus.append(a)
            fi.write("    " + str(len(anagrams_bonus)) + ",\t// " + word + ", bonus anagrams: " + str(anagrams_bonus) + "\n")
    fi.write("};\n\n")

    fi.write("const static bool puzzlesUseLeadingSpaces[] =\n")
    fi.write("{\n")
    for word, value in sorted_word_list_used:
        if letters_per_cube[len(word) - 1] > 1:
            fi.write("    " + str(word in word_list_leading_spaces).lower() + ",\t// " + word + "\n")
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
                while len(sorted_output_dict[i]) != pick_length or sorted_output_dict[i] not in word_list_used:
                    #print sorted_output_dict[i]
                    i  = (i + 1) % dict_len
                fi.write(sorted_output_dict[i] + "\n")
                pick += 1
            round += 1
        pick_length += 1
        fi.write("\n")
    fi.close()

def _load_puzzles(self):
    file_name = "puzzles.csv"
    self._app.puzzles = []
    self.path_levels, scriptname = os.path.split(__file__)
    print "--loading %s ---" % (self._path_puzzles + file_name)
    try:
        with open(self._path_puzzles + file_name, mode='rb') as f:
            reader = unicode_csv_reader(f)
            append_new_round_for_next_puzzle = True
            for i, row in enumerate(reader):
                # TODO error checking and display 
                if is_row_a_round_terminator(row):
                    append_new_round_for_next_puzzle = True
                elif not is_row_a_comment(row):
                    if append_new_round_for_next_puzzle:
                        self._app.puzzles.append([])
                        #print "appending new round for row %d %s" % (i, row)
                        append_new_round_for_next_puzzle = False
                    round_puzzles = self._app.puzzles[len(self._app.puzzles) - 1]
                    if len(row) == 1:
                        # single word, split into letters
#                        print "single word row %s" % list(row[0])
                        round_puzzles.append(list(row[0]))
                    else:
                        # remove any blank pieces
                        #print "processing row %s" % row
                        processed_row = []
                        for s in row:
                            if s and len(s) > 0:
                                processed_row.append(s)
                        round_puzzles.append(processed_row)
                else:
                    print "row %s appears to be a comment" % row
                print row
    except IOError:
        return False
    # filter out puzzles that have more pieces than the
    # user has siftables, or fewer than 2 pieces
    num_sifts = len(self._app.siftables)
    for r, round in enumerate(self._app.puzzles):
        filtered_puzzles = []
        for i, p in enumerate(round):
            num_pieces = self._get_num_puzzle_pieces(i, r)
            if num_pieces <= num_sifts and num_pieces >= 2:
                filtered_puzzles.append(p)
            else:
                print "filtered out puzzle %s, for %d cubes" % (p, num_sifts)
        self._app.puzzles[r] = filtered_puzzles
    # remove empty rounds
    self._app.puzzles = [round for round in self._app.puzzles if len(round) > 0]
    print "+++ finished loading:"
    print self._app.puzzles
    if not self._verify_puzzles():
        return False
    return len(self._app.puzzles) > 0 and len(self._app.puzzles[0]) > 0
    
def unicode_csv_DictReader(utf8_data, dialect=csv.excel, **kwargs):
    csv_reader = csv.DictReader(utf8_data, dialect=dialect, **kwargs)
    for row in csv_reader:
        # convert to unicode, stripping any Byte Order Marker
        yield [unicode(cell, 'utf-8').lstrip(unicode(codecs.BOM_UTF8, "utf8")) for cell in row]
        
def main():
    if len(sys.argv) > 1:
        global generate_examples
        generate_examples = True
        #print sys.argv
        #print generate_examples
    generate_dict()

if __name__ == '__main__':
    main()

