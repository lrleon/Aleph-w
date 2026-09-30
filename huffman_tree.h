#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

// 195 bits declaration
const unsigned char huffman_cdp [25] = {
  176, 193, 12, 219, 108, 134, 217, 204, 108, 198, 182, 217, 96, 216, 102, 
  219, 48, 204, 198, 182, 217, 216, 48, 219, 6
};

// `huffman_cdp` is `const`, so it gets internal linkage by default in C++
// (one private copy per translation unit); `huffman_k` is an array of
// pointers to `const char`, not a `const` array, so without `inline` it
// gets external linkage and a "multiple definition" link error when this
// header is included from more than one translation unit.
inline const char * huffman_k[] = { 
"", "", "", "e", "a", "", "", "", "", "b", "", "j", "", "", "", "Ã", "D", "Y", "", "H", "!", "", ".", "", "", "", "F", "", "?", ":", "", "T", "", "±", "", "", "Z", "", "V", "", "", "U", "O", "M", "i", "", "n", "r", "", "", "", "", "t", "", "", "", "C", "z", "p", "", "g", "q", "o", " ", "", "", "", "u", "", "", "", "h", "", "E", "L", "", "", "f", "B", "A", "v", "", "c", "d", "", "", "l", "s", "", "", "", ",", "", "y", "G", "m", "\n", nullptr };

#endif // HUFFMAN_TREE_H
