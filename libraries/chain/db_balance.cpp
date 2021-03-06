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

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>

namespace graphene { namespace chain {

asset database::get_balance(account_id_type owner, asset_id_type asset_id) const
{
   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(owner, asset_id));
   if( itr == index.end() )
      return asset(0, asset_id);
   return itr->get_balance();
}

asset database::get_balance(const account_object& owner, const asset_object& asset_obj) const
{
   return get_balance(owner.get_id(), asset_obj.get_id());
}
void database::set_balance(account_id_type owner, asset asset_obj) 
{
   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(owner, asset_obj.asset_id));
   if( itr != index.end() ){
      modify(*itr, [asset_obj](account_balance_object& b) {
         b.balance=asset_obj.amount;
      });
   }
}

string database::to_pretty_string( const asset& a )const
{
   return a.asset_id(*this).amount_to_pretty_string(a.amount);
}
//
// vesting balance objects of the same asset type should not be merged.
//  \because they have differnt begin time. 
//
void database::adjust_vesting_balance(account_id_type account, asset delta,struct linear_vesting_policy &vp )
{ try {
    
   if( delta.amount == 0 )
      return;

   account_object acc= account(*this);

   address addr;

   FC_ASSERT(acc.owner.key_auths.size()+acc.active.key_auths.size()>0,
          "no ${a}'s keys",
           ("a",account(*this).name));

   fc::ecc::public_key pk;
   if(acc.owner.key_auths.size()>0)
   {
       pk   = acc.owner.key_auths.begin()->first.operator fc::ecc::public_key();
   }
   else //if(acc.active.key_auths.size()>0)
   {
        pk  = acc.active.key_auths.begin()->first.operator fc::ecc::public_key();
   }
   addr =  pts_address( pk, false, 56 ) ;
   //addr =  pts_address( pk, true, 56 ) ;
   //addr =  pts_address( pk, false, 0 ) ;
   //addr =  pts_address( pk, true, 0 ) ;


   create<balance_object>([addr,&delta,vp](balance_object& b) {
         b.owner = addr;
         b.balance = delta;
         b.vesting_policy=vp;
      });


} FC_CAPTURE_AND_RETHROW( (account)(delta) ) }

void database::adjust_locking_balance(account_id_type creator, account_id_type owner, const public_key_type &  pub_key, asset delta, struct active_locking_policy& lp) {
      try {
            if (delta.amount == 0) 
                  return ;
            account_object acc_creator = creator(*this);

            address addr_creator, addr_owner;

            FC_ASSERT(acc_creator.owner.key_auths.size()+acc_creator.active.key_auths.size()>0,
                  "no ${a}'s keys",
                  ("a",creator(*this).name));
            fc::ecc::public_key pk;
            if(acc_creatoracc.owner.key_auths.size()>0)
            {
                  pk   = acc_creator.owner.key_auths.begin()->first.operator fc::ecc::public_key();
            }
            else //if(acc.active.key_auths.size()>0)
            {
                  pk  = acc_creator.active.key_auths.begin()->first.operator fc::ecc::public_key();
            }
            addr_creator =  pts_address( pk, false, 56 ) ;
            
            account_object acc_owner= owner(*this);

            fc::ecc::public_key pk_owner = fc::ecc::public_key((fc::ecc::public_key_data)pub_key);
            address addr_owner =  pts_address( pk_owner, false, 56 ) ;



            const lbo = create<locking_balance_object>([addr_creator, &delta, lp](balance_object& b) {
                  b.owner = addr_creator;
                  b.balance = delta;
                  b.locking_policy = lp;
                  b.pre_locking_balance_id = NULL;
            })

            create<balance_object>([addr_owner, &delta, lp](balance_object& b) {
                  b.owner = addr_owner;
                  b.balance = delta;
                  b.locking_policy = lp;
                  b.pre_locking_balance_id = lbo.id;
            })

      }
}

//
// vesting balance objects of the same asset type should not be merged.
//  \because they have differnt begin time. 
//
void database::adjust_vesting_balance(const account_id_type & account, const public_key_type &  pub_key,const asset delta,const struct linear_vesting_policy &vp )
{ try {
    
   if( delta.amount == 0 )
      return;

   account_object acc= account(*this);

   fc::ecc::public_key pk = fc::ecc::public_key((fc::ecc::public_key_data)pub_key);
   address addr =  pts_address( pk, false, 56 ) ;


   create<balance_object>([addr,&delta,vp](balance_object& b) {
         b.owner = addr;
         b.balance = delta;
         b.vesting_policy=vp;
      });

} FC_CAPTURE_AND_RETHROW( (account)(delta) ) }


