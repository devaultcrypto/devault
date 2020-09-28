// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include <ban.h>
#include <fs.h>
#include <addrman.h>
#include <addrdb.h>
#include <amount.h>
#include <bloom.h>
#include <chainparams.h>
#include <compat.h>
#include <hash.h>
#include <limitedmap.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <streams.h>
#include <sync.h>
#include <threadinterrupt.h>
#include <uint256.h>
#include <crypto/siphash.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <thread>

#ifndef WIN32
#include <arpa/inet.h>
#endif

class BanMan;
class Config;
class CNode;
class CScheduler;

/**
 * Time between pings automatically sent out for latency probing and keepalive
 * (in seconds).
 */
static const int PING_INTERVAL = 2 * 60;
/**
 * Time after which to disconnect, after waiting for a ping response (or
 * inactivity).
 */
static const int TIMEOUT_INTERVAL = 20 * 60;
/** Run the feeler connection loop once every 2 minutes or 120 seconds. **/
static const int FEELER_INTERVAL = 120;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
static_assert(MAX_PROTOCOL_MESSAGE_LENGTH > MAX_INV_SZ * sizeof(CInv),
              "Max protocol message length must be greater than largest "
              "possible INV message");
/** The maximum number of entries in a locator */
static const unsigned int MAX_LOCATOR_SZ = 101;
/** The maximum number of new addresses to accumulate before announcing. */
static const unsigned int MAX_ADDR_TO_SEND = 1000;
/** Maximum length of strSubVer in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
/** Maximum number of automatic outgoing nodes */
static const int MAX_OUTBOUND_CONNECTIONS = 8;
/** Maximum number of addnode outgoing nodes */
static const int MAX_ADDNODE_CONNECTIONS = 8;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** -upnp default */
#ifdef USE_UPNP
static const bool DEFAULT_UPNP = USE_UPNP;
#else
static const bool DEFAULT_UPNP = false;
#endif
/** The maximum number of entries in mapAskFor */
static const size_t MAPASKFOR_MAX_SZ = MAX_INV_SZ;
/** The maximum number of entries in setAskFor (larger due to getdata latency)*/
static const size_t SETASKFOR_MAX_SZ = 2 * MAX_INV_SZ;
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
/** The default for -maxuploadtarget. 0 = Unlimited */
static const uint64_t DEFAULT_MAX_UPLOAD_TARGET = 0;
/** The default timeframe for -maxuploadtarget. 1 day. */
static const uint64_t MAX_UPLOAD_TIMEFRAME = 60 * 60 * 24;
/** Default for blocks only*/
static const bool DEFAULT_BLOCKSONLY = false;

static const bool DEFAULT_FORCEDNSSEED = false;
static const size_t DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const size_t DEFAULT_MAXSENDBUFFER = 1 * 1000;

typedef int64_t NodeId;

struct AddedNodeInfo {
    std::string strAddedNode;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};

struct CNodeStats;
class CClientUIInterface;

struct CSerializedNetMsg {
    CSerializedNetMsg() = default;
    CSerializedNetMsg(CSerializedNetMsg &&) = default;
    CSerializedNetMsg &operator=(CSerializedNetMsg &&) = default;
    // No copying, only moves.
    CSerializedNetMsg(const CSerializedNetMsg &msg) = delete;
    CSerializedNetMsg &operator=(const CSerializedNetMsg &) = delete;

    std::vector<uint8_t> data;
    std::string command;
};

class NetEventsInterface;
class CConnman {
public:
    enum NumConnections {
        CONNECTIONS_NONE = 0,
        CONNECTIONS_IN = (1U << 0),
        CONNECTIONS_OUT = (1U << 1),
        CONNECTIONS_ALL = (CONNECTIONS_IN | CONNECTIONS_OUT),
    };

