#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"
#include "branch.h"

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  
  uint64_t branchNum = (uint64_t) accountNum >> 32;

  pthread_mutex_lock(& bank->branches[branchNum].branchLock);
  pthread_mutex_lock(& account->accLock);

  Account_Adjust(bank,account, amount, 1);

  pthread_mutex_unlock(& account->accLock);
  pthread_mutex_unlock(& bank->branches[branchNum].branchLock);

  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  
  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  
  uint64_t branchNum = (uint64_t) accountNum >> 32;
  
  pthread_mutex_lock(& bank->branches[branchNum].branchLock);
  pthread_mutex_lock(& account->accLock);
  
  if (amount > Account_Balance(account)) {
    pthread_mutex_unlock(& account->accLock);
    pthread_mutex_unlock(& bank->branches[branchNum].branchLock);

    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank,account, -amount, 1);

  pthread_mutex_unlock(& account->accLock);
  pthread_mutex_unlock(& bank->branches[branchNum].branchLock);

  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  } 

  if(srcAccount == dstAccount) {
    return ERROR_SUCCESS;
  }

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);

  uint64_t srcBranchNum = (uint64_t)srcAccountNum >> 32;
  uint64_t dstBranchNum = (uint64_t)dstAccountNum >> 32;

  if(updateBranch) {
    if(srcBranchNum < dstBranchNum) {
      pthread_mutex_lock(& bank->branches[srcBranchNum].branchLock);
      pthread_mutex_lock(& bank->branches[dstBranchNum].branchLock);
    } else {
      pthread_mutex_lock(& bank->branches[dstBranchNum].branchLock);
      pthread_mutex_lock(& bank->branches[srcBranchNum].branchLock);
    }
  }
  
  if(srcAccountNum < dstAccountNum) {
    pthread_mutex_lock(& srcAccount->accLock);
    pthread_mutex_lock(& dstAccount->accLock);
  } else {
    pthread_mutex_lock(& dstAccount->accLock);
    pthread_mutex_lock(& srcAccount->accLock);
  }

  if (amount > Account_Balance(srcAccount)) {
    pthread_mutex_unlock(& dstAccount->accLock);
    pthread_mutex_unlock(& srcAccount->accLock);
    if(updateBranch) {
      pthread_mutex_unlock(& bank->branches[srcBranchNum].branchLock);
      pthread_mutex_unlock(& bank->branches[dstBranchNum].branchLock);
    }
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank, srcAccount, -amount, updateBranch);
  Account_Adjust(bank, dstAccount, amount, updateBranch);

  pthread_mutex_unlock(& dstAccount->accLock);
  pthread_mutex_unlock(& srcAccount->accLock);
  if(updateBranch) {
    pthread_mutex_unlock(& bank->branches[srcBranchNum].branchLock);
    pthread_mutex_unlock(& bank->branches[dstBranchNum].branchLock);
  }
  return ERROR_SUCCESS;
}
