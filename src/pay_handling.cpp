#include <eosiolib/transaction.hpp>

void auditor::claimpay(uint64_t payid) {

    const pay &payClaim = pending_pay.get(payid, "ERR::CLAIMPAY_INVALID_CLAIM_ID::Invalid pay claim id.");

    name receiver = payClaim.receiver;

    require_auth(receiver);

    transaction deferredTrans{};

    string memo = payClaim.memo;

    print("constructed memo for the service contract: " + memo);

    deferredTrans.actions.emplace_back(
        action(permission_level{configs().tokenholder, "xfer"_n},
            name(TOKEN_CONTRACT), "transfer"_n,
            std::make_tuple(configs().tokenholder, receiver, payClaim.quantity, memo)
        ));

    deferredTrans.delay_sec = TRANSFER_DELAY;
    deferredTrans.send(uint128_t(payid) << 64 | now(), _self);

    pending_pay.erase(payClaim);
}
