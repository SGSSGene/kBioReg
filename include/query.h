//
// Created by Remy Schwab on 20.09.22.
//

#pragma once

#include "utils.h"
#include "index.h"
#include "arg_parse.h"
#include "korotkov_nfa.h"
#include "graphMaker.h"
#include "nfa_pointer.h"


bitvector query_ibf(uint32_t &bin_count, robin_hood::unordered_map<uint64_t, uint32_t> &hash_to_idx,
  std::vector<bitvector> &kmer_bitvex, std::vector<std::pair<std::string, uint64_t>> &path);

bitvector drive_query(const query_arguments & cmd_args);

template <typename MolType>
void extract_matrix_paths(std::vector<std::vector<std::string>> &matrix,
 path_vector &paths_vector, auto &hash_adaptor)
{
  for(auto i : matrix)
    {
        std::vector<std::pair<std::string, uint64_t>> hash_vector;
        for(auto j : i)
        {
            std::vector<MolType> acid_vec = convertStringToAcidVec<MolType>(j);
            auto digest = acid_vec | hash_adaptor;
            // Create a vector of kmer hashes that correspond
            hash_vector.push_back(std::make_pair(j, digest[0]));
        }
        paths_vector.push_back(hash_vector);
    }
}
