#include <iostream>
#include <chrono>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>


using namespace std;
using namespace std::chrono;

using Itemset = set<string>;
using TransactionDB = vector<Itemset>;
using FrequencyMap = map<Itemset, int>;

// Funzione per leggere il file CSV e creare il database delle transazioni
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

    cout << "Lettura file completata! \nInizio versione sequenziale dell'algoritmo APriori ...";
    return dataset;
}



// Funzione per generare gli itemsets candidati di dimensione k+1
vector<Itemset> generate_candidates(const vector<Itemset>& freq_sets, int k) {
    vector<Itemset> candidates;
    int n = freq_sets.size();

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            Itemset candidate = freq_sets[i]; 
            for (const string& item : freq_sets[j]) {
                candidate.insert(item);
            }

            if (candidate.size() == k + 1) {
                vector<string> v1, v2;
                for (const string& item : freq_sets[i]) {
                    v1.push_back(item);
                }
                for (const string& item : freq_sets[j]) {
                    v2.push_back(item);
                }

                //verifica che il k-1 PREFISSO sia uguale
                bool is_equal = true;
                for (int x = 0; x < k - 1; ++x) {
                    if (v1[x] != v2[x]) {
                        is_equal = false;
                        break;
                    }
                }

                if (is_equal) {
                    candidates.push_back(candidate);
                }
            }
        }
    }
    return candidates;
}

// Funzione per filtrare i candidate itemset
vector<Itemset> filter_candidates(const TransactionDB& transactions, vector<Itemset>& candidates, int min_support) {
    FrequencyMap count_map;
    for (const auto& transaction : transactions) {
        for (const auto& candidate : candidates) {
            if (includes(transaction.begin(), transaction.end(), candidate.begin(), candidate.end())) {
                count_map[candidate]++;
            }
        }
    }

    vector<Itemset> frequent_itemsets;
    for (const auto& pair : count_map) {
        
        if (pair.second  >= min_support) {
            frequent_itemsets.push_back(pair.first);
        }
    }
    return frequent_itemsets;
}

// Algoritmo Apriori
vector<Itemset> apriori(const TransactionDB& transactions, int min_support) {
    vector<Itemset> frequent_itemsets;
    map<string, int> item_count;

    for (const auto& transaction : transactions) {
        for (const string& item : transaction) {
            item_count[item]++;
        }
    }

    vector<Itemset> current_freq_sets;
    for (const auto& pair : item_count) {
        
        if (pair.second >= min_support) {
            current_freq_sets.push_back({ pair.first });
        }

    }

    int k = 1;
    while (!current_freq_sets.empty()) {
        for (const auto& itemset : current_freq_sets) {
            frequent_itemsets.push_back(itemset);
        }

        vector<Itemset> candidates = generate_candidates(current_freq_sets, k);
        current_freq_sets = filter_candidates(transactions, candidates, min_support);
        k++;
    }

    return frequent_itemsets;
}

int main() {

    string filename = "C:/Users/franc/Desktop/Uni/HPC/Input/input_50000.csv";
    TransactionDB transactions = read_csv(filename);

    int min_support = 50;

    auto start = high_resolution_clock::now();
    vector<Itemset> results = apriori(transactions, min_support);
    auto stop = high_resolution_clock::now();
    
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Tempo di esecuzione APriori Classico: " << duration.count() << " ms" << endl;

    ofstream outfile("C:/Users/franc/Desktop/Uni/HPC/Results/sequentialResult.txt");
    for (const auto& result : results) {
        outfile << "Frequent itemset: ";
        for (const string& item : result) {
            outfile << item << ", ";
        }
        outfile  <<  endl;
    }

    outfile.close();
    return 0;
}
