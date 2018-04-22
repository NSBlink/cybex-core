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

#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/static_variant.hpp>
#include <fc/uint128.hpp>

#include <algorithm>


namespace graphene { namespace chain {
    using namespace graphene::db;

    class locking_balance_object;
    struct locking_policy_context {
        locking_policy_context(
            asset _balance,
            fc::time_point_sec _now,
            asset _amount)
            : balance(_balance), now(_now), amount(_amount) {}
        
        asset               balance;
        fc::time_point_sec  now;
        asset               amount;
    }

    struct active_locking_policy {
        // time at which funds begin locking
        fc::time_point_sec begin_timestamp;
        // locking time
        uint32_t locking_cliff_seconds = 0;
        // total amount of asset to lock
        share_type begin_balance;

        locking_balance_id_type pre_locking_balance_id;

        asset get_allowed_withdraw(const locking_policy_context& ctx) const;
        bool is_deposit_allowed(const locking_policy_context& ctx) const;
        bool is_deposit_locked_allowed(const locking_policy_context& ctx) const;
        bool is_withdraw_allowed(const locking_policy_context& ctx) const;
        void on_deposit(const locking_policy_context& ctx);
        void on_deposit_locked(const locking_policy_context& ctx); 
        void on_withdraw(const locking_policy_context& ctx);
    }

    typedef fc::static_variant<active_locking_policy> locking_policy;

    /**
     * Locking balance object is a balance that is locked by the blockchain for a period of time
     * and can be recalled by creator after the period of time
     */
    class locking_balance_object : public abstract_object<locking_balance_object> {
    public:
        static const uint8_t space_id = protocol_ids;
        static const uint8_t type_id = locking_balance_object_type;
        
        // creator is who sent the transaction
        // owner is who sent the transaction
        account_id_type owner;
        // if this balance belongs to owner, point to creator's balance
        locking_balance_id_type pre_locking_balance_id;
        //total amount of locking balance
        asset balance;
        // policy
        locking_policy policy;

        locking_balance_object() {}

        // Deposit amount into locking balance, requring before withdraw
        void deposit(const fc::time_point_sec& now, const assert& amount);       
        bool is_deposit_allowed(const fc::time_point_sec& now, const assert& amount) const;

        // Deposit amount into locking balance, making new funds locked immediately
        void on_deposit_locked(const fc::time_point_sec& now, const assert& amount);
        bool is_deposit_locked_allowed(const fc::time_point_sec& now, const assert& amount) const;

        // Remove locking balance and credit it to the proper account
        void withdraw(const fc::time_point_sec& now, const asset& amount);
        bool is_withdraw_allowed(const fc::time_point_sec& now, const asset& amount) const;

        asset get_allowed_withdraw(const time_point_sec& now) const;
    };

    struct by_account;

    typedef multi_index_container<
        locking_balance_object,
        indexed_by<
            ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
            ordered_non_unique< tag<by_account>,
                member<locking_balance_object, account_id_type, &locking_balance_object::owner>
            >
        >
    > locking_balance_multi_index_type;

    typedef generic_index<locking_balance_object, locking_balance_multi_index_type> locking_balance_index; 
} } 

FC_REFLECT(graphene::chain::active_locking_policy,
            (begin_timestamp)
            (locking_cliff_seconds)
            (begin_balance)  
            (pre_locking_balance_id)
          )

FC_REFLECT_TYPENAME(graphene::chain::locking_policy)

FC_REFLECT_DERIVED( graphene::chain::locking_balance_object,
                    (graphene::db::object),
                    (owner)
                    (pre_locking_balance_id)
                    (balance)
                    (policy)
                  )