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

    void active_locking_policy::on_deposit(const locking_policy_context& ctx) const {

    }
    
    bool active_locking_policy::is_deposit_allowed(const locking_policy_context& ctx) {
        return (ctx.amount.asset_id == ctx.balance.asset_id)
      && sum_below_max_shares(ctx.amount, ctx.balance); 
    }

    void active_locking_policy::on_withdraw(const locking_policy_context& ctx) const {

    }

    bool active_locking_policy::is_withdraw_allowed(const locking_policy_context& ctx) {
        return (ctx.amount.asset_id == ctx.balance.asset_id)
          && (ctx.amount <= get_allowed_withdraw(ctx)); 
    }
    
    #define LOCKING_VISITOR(NAME, MAYBE_CONST)                    \
    struct NAME ## _visitor                                       \
    {                                                             \
        typedef decltype(                                         \
            std::declval<active_locking_policy>().NAME(           \
                std::declval<locking_policy_context>())           \
            ) result_type;                                        \
                                                                  \
        NAME ## _visitor(                                         \
            const asset& balance,                                 \
            const time_point_sec& now,                            \
            const asset& amount                                   \
            )                                                     \
        : ctx(balance, now, amount) {}                            \
                                                                  \
        template< typename Policy >                               \
        result_type                                               \
        operator()(MAYBE_CONST Policy& policy) MAYBE_CONST        \
        {                                                         \
            return policy.NAME(ctx);                              \
        }                                                         \
        locking_policy_context ctx;                               \
    }

    LOCKING_VISITOR(on_deposit,);
    LOCKING_VISITOR(on_deposit_vested,);
    LOCKING_VISITOR(on_withdraw,);
    LOCKING_VISITOR(is_deposit_allowed, const);
    LOCKING_VISITOR(is_withdraw_allowed, const);
    LOCKING_VISITOR(get_allowed_withdraw, const);
    
    bool locking_balance_object::is_deposit_allowed(const time_point_sec& now, const asset& amount) const {
        return policy.visit(is_deposit_allowed_visitor(balance, now, amount));
    }
    bool locking_balance_object::is_withdraw_allowed(const time_point_sec& now, const asset& amount)const {
        bool result = policy.visit(is_withdraw_allowed_visitor(balance, now, amount));
        // if some policy allows you to withdraw more than your balance,
        //    there's a programming bug in the policy algorithm
        assert((amount <= balance) || (!result));
        return result;
    }
    void locking_balance_object::withdraw(const time_point_sec& now, const asset& amount) {
        assert(amount <= balance);
        on_withdraw_visitor vtor(balance, now, amount);
        policy.visit(vtor);
        balance -= amount;
    }
    asset locking_balance_object::get_allowed_withdraw(const time_point_sec& now) const {
        asset amount = asset();
        return policy.visit(get_allowed_withdraw_visitor(balance, now, amount));
    }

} }