    struct Options {
        ServiceFlags nLocalServices = NODE_NONE;
        int nMaxConnections = 0;
        int nMaxOutbound = 0;
        int nMaxAddnode = 0;
        int nMaxFeeler = 0;
        int nBestHeight = 0;
        CClientUIInterface *uiInterface = nullptr;
        NetEventsInterface *m_msgproc = nullptr;
        BanMan *m_banman = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundTimeframe = 0;
        uint64_t nMaxOutboundLimit = 0;
        std::vector<std::string> vSeedNodes;
        std::vector<CSubNet> vWhitelistedRange;
        std::vector<CService> vBinds, vWhiteBinds;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
    };

    void Init(const Options &connOptions) {
        nLocalServices = connOptions.nLocalServices;
        nMaxConnections = connOptions.nMaxConnections;
        nMaxOutbound =
            std::min(connOptions.nMaxOutbound, connOptions.nMaxConnections);
        nMaxAddnode = connOptions.nMaxAddnode;
        nMaxFeeler = connOptions.nMaxFeeler;
        nBestHeight = connOptions.nBestHeight;
        clientInterface = connOptions.uiInterface;
        m_banman = connOptions.m_banman;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        {
            LOCK(cs_totalBytesSent);
            nMaxOutboundTimeframe = connOptions.nMaxOutboundTimeframe;
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRange = connOptions.vWhitelistedRange;
        {
            LOCK(cs_vAddedNodes);
            vAddedNodes = connOptions.m_added_nodes;
        }
    }

    CConnman(const Config &configIn, uint64_t seed0, uint64_t seed1);
    ~CConnman();

    bool Start(CScheduler &scheduler, const Options &options);
    void Stop();
    void Interrupt();
    bool GetNetworkActive() const { return fNetworkActive; };
    void SetNetworkActive(bool active);
    void OpenNetworkConnection(const CAddress &addrConnect, bool fCountFailure,
                               CSemaphoreGrant *grantOutbound = nullptr,
                               const char *strDest = nullptr,
                               bool fOneShot = false, bool fFeeler = false,
                               bool manual_connection = false);
    bool CheckIncomingNonce(uint64_t nonce);

    bool ForNode(NodeId id, std::function<bool(CNode *pnode)> func);

    void PushMessage(CNode *pnode, CSerializedNetMsg &&msg);

    template <typename Callable> void ForEachNode(Callable &&func) {
        LOCK(cs_vNodes);
        for (auto &&node : vNodes) {
            if (NodeFullyConnected(node)) {
                func(node);
            }
        }
    };

    template <typename Callable> void ForEachNode(Callable &&func) const {
        LOCK(cs_vNodes);
        for (auto &&node : vNodes) {
            if (NodeFullyConnected(node)) {
                func(node);
            }
        }
    };

    template <typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable &&pre, CallableAfter &&post) {
        LOCK(cs_vNodes);
        for (auto &&node : vNodes) {
            if (NodeFullyConnected(node)) {
                pre(node);
            }
        }
        post();
    };

