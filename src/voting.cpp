
void auditor::votecust(name voter, vector<name> newvotes) {
#ifdef VOTING_DISABLED
    eosio_assert(false,"ERR::VOTECUST_VOTING_IS_DISABLED::Voting is currently disabled.");
#endif

    require_auth(voter);

    eosio_assert(newvotes.size() <= configs().maxvotes, "ERR::VOTECUST_MAX_VOTES_EXCEEDED::Max number of allowed votes was exceeded.");
    std::set<name> dupSet{};
    for (name vote: newvotes) {
        eosio_assert(dupSet.insert(vote).second, "ERR::VOTECUST_DUPLICATE_VOTES::Added duplicate votes for the same candidate.");
        auto candidate = registered_candidates.get(vote.value, "ERR::VOTECUST_CANDIDATE_NOT_FOUND::Candidate could not be found.");
        eosio_assert(candidate.is_active, "ERR::VOTECUST_VOTING_FOR_INACTIVE_CAND::Attempting to vote for an inactive candidate.");
    }

    modifyVoteWeights(voter, newvotes);
}


void auditor::refreshvote(name voter) {
    if (configs().authaccount == name{0}) {
        require_auth(_self);
    } else {
        require_auth(configs().authaccount);
    }

    // Find a vote that has been cast by this voter previously.
    auto existingVote = votes_cast_by_members.find(voter.value);
    if (existingVote != votes_cast_by_members.end()) {
        //new votes is same as old votes, will just apply the deltas
        modifyVoteWeights(voter, existingVote->candidates);
    } 
}