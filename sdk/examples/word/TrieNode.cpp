#include "TrieNode.h"

static TrieNode children_Jame[] = { { 's', 0, 0 }, };
static TrieNode children_Jam[] = { { 'e', 1, children_Jame }, };
static TrieNode children_Jak[] = { { 'e', 0, 0 }, };
static TrieNode children_Ja[] = { { 'm', 1, children_Jam }, { 'k', 1, children_Jak }, };
static TrieNode children_J[] = { { 'a', 2, children_Ja }, };
static TrieNode children_Fre[] = { { 'd', 0, 0 }, };
static TrieNode children_Fr[] = { { 'e', 1, children_Fre }, };
static TrieNode children_F[] = { { 'r', 1, children_Fr }, };
static TrieNode children_[] = { { 'J', 1, children_J }, { 'F', 1, children_F }, };
static TrieNode root = { '\0', 2, children_ };