    template <typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable &&pre, CallableAfter &&post) const {
        LOCK(cs_vNodes);
        for (auto &&node : vNodes) {
            if (NodeFullyConnected(node)) {
                pre(node);
            }
        }
        post();
    };

    // Addrman functions
    size_t GetAddressCount() const;
    void SetServices(const CService &addr, ServiceFlags nServices);
    void MarkAddressGood(const CAddress &addr);
    void AddNewAddresses(const std::vector<CAddress> &vAddr,
                         const CAddress &addrFrom, int64_t nTimePenalty = 0);
    std::vector<CAddress> GetAddresses();

    // This allows temporarily exceeding nMaxOutbound, with the goal of finding
    // a peer that is better than all our current peers.
    void SetTryNewOutboundPeer(bool flag);
    bool GetTryNewOutboundPeer();

    // Return the number of outbound peers we have in excess of our target (eg,
    // if we previously called SetTryNewOutboundPeer(true), and have since set
    // to false, we may have extra peers that we wish to disconnect). This may
    // return a value less than (num_outbound_connections - num_outbound_slots)
    // in cases where some outbound connections are not yet fully connected, or
    // not yet fully disconnected.
    int GetExtraOutboundCount();

    bool AddNode(const std::string &node);
    bool RemoveAddedNode(const std::string &node);
    std::vector<AddedNodeInfo> GetAddedNodeInfo();

    size_t GetNodeCount(NumConnections num);
    void GetNodeStats(std::vector<CNodeStats> &vstats);
    bool DisconnectNode(const std::string &node);
    bool DisconnectNode(const CSubNet &subnet);
    bool DisconnectNode(const CNetAddr &addr);
    bool DisconnectNode(NodeId id);

    ServiceFlags GetLocalServices() const;

    //! set the max outbound target in bytes.
    void SetMaxOutboundTarget(uint64_t limit);
    uint64_t GetMaxOutboundTarget();

    //! set the timeframe for the max outbound target.
    void SetMaxOutboundTimeframe(uint64_t timeframe);
    uint64_t GetMaxOutboundTimeframe();

    //! check if the outbound target is reached.
    // If param historicalBlockServingLimit is set true, the function will
    // response true if the limit for serving historical blocks has been
    // reached.
    bool OutboundTargetReached(bool historicalBlockServingLimit);

    //! response the bytes left in the current max outbound cycle
    // in case of no limit, it will always response 0
    uint64_t GetOutboundTargetBytesLeft();

    //! response the time in second left in the current max outbound cycle
    // in case of no limit, it will always response 0
    uint64_t GetMaxOutboundTimeLeftInCycle();

    uint64_t GetTotalBytesRecv();
    uint64_t GetTotalBytesSent();

    void SetBestHeight(int height);
    int GetBestHeight() const;

    /** Get a unique deterministic randomizer. */
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;

    unsigned int GetReceiveFloodSize() const;

    void WakeMessageHandler();

private:
    struct ListenSocket {
        SOCKET socket;
        bool whitelisted;

        ListenSocket(SOCKET socket_, bool whitelisted_)
            : socket(socket_), whitelisted(whitelisted_) {}
    };

    bool BindListenPort(const CService &bindAddr, std::string &strError,
                        bool fWhitelisted = false);
    bool Bind(const CService &addr, unsigned int flags);
    bool InitBinds(const std::vector<CService> &binds,
                   const std::vector<CService> &whiteBinds);
    void ThreadOpenAddedConnections();
    void AddOneShot(const std::string &strDest);
    void ProcessOneShot();
    void ThreadOpenConnections(std::vector<std::string> connect);
    void ThreadMessageHandler();
    void AcceptConnection(const ListenSocket &hListenSocket);
    void DisconnectNodes();
    void NotifyNumConnectionsChanged();
    void InactivityCheck(CNode *pnode);
    void SocketHandler();
    void ThreadSocketHandler();
    void ThreadDNSAddressSeed();

    uint64_t CalculateKeyedNetGroup(const CAddress &ad) const;

    CNode *FindNode(const CNetAddr &ip);
    CNode *FindNode(const CSubNet &subNet);
    CNode *FindNode(const std::string &addrName);
    CNode *FindNode(const CService &addr);

    bool AttemptToEvictConnection();
    CNode *ConnectNode(CAddress addrConnect, const char *pszDest,
                       bool fCountFailure, bool manual_connection);
    bool IsWhitelistedRange(const CNetAddr &addr);

    void DeleteNode(CNode *pnode);

    NodeId GetNewNodeId();

    size_t SocketSendData(CNode *pnode) const;
    void DumpAddresses();

    // Network stats
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes);

    // Whether the node should be passed out in ForEach* callbacks
    static bool NodeFullyConnected(const CNode *pnode);

    const Config *config;

    // Network usage totals
    CCriticalSection cs_totalBytesRecv;
    CCriticalSection cs_totalBytesSent;
    uint64_t nTotalBytesRecv GUARDED_BY(cs_totalBytesRecv);
    uint64_t nTotalBytesSent GUARDED_BY(cs_totalBytesSent);

    // outbound limit & stats
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundCycleStartTime GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundLimit GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundTimeframe GUARDED_BY(cs_totalBytesSent);

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<CSubNet> vWhitelistedRange;

    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};

    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    bool fAddressesInitialized{false};
    CAddrMan addrman;
    std::deque<std::string> vOneShots GUARDED_BY(cs_vOneShots);
    CCriticalSection cs_vOneShots;
    std::vector<std::string> vAddedNodes GUARDED_BY(cs_vAddedNodes);
    CCriticalSection cs_vAddedNodes;
    std::vector<CNode *> vNodes;
    std::list<CNode *> vNodesDisconnected;
    mutable CCriticalSection cs_vNodes;
    std::atomic<NodeId> nLastNodeId{0};
    unsigned int nPrevNodeCount{0};

    /** Services this instance offers */
    ServiceFlags nLocalServices;

    std::unique_ptr<CSemaphore> semOutbound;
    std::unique_ptr<CSemaphore> semAddnode;
    int nMaxConnections;
    int nMaxOutbound;
    int nMaxAddnode;
    int nMaxFeeler;
    std::atomic<int> nBestHeight;
    CClientUIInterface *clientInterface;
    NetEventsInterface *m_msgproc;
    BanMan *m_banman;

    /** SipHasher seeds for deterministic randomness */
    const uint64_t nSeed0, nSeed1;

    /** flag for waking the message processor. */
    bool fMsgProcWake;

    std::condition_variable condMsgProc;
    Mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc{false};

    CThreadInterrupt interruptNet;

    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadMessageHandler;

    /**
     * Flag for deciding to connect to an extra outbound peer, in excess of
     * nMaxOutbound.
     * This takes the place of a feeler connection.
     */
    std::atomic_bool m_try_another_outbound_peer;

    friend struct CConnmanTest;
};

