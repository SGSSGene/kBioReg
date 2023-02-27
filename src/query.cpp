//
// Created by Remy Schwab on 20.09.22.
//

#include "query.h"


bitvector query_ibf(size_t &bin_count, robin_hood::unordered_map<uint64_t, bitvector> &hash_to_bits, std::vector<std::pair<std::string, uint64_t>> &path)
{   
    seqan3::interleaved_bloom_filter<seqan3::data_layout::uncompressed>::membership_agent::binning_bitvector hit_vector{bin_count};
    std::fill(hit_vector.begin(), hit_vector.end(), true);
    for (auto && kmer : path)
    {
        auto & result = hash_to_bits[kmer.second];
        hit_vector.raw_data() &= result.raw_data();
    }
    return hit_vector;
}


void disk_search(const bitvector &hits, std::string &query, IndexStructure &ibf)
{
    size_t bins = hits.size();
    std::string bin_text;
    int match_count = 0;
    for(size_t i = 0; i < bins; i++)
    {
        if(hits[i])
        {
            const std::filesystem::path lib_path = ibf.acid_libs_[i];
            bin_text = stream_as_string(lib_path);
            match_count += RE2::PartialMatch(bin_text, query);
        }
    }
    seqan3::debug_stream << "Confirmed " << match_count << " hits ";
}


bitvector drive_query(const query_arguments &cmd_args)
{
    double t1, t2;
    // Load index from disk
    seqan3::debug_stream << "Reading Index from Disk... ";
    IndexStructure ibf;
    t1 = omp_get_wtime();
    load_ibf(ibf, cmd_args.idx);
    t2 = omp_get_wtime();
    seqan3::debug_stream << "DONE in " << t2-t1 << "s" << std::endl;

    auto bin_count = ibf.getBinCount();

    // Evaluate and search for Regular Expression
    seqan3::debug_stream << "Querying:" << std::endl;
    t1 = omp_get_wtime();
    uint8_t qlength = ibf.k_;
    std::string rx = cmd_args.regex;
    std::string query = cmd_args.query;
    std::vector<char> a = getAlphabet(query);

    // Postfix to Thompson NFA
    seqan3::debug_stream << "   - Constructing Thompson NFA from RegEx... ";
    State* nfa = post2nfaE(query);
    seqan3::debug_stream << "DONE" << std::endl;

    // Thompson NFA to Korotkov NFA
    seqan3::debug_stream << "   - Construction kNFA from Thompson NFA... ";
    std::vector<kState *> knfa = nfa2knfa(nfa, qlength);
    seqan3::debug_stream << "DONE" << std::endl;
    deleteGraph(nfa);

    // Create kmer path matrix from kNFA
    seqan3::debug_stream << "   - Computing kmer path matrix from kNFA... ";
    
    // Create auxiliary data structures to avoid redundant kmer lookup
    auto hash_adaptor = seqan3::views::kmer_hash(seqan3::ungapped{qlength});
    robin_hood::unordered_map<uint64_t, bitvector> hash_to_bitvector{};

    // Spawn IBF membership agent in this scope because it is expensive
    auto && ibf_ref = ibf.getIBF();
    auto agent = ibf_ref.membership_agent();
    
    std::vector<std::vector<std::string>> matrix{};
    for(auto i : knfa)
    {
        if(ibf.molecule_ == "na")
        {        
            dfs_na(i, matrix, hash_to_bitvector, agent);
        } else {
            dfs_aa(i, matrix, hash_to_bitvector, agent);
        }
    }
    uMatrix(matrix);
    seqan3::debug_stream << "DONE" << std::endl;

    // Search kmer paths in index
    seqan3::debug_stream << "   - Search kmers in index... ";
    path_vector paths_vector;
    if(ibf.molecule_ == "na")
    {
        extract_matrix_paths<seqan3::dna5>(matrix, paths_vector, hash_adaptor);
    } else {
        extract_matrix_paths<seqan3::aa27>(matrix, paths_vector, hash_adaptor);
    }

    seqan3::interleaved_bloom_filter<seqan3::data_layout::uncompressed>::membership_agent::binning_bitvector hit_vector{ibf.getBinCount()};
    std::fill(hit_vector.begin(), hit_vector.end(), false);

    for (auto path : paths_vector)
    {
        auto hits = query_ibf(bin_count, hash_to_bitvector, path);
        hit_vector.raw_data() |= hits.raw_data();
    }
    seqan3::debug_stream << "DONE" << std::endl;
    
    seqan3::debug_stream << "Verifying hits... ";
    disk_search(hit_vector, rx, ibf);
    t2 = omp_get_wtime();
    seqan3::debug_stream << "DONE in " << t2-t1 << "s" << std::endl;
    return hit_vector;
}
