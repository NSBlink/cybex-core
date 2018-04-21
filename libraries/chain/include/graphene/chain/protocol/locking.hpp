/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {
    struct active_locking_policy_initializer {
        fc::time_point_sec begin_timestamp;
        uint32_t           locking_cliff_seconds = 0;
    };

    typedef fc::static_variant<active_locking_policy_initializer> locking_policy_initializer;


    struct locking_balance_create_operation: public base_operation {
        struct fee_parameters { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };

        asset                           fee;
        account_id_type                 creator;
        account_id_type                 owner;
        asset                           amount;
        locking_policy_initializer      policy;

        account_id_type     fee_payer() const { return creator; }
        void validate() const {
            FC_ASSERT(fee.amount >= 0);
            FC_ASSERT(amount.amount > 0);
        }
    };

    struct locking_balance_withdraw_operation: public base_operation {
        struct fee_parameters_type { uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION; };

        asset                           fee;
        locking_balance_id_type         locking_balance;
        account_id_type                 owner;
        asset                           amount;

        account_id_type     fee_payer() const { return creator; };
        void validate() const {
            FC_ASSERT(fee.amount >= 0);
            FC_ASSERT(amount.amount > 0);
        }
    };
} }

FC_REFLECT( graphene::chain::locking_balance_create_operation::fee_parameters_type, (fee))
FC_REFLECT( graphene::chain::locking_balance_withdraw_operation::fee_parameters_type, (fee))

FC_REFLECT( graphene::chain::locking_balance_create_operation, (fee)(creator)(owner)(amount)(policy))
FC_REFLECT( graphene::chain::locking_balance_withdraw_operation, (fee)(locking_balance)(owner)(amount))

FC_REFLECT( graphene::chain::active_locking_policy_initializer, (begin_timestamp)(locking_cliff_seconds))
FC_REFLECT(graphene::chain::locking_policy_initializer)