extern std::unique_ptr<CConnman> g_connman;
extern std::unique_ptr<BanMan> g_banman;
void Discover();
void StartMapPort();
void InterruptMapPort();
void StopMapPort();
unsigned short GetListenPort();
bool BindListenPort(const CService &bindAddr, std::string &strError,
                    bool fWhitelisted = false);

/**
 * Interface for message handling
 */
class NetEventsInterface {
public:
    virtual bool ProcessMessages(const Config &config, CNode *pnode,
                                 std::atomic<bool> &interrupt) = 0;
    virtual bool SendMessages(const Config &config, CNode *pnode,
                              std::atomic<bool> &interrupt) = 0;
    virtual void InitializeNode(const Config &config, CNode *pnode) = 0;
    virtual void FinalizeNode(const Config &config, NodeId id,
                              bool &update_connection_time) = 0;

protected:
    /**
     * Protected destructor so that instances can only be deleted by derived
     * classes. If that restriction is no longer desired, this should be made
     * public and virtual.
     */
    ~NetEventsInterface() = default;
};

enum {
    // unknown
    LOCAL_NONE,
    // address a local interface listens on
    LOCAL_IF,
    // address explicit bound to
    LOCAL_BIND,
    // address reported by UPnP
    LOCAL_UPNP,
    // address explicitly specified (-externalip=)
    LOCAL_MANUAL,

    LOCAL_MAX
};

bool IsPeerAddrLocalGood(CNode *pnode);
void AdvertiseLocal(CNode *pnode);

/**
 * Mark a network as reachable or unreachable (no automatic connects to it)
 * @note Networks are reachable by default
 */
void SetReachable(enum Network net, bool reachable);
/** @returns true if the network is reachable, false otherwise */
bool IsReachable(enum Network net);
/** @returns true if the address is in a reachable network, false otherwise */
bool IsReachable(const CNetAddr &addr);

bool AddLocal(const CService &addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr &addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService &addr);
bool SeenLocal(const CService &addr);
bool IsLocal(const CService &addr);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer = nullptr);
CAddress GetLocalAddress(const CNetAddr *paddrPeer,
                         ServiceFlags nLocalServices);

extern bool fDiscover;
extern bool fListen;
extern bool fRelayTxes;

