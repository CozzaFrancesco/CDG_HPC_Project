#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <roaring/roaring.hh>
#include <omp.h>
#include <string>
#include <algorithm>
#include <regex>
#include <chrono>
#include <set>

using namespace std;
using namespace std::chrono;

using Itemset = set<string>;
using TransactionDB = vector<Itemset>;


TransactionDB read_csv(const string& filename) {
    ifstream infile(filename);
    TransactionDB dataset;
    string line;
    regex pattern("'([^']*)'");

    while (getline(infile, line)) {
        Itemset transaction;
        sregex_iterator iter(line.begin(), line.end(), pattern);
        sregex_iterator end;

        for (; iter != end; ++iter) {
            string item = (*iter)[1].str();
            transform(item.begin(), item.end(), item.begin(), ::tolower); // conversione in lowercase
            transaction.insert(item);
        }

        dataset.push_back(transaction);
    }
    cout << "Lettura file completata! \nInizio versione con Roaring BitMap e OpenMP dell'algoritmo APriori ..." << endl;
    return dataset;
}


uint64_t calculate_support(const roaring::Roaring& bitmap) {
    return bitmap.cardinality();
}

vector<pair<Itemset, roaring::Roaring>> generate_candidates(const vector<pair<Itemset, roaring::Roaring>>& frequent_itemsets, int k) {
    vector<pair<Itemset, roaring::Roaring>> candidates;

    for (int i = 0; i < frequent_itemsets.size(); ++i) {
        for (int j = i + 1; j < frequent_itemsets.size(); ++j) {
            auto& itemset_i = frequent_itemsets[i].first;
            auto& itemset_j = frequent_itemsets[j].first;

            auto it1 = itemset_i.begin();
            auto it2 = itemset_j.begin();
            bool prefissoUguale = true;

            for (int x = 0; x < k - 1; ++x, ++it1, ++it2) {
                if (*it1 != *it2) {
                    prefissoUguale = false;
                    break;
                }
            }

            if (!prefissoUguale) continue;

            Itemset combined = itemset_i;
            combined.insert(*itemset_j.rbegin());

            roaring::Roaring combined_bitmap = frequent_itemsets[i].second & frequent_itemsets[j].second;
            candidates.emplace_back(combined, combined_bitmap);
        }
    }
    return candidates;
}

vector<pair<Itemset, int>> apriori_roaring_bitmap(const TransactionDB& dataset, int minsup) {
    vector<pair<Itemset, int>> results;

    unordered_map<string, roaring::Roaring> item_bitmaps;
    for (size_t tid = 0; tid < dataset.size(); ++tid) {
        for (const string& item : dataset[tid]) {
            item_bitmaps[item].add(tid);
        }
    }

    vector<pair<Itemset, roaring::Roaring>> frequent_itemsets;
    for (auto& it : item_bitmaps) {
        const string& item = it.first;
        roaring::Roaring& bitmap = it.second;
        int support = calculate_support(bitmap);
        if (support >= minsup) {
            frequent_itemsets.push_back({ {item}, bitmap });
            results.push_back({ {item}, support });
        }
    }

    #pragma omp parallel for shared(frequent_itemsets)
    for (int i = 0; i < frequent_itemsets.size(); ++i) {
        frequent_itemsets[i].second.runOptimize();
    }

    int k = 1;
    vector<pair<Itemset, roaring::Roaring>> candidates = generate_candidates(frequent_itemsets, k);

    k++;
    while (!candidates.empty()) {
        vector<pair<Itemset, roaring::Roaring>> new_frequent_itemsets;

        #pragma omp parallel for
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            int support = calculate_support(candidates[i].second);
            if (support >= minsup) {
                #pragma omp critical
                new_frequent_itemsets.push_back(candidates[i]);
                #pragma omp critical
                results.push_back({ candidates[i].first, support });
            }
        }

        candidates = generate_candidates(new_frequent_itemsets, k);
        frequent_itemsets = new_frequent_itemsets;
        k++;
    }
    return results;

    }

int main() {
    int minsup = 50;
    auto dataset = read_csv("C:/Users/franc/Desktop/Uni/HPC/Input/input_25000.csv");
    
    auto start = high_resolution_clock::now();
    vector<pair<Itemset,int>> results = apriori_roaring_bitmap(dataset, minsup);
    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);
    

    ofstream outfile("C:/Users/franc/Desktop/Uni/HPC/Results/roaringBitMapResult.txt");
    for (const auto& result : results) {
        outfile << "Frequent itemset: ";
        for (const auto& item : result.first) {
            outfile << item << " ";
        }
        outfile << "Support: " << result.second << endl;
    }
    outfile.close();
    cout << "Analisi completata, frequent itemsets presenti all'interno del file roaringBitMapResult.txt " <<endl;
    cout << "Tempo di esecuzione APriori con Roaring BitMap e OpenMP: " << duration.count() << " ms" << endl;


    return 0;
}