void database::adjust_balance(account_id_type account, asset delta )
{ try {
   if( delta.amount == 0 )
      return;

   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(account, delta.asset_id));
   if(itr == index.end())
   {
      FC_ASSERT( delta.amount > 0, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", 
                 ("a",account(*this).name)
                 ("b",to_pretty_string(asset(0,delta.asset_id)))
                 ("r",to_pretty_string(-delta)));
      create<account_balance_object>([account,&delta](account_balance_object& b) {
         b.owner = account;
         b.asset_type = delta.asset_id;
         b.balance = delta.amount.value;
      });
   } else {
      if( delta.amount < 0 )
         FC_ASSERT( itr->get_balance() >= -delta, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", ("a",account(*this).name)("b",to_pretty_string(itr->get_balance()))("r",to_pretty_string(-delta)));
      modify(*itr, [delta](account_balance_object& b) {
         b.adjust_balance(delta);
      });
   }

} FC_CAPTURE_AND_RETHROW( (account)(delta) ) }

optional< vesting_balance_id_type > database::deposit_lazy_vesting(
   const optional< vesting_balance_id_type >& ovbid,
   share_type amount, uint32_t req_vesting_seconds,
   account_id_type req_owner,
   bool require_vesting )
{
   if( amount == 0 )
      return optional< vesting_balance_id_type >();

   fc::time_point_sec now = head_block_time();

   while( true )
   {
      if( !ovbid.valid() )
         break;
      const vesting_balance_object& vbo = (*ovbid)(*this);
      if( vbo.owner != req_owner )
         break;
      if( vbo.policy.which() != vesting_policy::tag< cdd_vesting_policy >::value )
         break;
      if( vbo.policy.get< cdd_vesting_policy >().vesting_seconds != req_vesting_seconds )
         break;
      modify( vbo, [&]( vesting_balance_object& _vbo )
      {
         if( require_vesting )
            _vbo.deposit(now, amount);
         else
            _vbo.deposit_vested(now, amount);
      } );
      return optional< vesting_balance_id_type >();
   }

   const vesting_balance_object& vbo = create< vesting_balance_object >( [&]( vesting_balance_object& _vbo )
   {
      _vbo.owner = req_owner;
      _vbo.balance = amount;

      cdd_vesting_policy policy;
      policy.vesting_seconds = req_vesting_seconds;
      policy.coin_seconds_earned = require_vesting ? 0 : amount.value * policy.vesting_seconds;
      policy.coin_seconds_earned_last_update = now;

      _vbo.policy = policy;
   } );

   return vbo.id;
}

void database::deposit_cashback(const account_object& acct, share_type amount, bool require_vesting)
{
   // If we don't have a VBO, or if it has the wrong maturity
   // due to a policy change, cut it loose.

   if( amount == 0 )
      return;

   if( acct.get_id() == GRAPHENE_COMMITTEE_ACCOUNT || acct.get_id() == GRAPHENE_WITNESS_ACCOUNT ||
       acct.get_id() == GRAPHENE_RELAXED_COMMITTEE_ACCOUNT || acct.get_id() == GRAPHENE_NULL_ACCOUNT ||
       acct.get_id() == GRAPHENE_TEMP_ACCOUNT )
   {
      // The blockchain's accounts do not get cashback; it simply goes to the reserve pool.
      modify(get(asset_id_type()).dynamic_asset_data_id(*this), [amount](asset_dynamic_data_object& d) {
         d.current_supply -= amount;
      });
      return;
   }

   optional< vesting_balance_id_type > new_vbid = deposit_lazy_vesting(
      acct.cashback_vb,
      amount,
      get_global_properties().parameters.cashback_vesting_period_seconds,
      acct.id,
      require_vesting );

   if( new_vbid.valid() )
   {
      modify( acct, [&]( account_object& _acct )
      {
         _acct.cashback_vb = *new_vbid;
      } );
   }

   return;
}

void database::deposit_witness_pay(const witness_object& wit, share_type amount)
{
   if( amount == 0 )
      return;

   optional< vesting_balance_id_type > new_vbid = deposit_lazy_vesting(
      wit.pay_vb,
      amount,
      get_global_properties().parameters.witness_pay_vesting_seconds,
      wit.witness_account,
      true );

   if( new_vbid.valid() )
   {
      modify( wit, [&]( witness_object& _wit )
      {
         _wit.pay_vb = *new_vbid;
      } );
   }

   return;
}

} }