extern limitedmap<uint256, int64_t> mapAlreadyAskedFor;

struct LocalServiceInfo {
    int nScore;
    int nPort;
};

extern CCriticalSection cs_mapLocalHost;
extern std::map<CNetAddr, LocalServiceInfo>
    mapLocalHost GUARDED_BY(cs_mapLocalHost);

// Command, total bytes
typedef std::map<std::string, uint64_t> mapMsgCmdSize;

/**
 * POD that contains various stats about a node.
 * Usually constructed from CConman::GetNodeStats. Stats are filled from the
 * node using CNode::copyStats.
 */
struct CNodeStats {
    NodeId nodeid;
    ServiceFlags nServices;
    bool fRelayTxes;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    int64_t nTimeOffset;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    bool m_manual_connection;
    int nStartingHeight;
    uint64_t nSendBytes;
    mapMsgCmdSize mapSendBytesPerMsgCmd;
    uint64_t nRecvBytes;
    mapMsgCmdSize mapRecvBytesPerMsgCmd;
    bool fWhitelisted;
    double dPingTime;
    double dPingWait;
    double dMinPing;
    Amount minFeeFilter;
    // Our address, as reported by the peer
    std::string addrLocal;
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    CAddress addrBind;
};

class CNetMessage {
private:
    mutable CHash256 hasher;
    mutable uint256 data_hash;

public:
    // Parsing header (false) or data (true)
    bool in_data;

    // Partially received header.
    CDataStream hdrbuf;
    // Complete header.
    CMessageHeader hdr;
    uint32_t nHdrPos;

    // Received message data.
    CDataStream vRecv;
    uint32_t nDataPos;

    // Time (in microseconds) of message receipt.
    int64_t nTime;

    CNetMessage(const CMessageHeader::MessageMagic &pchMessageStartIn,
                int nTypeIn, int nVersionIn)
        : hdrbuf(nTypeIn, nVersionIn), hdr(pchMessageStartIn),
          vRecv(nTypeIn, nVersionIn) {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        nTime = 0;
    }

    bool complete() const {
        if (!in_data) {
            return false;
        }

        return (hdr.nMessageSize == nDataPos);
    }

    const uint256 &GetMessageHash() const;

    void SetVersion(int nVersionIn) {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }

    int readHeader(const Config &config, const char *pch, uint32_t nBytes);
    int readData(const char *pch, uint32_t nBytes);
};

/** Information about a peer */
class CNode {
    friend class CConnman;

public:
    // socket
    std::atomic<ServiceFlags> nServices{NODE_NONE};
    SOCKET hSocket GUARDED_BY(cs_hSocket);
    // Total size of all vSendMsg entries.
    size_t nSendSize{0};
    // Offset inside the first vSendMsg already sent.
    size_t nSendOffset{0};
    uint64_t nSendBytes GUARDED_BY(cs_vSend){0};
    std::deque<std::vector<uint8_t>> vSendMsg GUARDED_BY(cs_vSend);
    CCriticalSection cs_vSend;
    CCriticalSection cs_hSocket;
    CCriticalSection cs_vRecv;

    CCriticalSection cs_vProcessMsg;
    std::list<CNetMessage> vProcessMsg GUARDED_BY(cs_vProcessMsg);
    size_t nProcessQueueSize{0};

    CCriticalSection cs_sendProcessing;

    std::deque<CInv> vRecvGetData;
    uint64_t nRecvBytes GUARDED_BY(cs_vRecv){0};
    std::atomic<int> nRecvVersion{INIT_PROTO_VERSION};

