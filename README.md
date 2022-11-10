# Evaluation of Biological Regular Expressions with Korotkov NFAs
This app constructs a Korotkov Automata from an input Regular Expression. A Korotkov Automata is an NFA with Q-gram/K-mer transitions. It is based on this [lecture](https://wiki.postgresql.org/images/6/6c/Index_support_for_regular_expression_search.pdf)

A matrix is generated from the automata, which can be used for pattern matching. The regex need to be written in reverse polish notation and supports the following operations:

1. "|" - or
2. "*" - kleene star
3. "+" - min. 1
4. "?" - 0 or 1
5. "." - concatination


## Installation

1. Clone the repository with
```git clone --recurse-submodules git@github.com:remyschwab/kBioReg.git```
2. Descend into the home directory and input:
```mkdir build && cd build```
3. Configure with cmake ```cmake -DCMAKE_CXX_COMPILER=/path/to/g++-11 ..```
4. Build with make ```make```

## Usage
```bash
## Index
kbioreg index -k 3 -o dna_idx -m na ../data/ibf_example.fna
## Query RegEx
kbioreg query dna_idx.ibf "AC+G+|.T."
```

## Notes
This app was generated from the [SeqAn App Template](https://github.com/seqan/app-template) and makes heavy use of the [SeqAn library](https://github.com/seqan/seqan3/tree/4668203ee1526b4ac3dbdc47869bee72253f684c).
