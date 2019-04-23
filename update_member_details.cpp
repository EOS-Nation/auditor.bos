
void daccustodian::updatebio(name cand, string bio) {

    require_auth(cand);
    assertValidMember(cand);

    const auto &reg_candidate = registered_candidates.get(cand.value, "ERR::UPDATEBIO_NOT_CURRENT_REG_CANDIDATE::Candidate is not already registered.");
    eosio_assert(bio.size() < 256, "ERR::UPDATEBIO_BIO_SIZE_TOO_LONG::The bio should be less than 256 characters.");
}