    std::atomic<int64_t> nLastSend{0};
    std::atomic<int64_t> nLastRecv{0};
    const int64_t nTimeConnected;
    std::atomic<int64_t> nTimeOffset{0};
    // Address of this peer
    const CAddress addr;
    // Bind address of our side of the connection
    const CAddress addrBind;
    std::atomic<int> nVersion{0};
    // strSubVer is whatever byte array we read from the wire. However, this
    // field is intended to be printed out, displayed to humans in various forms
    // and so on. So we sanitize it and store the sanitized version in
    // cleanSubVer. The original should be used when dealing with the network or
    // wire types and the cleaned string used when displayed or logged.
    std::string strSubVer GUARDED_BY(cs_SubVer), cleanSubVer
        GUARDED_BY(cs_SubVer);
    // Used for both cleanSubVer and strSubVer.
    CCriticalSection cs_SubVer;
    // This peer is preferred for eviction.
    bool m_prefer_evict{false};
    // This peer can bypass DoS banning.
    bool fWhitelisted{false};
    // If true this node is being used as a short lived feeler.
    bool fFeeler{false};
    bool fOneShot{false};
    bool m_manual_connection{false};
    // set by version message
    bool fClient{false};
    // after BIP159, set by version message
    bool m_limited_node{false};
    const bool fInbound;
    std::atomic_bool fSuccessfullyConnected{false};
    std::atomic_bool fDisconnect{false};
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version
    // message.
    // b) the peer may tell us in its version message that we should not relay
    // tx invs unless it loads a bloom filter.
    bool fRelayTxes GUARDED_BY(cs_filter){false};
    bool fSentAddr{false};
    CSemaphoreGrant grantOutbound;
    mutable CCriticalSection cs_filter;
    std::unique_ptr<CBloomFilter> pfilter PT_GUARDED_BY(cs_filter);
    std::atomic<int> nRefCount{0};

    const uint64_t nKeyedNetGroup;
    std::atomic_bool fPauseRecv{false};
    std::atomic_bool fPauseSend{false};

protected:
    mapMsgCmdSize mapSendBytesPerMsgCmd;
    mapMsgCmdSize mapRecvBytesPerMsgCmd GUARDED_BY(cs_vRecv);

public:
    uint256 hashContinue;
    std::atomic<int> nStartingHeight{-1};

    // flood relay
    std::vector<CAddress> vAddrToSend;
    CRollingBloomFilter addrKnown;
    bool fGetAddr{false};
    std::set<uint256> setKnown;
    int64_t nNextAddrSend GUARDED_BY(cs_sendProcessing){0};
    int64_t nNextLocalAddrSend GUARDED_BY(cs_sendProcessing){0};

    // Inventory based relay.
    CRollingBloomFilter filterInventoryKnown GUARDED_BY(cs_inventory);
    // Set of transaction ids we still have to announce. They are sorted by the
    // mempool before relay, so the order is not important.
    std::set<uint256> setInventoryTxToSend;
    // List of block ids we still have announce. There is no final sorting
    // before sending, as they are always sent immediately and in the order
    // requested.
    std::vector<uint256> vInventoryBlockToSend GUARDED_BY(cs_inventory);
    CCriticalSection cs_inventory;
    std::set<uint256> setAskFor;
    std::multimap<int64_t, CInv> mapAskFor;
    int64_t nNextInvSend{0};
    // Used for headers announcements - unfiltered blocks to relay.
    std::vector<uint256> vBlockHashesToAnnounce GUARDED_BY(cs_inventory);
    // Used for BIP35 mempool sending.
    bool fSendMempool GUARDED_BY(cs_inventory){false};

    // Last time a "MEMPOOL" request was serviced.
    std::atomic<int64_t> timeLastMempoolReq{0};

    // Block and TXN accept times
    std::atomic<int64_t> nLastBlockTime{0};
    std::atomic<int64_t> nLastTXTime{0};

    // Ping time measurement:
    // The pong reply we're expecting, or 0 if no pong expected.
    std::atomic<uint64_t> nPingNonceSent{0};
    // Time (in usec) the last ping was sent, or 0 if no ping was ever sent.
    std::atomic<int64_t> nPingUsecStart{0};
    // Last measured round-trip time.
    std::atomic<int64_t> nPingUsecTime{0};
    // Best measured round-trip time.
    std::atomic<int64_t> nMinPingUsecTime{std::numeric_limits<int64_t>::max()};
    // Whether a ping is requested.
    std::atomic<bool> fPingQueued{false};
    // Minimum fee rate with which to filter inv's to this node
    Amount minFeeFilter GUARDED_BY(cs_feeFilter){Amount::zero()};
    CCriticalSection cs_feeFilter;
    Amount lastSentFeeFilter{Amount::zero()};
    int64_t nextSendTimeFeeFilter{0};

