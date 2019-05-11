void auditor::assertPeriodTime() {
    uint32_t timestamp = now();
    uint32_t periodBlockCount = timestamp - _currentState.lastperiodtime;
    eosio_assert(periodBlockCount > configs().auditor_tenure,
                 "ERR::NEWTENURE_EARLY::New period is being called too soon. Wait until the period has completed.");
}

void auditor::allocateCustodians(bool early_election) {

    eosio::print("Configure custodians for the next period.");

    custodians_table custodians(_self, _self.value);
    auto byvotes = registered_candidates.get_index<"byvotesrank"_n>();
    auto cand_itr = byvotes.begin();

    int32_t electcount = configs().numelected;
    uint8_t currentCustodianCount = 0;

    if (!early_election) {
        eosio::print("Empty the custodians table to get a full set of new custodians based on the current votes.");
        auto cust_itr = custodians.begin();
        while (cust_itr != custodians.end()) {
            const auto &reg_candidate = registered_candidates.get(cust_itr->cust_name.value, "ERR::NEWTENURE_EXPECTED_CAND_NOT_FOUND::Corrupt data: Trying to set a lockup delay on candidate leaving office.");
            registered_candidates.modify(reg_candidate, cust_itr->cust_name, [&](candidate &c) {
                eosio::print("Lockup stake for release delay.");
                c.auditor_end_time_stamp = time_point_sec(now() + configs().lockup_release_time_delay);
            });
            cust_itr = custodians.erase(cust_itr);
        }
    }

    eosio::print("Select only enough candidates to fill the gaps.");
    for (auto itr = custodians.begin(); itr != custodians.end(); itr++) { ++currentCustodianCount; }

    while (currentCustodianCount < electcount) {
        if (cand_itr == byvotes.end() || cand_itr->total_votes == 0) {
            eosio::print("The pool of eligible candidates has been exhausted");
            return;
        }

        //  If the candidate is inactive or is already a custodian skip to the next one.
        if (!cand_itr->is_active || custodians.find(cand_itr->candidate_name.value) != custodians.end()) {
            cand_itr++;
        } else {
            custodians.emplace(_self, [&](custodian &c) {
                c.cust_name = cand_itr->candidate_name;
                c.total_votes = cand_itr->total_votes;
            });

            byvotes.modify(cand_itr, cand_itr->candidate_name, [&](candidate &c) {
                    eosio::print("Lockup stake for release delay.");
                    c.auditor_end_time_stamp = time_point_sec(now() + configs().lockup_release_time_delay);
            });

            currentCustodianCount++;
            cand_itr++;
        }
    }
}

void auditor::setCustodianAuths() {

    custodians_table custodians(_self, _self.value);

    name accountToChange = configs().authaccount;

    vector<eosiosystem::permission_level_weight> accounts;

    for (auto it = custodians.begin(); it != custodians.end(); it++) {
        eosiosystem::permission_level_weight account{
                .permission = eosio::permission_level(it->cust_name, "active"_n),
                .weight = (uint16_t) 1,
        };
        accounts.push_back(account);
    }

    eosiosystem::authority auditors_contract_authority{
            .threshold = configs().auth_threshold_auditors,
            .keys = {},
            .accounts = accounts
    };

    action(permission_level{accountToChange, "active"_n},
           "eosio"_n, "updateauth"_n,
           std::make_tuple(
                   accountToChange,
                   AUDITORS_PERMISSION,
                   "active"_n,
                   auditors_contract_authority))
            .send();
}

void auditor::newtenure(string message) {

    print("Start");

    assertPeriodTime();

    print("\nPeriodTime");

    contr_config config = configs();

    print("\nConfigs");

    // Get the max supply of the lockup asset token (eg. BOS)
    auto tokenStats = stats(name(TOKEN_CONTRACT), config.lockupasset.symbol.code().raw()).begin();

    uint64_t max_supply = tokenStats->supply.amount;

    print("\nRead stats");

    double percent_of_current_voter_engagement =
            double(_currentState.total_weight_of_votes) / double(max_supply) * 100.0;

    eosio::print("\n\nToken max supply: ", max_supply, " total votes so far: ", _currentState.total_weight_of_votes);
    eosio::print("\n\nNeed inital engagement of: ", config.initial_vote_quorum_percent, "% to start the Audit Cycle.");
    eosio::print("\n\nToken supply: ", max_supply * 0.0001, " total votes so far: ", _currentState.total_weight_of_votes * 0.0001);
    eosio::print("\n\nNeed initial engagement of: ", config.initial_vote_quorum_percent, "% to start the Audit Cycle..");
    eosio::print("\n\nNeed ongoing engagement of: ", config.vote_quorum_percent,
                 "% to allow new periods to trigger after initial activation.");
    eosio::print("\n\nPercent of current voter engagement: ", percent_of_current_voter_engagement, "\n\n");

    eosio_assert(_currentState.met_initial_votes_threshold == true ||
                 percent_of_current_voter_engagement > config.initial_vote_quorum_percent,
                 "ERR::NEWTENURE_VOTER_ENGAGEMENT_LOW_ACTIVATE::Voter engagement is insufficient to activate the Audit Cycle..");
    _currentState.met_initial_votes_threshold = true;

    eosio_assert(percent_of_current_voter_engagement > config.vote_quorum_percent,
                 "ERR::NEWTENURE_VOTER_ENGAGEMENT_LOW_PROCESS::Voter engagement is insufficient to process a new period");

    // Set custodians for the next period.
    allocateCustodians(false);

    // Set the auths on the BOS auditor authority account
    setCustodianAuths();

    _currentState.lastperiodtime = now();


//        Schedule the the next election cycle at the end of the period.
//        transaction nextTrans{};
//        nextTrans.actions.emplace_back(permission_level(_self,N(active)), _self, N(newtenure), std::make_tuple("", false));
//        nextTrans.delay_sec = configs().auditor_tenure;
//        nextTrans.send(N(newtenure), false);
}
