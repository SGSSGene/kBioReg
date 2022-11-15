//
// Created by Remy Schwab on 20.09.22.
//

#include "query.h"



void query_ibf(IndexStructure &ibf, std::vector<std::pair<std::string, uint64_t>> &path)
{
    // seqan3::debug_stream << path << ":::";
    // seqan3::debug_stream << path << std::endl;
    
    seqan3::interleaved_bloom_filter<seqan3::data_layout::uncompressed>::membership_agent::binning_bitvector hit_vector{ibf.getBinCount()};
    std::fill(hit_vector.begin(), hit_vector.end(), true);
    
    auto && ibf_ref = ibf.getIBF();
    auto agent = ibf_ref.membership_agent();

    for (auto && kmer : path)
    {
        auto & result = agent.bulk_contains(kmer.second);
        seqan3::debug_stream << kmer.first << "\t" << result << std::endl;
        hit_vector.raw_data() &= result.raw_data();
    }
    seqan3::debug_stream << "FINAL RESULT: " << hit_vector << std::endl;
}

void drive_query(const query_arguments &cmd_args)
{
    // Load index from disk
    seqan3::debug_stream << "Reading Index from Disk... ";
    IndexStructure ibf;
    load_ibf(ibf, cmd_args.idx);
    seqan3::debug_stream << "DONE" << std::endl;

    // Evaluate and search for Regular Expression
    seqan3::debug_stream << "Querying:" << std::endl;
    uint8_t qlength = ibf.k_;
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
    //deleteGraph(nfa);

    // Create kmer path matrix from kNFA
    seqan3::debug_stream << "   - Computing kmer path matrix from kNFA... ";
    std::vector<std::vector<std::string>> matrix{};
    for(auto i : knfa)
    {
        dfs(i,matrix);
    }
    uMatrix(matrix);
    seqan3::debug_stream << "DONE" << std::endl;

    // Search kmer paths in index
    seqan3::debug_stream << "   - Search kmers in index... ";
    seqan3::debug_stream << std::endl;

    // A vector to store kNFA paths as hashed constituent kmers eg one element would be. <78, 45, 83...> <--> AC, CG, GT
    auto hash_adaptor = seqan3::views::kmer_hash(seqan3::ungapped{qlength});
    std::vector<std::vector<std::pair<std::string, uint64_t>>> paths_vector;

    for(auto i : matrix)
    {
        std::vector<std::pair<std::string, uint64_t>> hash_vector;
        for(auto j : i)
        {
            std::vector<seqan3::dna5> acid_vec = convertStringToDNA(j);
            auto digest = acid_vec | hash_adaptor;
            // Create a vector of kmer hashes that correspond
            hash_vector.push_back(std::make_pair(j, digest[0]));
        }
        paths_vector.push_back(hash_vector);
    }

    for (auto path : paths_vector)
    {
        //seqan3::debug_stream << collapse_kmers(qlength, path) << ":::";
        query_ibf(ibf, path);
    }
    
    seqan3::debug_stream << "DONE" << std::endl;
}


std::string collapse_kmers(uint8_t const &k, std::vector<std::pair<std::string, uint64_t>> const &kvec)
{
    std::string assembly = kvec[0].first;
    for (size_t i = 1; i < kvec.size(); i++)
        assembly += kvec[i].first.substr(k-1, 1);
    return assembly;
}