    CNode(NodeId id, ServiceFlags nLocalServicesIn, int nMyStartingHeightIn,
          SOCKET hSocketIn, const CAddress &addrIn, uint64_t nKeyedNetGroupIn,
          uint64_t nLocalHostNonceIn, const CAddress &addrBindIn,
          const std::string &addrNameIn = "", bool fInboundIn = false);
    ~CNode();
    CNode(const CNode &) = delete;
    CNode &operator=(const CNode &) = delete;

private:
    const NodeId id;
    const uint64_t nLocalHostNonce;
    // Services offered to this peer
    const ServiceFlags nLocalServices;
    const int nMyStartingHeight;
    int nSendVersion{0};
    // Used only by SocketHandler thread.
    std::list<CNetMessage> vRecvMsg;

    mutable CCriticalSection cs_addrName;
    std::string addrName GUARDED_BY(cs_addrName);

    // Our address, as reported by the peer
    CService addrLocal GUARDED_BY(cs_addrLocal);
    mutable CCriticalSection cs_addrLocal;

public:
    NodeId GetId() const { return id; }

    uint64_t GetLocalNonce() const { return nLocalHostNonce; }

    int GetMyStartingHeight() const { return nMyStartingHeight; }

    int GetRefCount() const {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    bool ReceiveMsgBytes(const Config &config, const char *pch, uint32_t nBytes,
                         bool &complete);

    void SetRecvVersion(int nVersionIn) { nRecvVersion = nVersionIn; }
    int GetRecvVersion() const { return nRecvVersion; }
    void SetSendVersion(int nVersionIn);
    int GetSendVersion() const;

    CService GetAddrLocal() const;
    //! May not be called more than once
    void SetAddrLocal(const CService &addrLocalIn);

    CNode *AddRef() {
        nRefCount++;
        return this;
    }

    void Release() { nRefCount--; }

    void AddAddressKnown(const CAddress &_addr) {
        addrKnown.insert(_addr.GetKey());
    }

    void PushAddress(const CAddress &_addr, FastRandomContext &insecure_rand) {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (_addr.IsValid() && !addrKnown.contains(_addr.GetKey())) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand.randrange(vAddrToSend.size())] =
                    _addr;
            } else {
                vAddrToSend.push_back(_addr);
            }
        }
    }

    void AddInventoryKnown(const CInv &inv) {
        LOCK(cs_inventory);
        filterInventoryKnown.insert(inv.hash);
    }

    void PushInventory(const CInv &inv) {
        LOCK(cs_inventory);
        if (inv.type == MSG_TX) {
            if (!filterInventoryKnown.contains(inv.hash)) {
                setInventoryTxToSend.insert(inv.hash);
            }
        } else if (inv.type == MSG_BLOCK) {
            vInventoryBlockToSend.push_back(inv.hash);
        }
    }

    void PushBlockHash(const uint256 &hash) {
        LOCK(cs_inventory);
        vBlockHashesToAnnounce.push_back(hash);
    }

    void AskFor(const CInv &inv);

    void CloseSocketDisconnect();

    void copyStats(CNodeStats &stats);

    ServiceFlags GetLocalServices() const { return nLocalServices; }

    std::string GetAddrName() const;
    //! Sets the addrName only if it was not previously set
    void MaybeSetAddrName(const std::string &addrNameIn);
};

/**
 * Return a timestamp in the future (in microseconds) for exponentially
 * distributed events.
 */
int64_t PoissonNextSend(int64_t nNow, int average_interval_seconds);

std::string getSubVersionEB(uint64_t MaxBlockSize);
std::string userAgent(const Config &config);
#endif // BITCOIN_NET_H
