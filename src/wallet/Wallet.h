// Copyright (c) 2012-2014, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "IWallet.h"
#include "INode.h"
#include "WalletErrors.h"
#include "WalletAsyncContextCounter.h"
#include "WalletTxSendingState.h"
#include "common/ObserverManager.h"
#include "cryptonote_core/tx_extra.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/Currency.h"
#include "WalletTransferDetails.h"
#include "WalletUserTransactionsCache.h"
#include "WalletUnconfirmedTransactions.h"
#include "WalletSynchronizer.h"
#include "WalletTransactionSender.h"
#include "WalletRequest.h"

namespace CryptoNote {

class Wallet : public IWallet {
public:
  Wallet(const cryptonote::Currency& currency, INode& node);
  ~Wallet() {};

  virtual void addObserver(IWalletObserver* observer);
  virtual void removeObserver(IWalletObserver* observer);

  virtual void initAndGenerate(const std::string& password);
  virtual void initAndLoad(std::istream& source, const std::string& password);
  virtual void initWithKeys(const AccountKeys& accountKeys, const std::string& password);
  virtual void shutdown();

  virtual void save(std::ostream& destination, bool saveDetailed = true, bool saveCache = true);

  virtual std::error_code changePassword(const std::string& oldPassword, const std::string& newPassword);

  virtual std::string getAddress();

  virtual uint64_t actualBalance();
  virtual uint64_t pendingBalance();

  virtual size_t getTransactionCount();
  virtual size_t getTransferCount();

  virtual TransactionId findTransactionByTransferId(TransferId transferId);

  virtual bool getTransaction(TransactionId transactionId, TransactionInfo& transaction);
  virtual bool getTransfer(TransferId transferId, Transfer& transfer);

  virtual TransactionId sendTransaction(const Transfer& transfer, uint64_t fee, const std::string& extra = "", uint64_t mixIn = 0, uint64_t unlockTimestamp = 0, const std::vector<TransactionMessage>& messages = std::vector<TransactionMessage>());
  virtual TransactionId sendTransaction(const std::vector<Transfer>& transfers, uint64_t fee, const std::string& extra = "", uint64_t mixIn = 0, uint64_t unlockTimestamp = 0, const std::vector<TransactionMessage>& messages = std::vector<TransactionMessage>());
  virtual std::error_code cancelTransaction(size_t transactionId);

  virtual void getAccountKeys(AccountKeys& keys);

  void startRefresh();

private:
  void throwIfNotInitialised();
  void refresh();
  uint64_t doPendingBalance();

  void doSave(std::ostream& destination, bool saveDetailed, bool saveCache);
  void doLoad(std::istream& source);

  crypto::chacha_iv encrypt(const std::string& plain, std::string& cipher);
  void decrypt(const std::string& cipher, std::string& plain, crypto::chacha_iv iv, const std::string& password);

  void synchronizationCallback(WalletRequest::Callback callback, std::error_code ec);
  void sendTransactionCallback(WalletRequest::Callback callback, std::error_code ec);
  void notifyClients(std::deque<std::shared_ptr<WalletEvent> >& events);

  void storeGenesisBlock();

  enum WalletState
  {
    NOT_INITIALIZED = 0,
    INITIALIZED,
    LOADING,
    SAVING
  };

  WalletState m_state;
  std::mutex m_cacheMutex;
  cryptonote::account_base m_account;
  std::string m_password;
  const cryptonote::Currency& m_currency;
  INode& m_node;
  bool m_isSynchronizing;
  bool m_isStopping;

  typedef std::vector<crypto::hash> BlockchainContainer;

  BlockchainContainer m_blockchain;
  WalletTransferDetails m_transferDetails;
  WalletUnconfirmedTransactions m_unconfirmedTransactions;
  tools::ObserverManager<CryptoNote::IWalletObserver> m_observerManager;

  WalletTxSendingState m_sendingTxsStates;
  WalletUserTransactionsCache m_transactionsCache;

  struct WalletNodeObserver: public INodeObserver, public IWalletObserver
  {
    WalletNodeObserver(Wallet* wallet) : m_wallet(wallet), postponed(false) {}
    virtual void lastKnownBlockHeightUpdated(uint64_t height) { m_wallet->startRefresh(); }
    virtual void saveCompleted(std::error_code result);
    void postponeRefresh();

    Wallet* m_wallet;

    std::mutex postponeMutex;
    bool postponed;
  };

  std::unique_ptr<WalletNodeObserver> m_autoRefresher;
  WalletAsyncContextCounter m_asyncContextCounter;

  WalletSynchronizer m_synchronizer;
  WalletTransactionSender m_sender;
};

} //namespace CryptoNote
