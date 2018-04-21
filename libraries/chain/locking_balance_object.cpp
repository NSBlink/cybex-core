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

#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {
    inline bool sum_below_max_shares(const asset& a, const asset& b){
        assert(GRAPHENE_MAX_SHARE_SUPPLY + GRAPHENE_MAX_SHARE_SUPPLY > GRAPHENE_MAX_SHARE_SUPPLY);
        return (a.amount              <= GRAPHENE_MAX_SHARE_SUPPLY)
            && (            b.amount  <= GRAPHENE_MAX_SHARE_SUPPLY)
            && ((a.amount + b.amount) <= GRAPHENE_MAX_SHARE_SUPPLY);
    }

    asset active_locking_policy::get_allowed_withdraw(const locking_policy_context& ctx) const {
        share_type allowed_withdraw = 0;
        if (ctx.now > begin_timestamp) {
            const auto elapsed_seconds = (ctx.now - begin_timestamp).to_seconds();
            assert(elapsed_seconds > 0);
            // belongs to creator
            if (pre_locking_balance_id == NULL) {
                if (elapsed_seconds >= locking_cliff_seconds) {
                    allowed_withdraw = 0;
                } else {
                    allowed_withdraw = ctx.balance.amount;
                }
            } else { // belongs to owner
                auto pre_locking_balance = db().get_locking_balance(pre_locking_balance_id);
                if (pre_locking_balance.get_allowed_withdraw() >= begin_balance) {
                    allowed_withdraw = ctx.balance.amount;
                } else {
                    allowed_withdraw = 0;
                }
            }
        }
        return asset(allowed_withdraw, ctx.blance.asset_id);
    }

    void active_locking_policy::on_deposit(const locking_policy_context& ctx) {

    }
    
    bool active_locking_policy::is_deposit_allowed(const locking_policy_context& ctx) {
        return (ctx.amount.asset_id == ctx.balance.asset_id)
      && sum_below_max_shares(ctx.amount, ctx.balance); 
    }

    
} }