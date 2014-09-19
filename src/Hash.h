#pragma once
#include <map>

class Chess;

class Hash {
    public:
        Hash();
        ~Hash();

        unsigned long long hash;
        unsigned long long hash_inner;
        unsigned long long hash_side_white;
        unsigned long long hash_side_black;
        unsigned long long hash_piece[2][7][120];
        unsigned long long hash_enpassant[120];
        unsigned long long hash_castle[15];
        unsigned long long hash_index;

        typedef struct {
            unsigned long long lock;
            int u;
        } hash_t;

        typedef struct {
            unsigned long long lock;
            int u;
            int depth;
            int move;
            //byte beta;
        } hash_inner_t;

        hash_t *hashtable;
        hash_inner_t *hashtable_inner;
        unsigned int hash_nodes;
        unsigned int hash_inner_nodes;
        unsigned int hash_collision;
        unsigned int hash_collision_inner;

        void set_hash(Chess* chess);
        unsigned long long rand64();
        unsigned long long hash_rand();
        void init_hash();
        void init_hash_inner();
        void reset_counters();
        bool posInHashtable();
        int getU();
        void setU(int u);
        void printStatistics(int nodes);
        std::map<unsigned long long, int> hashes;
};

