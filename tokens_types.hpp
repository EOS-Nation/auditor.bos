#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace std;
using namespace eosio;

struct account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
};

typedef eosio::multi_index<"accounts"_n, account> accounts;