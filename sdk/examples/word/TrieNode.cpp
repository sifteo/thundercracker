#include "TrieNode.h"

static TrieNode children_Jame[] = { { 's', { 0, true }, 0 }, };
static TrieNode children_Jam[] = { { 'e', { 1, false }, children_Jame }, };
static TrieNode children_Jak[] = { { 'e', { 0, true }, 0 }, };
static TrieNode children_Ja[] = { { 'm', { 1, false }, children_Jam }, { 'k', { 1, false }, children_Jak }, };
static TrieNode children_J[] = { { 'a', { 2, false }, children_Ja }, };
static TrieNode children_Fre[] = { { 'd', { 0, true }, 0 }, };
static TrieNode children_Fr[] = { { 'e', { 1, false }, children_Fre }, };
static TrieNode children_F[] = { { 'r', { 1, false }, children_Fr }, };
static TrieNode children_[] = { { 'J', { 1, false }, children_J }, { 'F', { 1, false }, children_F }, };
TrieNode root = { '\0', { 2, false }, children_ };
