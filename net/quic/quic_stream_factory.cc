// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_stream_factory.h"

#include <set>

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/metrics/histogram.h"
#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verifier.h"
#include "net/dns/host_resolver.h"
#include "net/dns/single_request_host_resolver.h"
#include "net/http/http_server_properties.h"
#include "net/quic/congestion_control/tcp_receiver.h"
#include "net/quic/crypto/proof_verifier_chromium.h"
#include "net/quic/crypto/quic_random.h"
#include "net/quic/port_suggester.h"
#include "net/quic/quic_client_session.h"
#include "net/quic/quic_clock.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_connection_helper.h"
#include "net/quic/quic_crypto_client_stream_factory.h"
#include "net/quic/quic_default_packet_writer.h"
#include "net/quic/quic_http_stream.h"
#include "net/quic/quic_protocol.h"
#include "net/socket/client_socket_factory.h"

using std::string;
using std::vector;

namespace net {

// Responsible for creating a new QUIC session to the specified server, and
// for notifying any associated requests when complete.
class QuicStreamFactory::Job {
 public:
  Job(QuicStreamFactory* factory,
      HostResolver* host_resolver,
      const HostPortProxyPair& host_port_proxy_pair,
      bool is_https,
      CertVerifier* cert_verifier,
      const BoundNetLog& net_log);

  ~Job();

  int Run(const CompletionCallback& callback);

  int DoLoop(int rv);
  int DoResolveHost();
  int DoResolveHostComplete(int rv);
  int DoConnect();
  int DoConnectComplete(int rv);

  void OnIOComplete(int rv);

  CompletionCallback callback() {
    return callback_;
  }

  const HostPortProxyPair& host_port_proxy_pair() const {
    return host_port_proxy_pair_;
  }

 private:
  enum IoState {
    STATE_NONE,
    STATE_RESOLVE_HOST,
    STATE_RESOLVE_HOST_COMPLETE,
    STATE_CONNECT,
    STATE_CONNECT_COMPLETE,
  };
  IoState io_state_;

  QuicStreamFactory* factory_;
  SingleRequestHostResolver host_resolver_;
  const HostPortProxyPair host_port_proxy_pair_;
  bool is_https_;
  CertVerifier* cert_verifier_;
  const BoundNetLog net_log_;
  QuicClientSession* session_;
  CompletionCallback callback_;
  AddressList address_list_;
  DISALLOW_COPY_AND_ASSIGN(Job);
};

QuicStreamFactory::Job::Job(
    QuicStreamFactory* factory,
    HostResolver* host_resolver,
    const HostPortProxyPair& host_port_proxy_pair,
    bool is_https,
    CertVerifier* cert_verifier,
    const BoundNetLog& net_log)
    : factory_(factory),
      host_resolver_(host_resolver),
      host_port_proxy_pair_(host_port_proxy_pair),
      is_https_(is_https),
      cert_verifier_(cert_verifier),
      net_log_(net_log),
      session_(NULL) {
}

QuicStreamFactory::Job::~Job() {
}

int QuicStreamFactory::Job::Run(const CompletionCallback& callback) {
  io_state_ = STATE_RESOLVE_HOST;
  int rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING)
    callback_ = callback;

  return rv > 0 ? OK : rv;
}

int QuicStreamFactory::Job::DoLoop(int rv) {
  do {
    IoState state = io_state_;
    io_state_ = STATE_NONE;
    switch (state) {
      case STATE_RESOLVE_HOST:
        CHECK_EQ(OK, rv);
        rv = DoResolveHost();
        break;
      case STATE_RESOLVE_HOST_COMPLETE:
        rv = DoResolveHostComplete(rv);
        break;
      case STATE_CONNECT:
        CHECK_EQ(OK, rv);
        rv = DoConnect();
        break;
      case STATE_CONNECT_COMPLETE:
        rv = DoConnectComplete(rv);
        break;
      default:
        NOTREACHED() << "io_state_: " << io_state_;
        break;
    }
  } while (io_state_ != STATE_NONE && rv != ERR_IO_PENDING);
  return rv;
}

void QuicStreamFactory::Job::OnIOComplete(int rv) {
  rv = DoLoop(rv);

  if (rv != ERR_IO_PENDING && !callback_.is_null()) {
    callback_.Run(rv);
  }
}

int QuicStreamFactory::Job::DoResolveHost() {
  io_state_ = STATE_RESOLVE_HOST_COMPLETE;
  return host_resolver_.Resolve(
      HostResolver::RequestInfo(host_port_proxy_pair_.first),
      DEFAULT_PRIORITY,
      &address_list_,
      base::Bind(&QuicStreamFactory::Job::OnIOComplete, base::Unretained(this)),
      net_log_);
}

int QuicStreamFactory::Job::DoResolveHostComplete(int rv) {
  if (rv != OK)
    return rv;

  DCHECK(!factory_->HasActiveSession(host_port_proxy_pair_));

  // Inform the factory of this resolution, which will set up
  // a session alias, if possible.
  if (factory_->OnResolution(host_port_proxy_pair_, address_list_)) {
    return OK;
  }

  io_state_ = STATE_CONNECT;
  return OK;
}

QuicStreamRequest::QuicStreamRequest(QuicStreamFactory* factory)
    : factory_(factory) {}

QuicStreamRequest::~QuicStreamRequest() {
  if (factory_ && !callback_.is_null())
    factory_->CancelRequest(this);
}

int QuicStreamRequest::Request(
    const HostPortProxyPair& host_port_proxy_pair,
    bool is_https,
    CertVerifier* cert_verifier,
    const BoundNetLog& net_log,
    const CompletionCallback& callback) {
  DCHECK(!stream_);
  DCHECK(callback_.is_null());
  DCHECK(factory_);
  int rv = factory_->Create(host_port_proxy_pair, is_https, cert_verifier,
                            net_log, this);
  if (rv == ERR_IO_PENDING) {
    host_port_proxy_pair_ = host_port_proxy_pair;
    is_https_ = is_https;
    cert_verifier_ = cert_verifier;
    net_log_ = net_log;
    callback_ = callback;
  } else {
    factory_ = NULL;
  }
  if (rv == OK)
    DCHECK(stream_);
  return rv;
}

void QuicStreamRequest::set_stream(scoped_ptr<QuicHttpStream> stream) {
  DCHECK(stream);
  stream_ = stream.Pass();
}

void QuicStreamRequest::OnRequestComplete(int rv) {
  factory_ = NULL;
  callback_.Run(rv);
}

scoped_ptr<QuicHttpStream> QuicStreamRequest::ReleaseStream() {
  DCHECK(stream_);
  return stream_.Pass();
}

int QuicStreamFactory::Job::DoConnect() {
  io_state_ = STATE_CONNECT_COMPLETE;

  int rv = factory_->CreateSession(host_port_proxy_pair_, is_https_,
      cert_verifier_, address_list_, net_log_, &session_);
  if (rv != OK) {
    DCHECK(rv != ERR_IO_PENDING);
    DCHECK(!session_);
    return rv;
  }

  session_->StartReading();
  if (!session_->connection()->connected()) {
    return ERR_QUIC_PROTOCOL_ERROR;
  }
  rv = session_->CryptoConnect(
      factory_->require_confirmation() || is_https_,
      base::Bind(&QuicStreamFactory::Job::OnIOComplete,
                 base::Unretained(this)));
  return rv;
}

int QuicStreamFactory::Job::DoConnectComplete(int rv) {
  if (rv != OK)
    return rv;

  DCHECK(!factory_->HasActiveSession(host_port_proxy_pair_));
  // There may well now be an active session for this IP.  If so, use the
  // existing session instead.
  AddressList address(session_->connection()->peer_address());
  if (factory_->OnResolution(host_port_proxy_pair_, address)) {
    session_->connection()->SendConnectionClose(QUIC_NO_ERROR);
    session_ = NULL;
    return OK;
  }

  factory_->ActivateSession(host_port_proxy_pair_, session_);

  return OK;
}

QuicStreamFactory::QuicStreamFactory(
    HostResolver* host_resolver,
    ClientSocketFactory* client_socket_factory,
    base::WeakPtr<HttpServerProperties> http_server_properties,
    QuicCryptoClientStreamFactory* quic_crypto_client_stream_factory,
    QuicRandom* random_generator,
    QuicClock* clock,
    size_t max_packet_length,
    const QuicVersionVector& supported_versions)
    : require_confirmation_(true),
      host_resolver_(host_resolver),
      client_socket_factory_(client_socket_factory),
      http_server_properties_(http_server_properties),
      quic_crypto_client_stream_factory_(quic_crypto_client_stream_factory),
      random_generator_(random_generator),
      clock_(clock),
      max_packet_length_(max_packet_length),
      supported_versions_(supported_versions),
      port_seed_(random_generator_->RandUint64()),
      weak_factory_(this) {
  config_.SetDefaults();
  config_.set_idle_connection_state_lifetime(
      QuicTime::Delta::FromSeconds(30),
      QuicTime::Delta::FromSeconds(30));

  cannoncial_suffixes_.push_back(string(".c.youtube.com"));
  cannoncial_suffixes_.push_back(string(".googlevideo.com"));
}

QuicStreamFactory::~QuicStreamFactory() {
  CloseAllSessions(ERR_ABORTED);
  STLDeleteElements(&all_sessions_);
  STLDeleteValues(&active_jobs_);
  STLDeleteValues(&all_crypto_configs_);
}

int QuicStreamFactory::Create(const HostPortProxyPair& host_port_proxy_pair,
                              bool is_https,
                              CertVerifier* cert_verifier,
                              const BoundNetLog& net_log,
                              QuicStreamRequest* request) {
  if (HasActiveSession(host_port_proxy_pair)) {
    request->set_stream(CreateIfSessionExists(host_port_proxy_pair, net_log));
    return OK;
  }

  if (HasActiveJob(host_port_proxy_pair)) {
    Job* job = active_jobs_[host_port_proxy_pair];
    active_requests_[request] = job;
    job_requests_map_[job].insert(request);
    return ERR_IO_PENDING;
  }

  scoped_ptr<Job> job(new Job(this, host_resolver_, host_port_proxy_pair,
                              is_https, cert_verifier, net_log));
  int rv = job->Run(base::Bind(&QuicStreamFactory::OnJobComplete,
                               base::Unretained(this), job.get()));

  if (rv == ERR_IO_PENDING) {
    active_requests_[request] = job.get();
    job_requests_map_[job.get()].insert(request);
    active_jobs_[host_port_proxy_pair] = job.release();
  }
  if (rv == OK) {
    DCHECK(HasActiveSession(host_port_proxy_pair));
    request->set_stream(CreateIfSessionExists(host_port_proxy_pair, net_log));
  }
  return rv;
}

bool QuicStreamFactory::OnResolution(
    const HostPortProxyPair& host_port_proxy_pair,
    const AddressList& address_list) {
  DCHECK(!HasActiveSession(host_port_proxy_pair));
  for (size_t i = 0; i < address_list.size(); ++i) {
    const IPEndPoint& address = address_list[i];
    if (!ContainsKey(ip_aliases_, address))
      continue;

    const SessionSet& sessions = ip_aliases_[address];
    for (SessionSet::const_iterator i = sessions.begin();
         i != sessions.end(); ++i) {
      QuicClientSession* session = *i;
      if (!session->CanPool(host_port_proxy_pair.first.host()))
        continue;
      active_sessions_[host_port_proxy_pair] = session;
      session_aliases_[session].insert(host_port_proxy_pair);
      return true;
    }
  }
  return false;
}

void QuicStreamFactory::OnJobComplete(Job* job, int rv) {
  if (rv == OK) {
    require_confirmation_ = false;

    // Create all the streams, but do not notify them yet.
    for (RequestSet::iterator it = job_requests_map_[job].begin();
         it != job_requests_map_[job].end() ; ++it) {
      DCHECK(HasActiveSession(job->host_port_proxy_pair()));
      (*it)->set_stream(CreateIfSessionExists(job->host_port_proxy_pair(),
                                              (*it)->net_log()));
    }
  }
  while (!job_requests_map_[job].empty()) {
    RequestSet::iterator it = job_requests_map_[job].begin();
    QuicStreamRequest* request = *it;
    job_requests_map_[job].erase(it);
    active_requests_.erase(request);
    // Even though we're invoking callbacks here, we don't need to worry
    // about |this| being deleted, because the factory is owned by the
    // profile which can not be deleted via callbacks.
    request->OnRequestComplete(rv);
  }
  active_jobs_.erase(job->host_port_proxy_pair());
  job_requests_map_.erase(job);
  delete job;
  return;
}

// Returns a newly created QuicHttpStream owned by the caller, if a
// matching session already exists.  Returns NULL otherwise.
scoped_ptr<QuicHttpStream> QuicStreamFactory::CreateIfSessionExists(
    const HostPortProxyPair& host_port_proxy_pair,
    const BoundNetLog& net_log) {
  if (!HasActiveSession(host_port_proxy_pair)) {
    DVLOG(1) << "No active session";
    return scoped_ptr<QuicHttpStream>();
  }

  QuicClientSession* session = active_sessions_[host_port_proxy_pair];
  DCHECK(session);
  return scoped_ptr<QuicHttpStream>(
      new QuicHttpStream(session->GetWeakPtr()));
}

void QuicStreamFactory::OnIdleSession(QuicClientSession* session) {
}

void QuicStreamFactory::OnSessionGoingAway(QuicClientSession* session) {
  const AliasSet& aliases = session_aliases_[session];
  for (AliasSet::const_iterator it = aliases.begin(); it != aliases.end();
       ++it) {
    DCHECK(active_sessions_.count(*it));
    DCHECK_EQ(session, active_sessions_[*it]);
    active_sessions_.erase(*it);
    if (!session->IsCryptoHandshakeConfirmed() && http_server_properties_) {
      // TODO(rch):  In the special case where the session has received no
      // packets from the peer, we should consider blacklisting this
      // differently so that we still race TCP but we don't consider the
      // session connected until the handshake has been confirmed.
      http_server_properties_->SetBrokenAlternateProtocol(it->first);
    }
  }
  IPEndPoint peer_address = session->connection()->peer_address();
  ip_aliases_[peer_address].erase(session);
  if (ip_aliases_[peer_address].empty()) {
    ip_aliases_.erase(peer_address);
  }
  session_aliases_.erase(session);
}

void QuicStreamFactory::OnSessionClosed(QuicClientSession* session) {
  DCHECK_EQ(0u, session->GetNumOpenStreams());
  OnSessionGoingAway(session);
  all_sessions_.erase(session);
  delete session;
}

void QuicStreamFactory::CancelRequest(QuicStreamRequest* request) {
  DCHECK(ContainsKey(active_requests_, request));
  Job* job = active_requests_[request];
  job_requests_map_[job].erase(request);
  active_requests_.erase(request);
}

void QuicStreamFactory::CloseAllSessions(int error) {
  while (!active_sessions_.empty()) {
    size_t initial_size = active_sessions_.size();
    active_sessions_.begin()->second->CloseSessionOnError(error);
    DCHECK_NE(initial_size, active_sessions_.size());
  }
  while (!all_sessions_.empty()) {
    size_t initial_size = all_sessions_.size();
    (*all_sessions_.begin())->CloseSessionOnError(error);
    DCHECK_NE(initial_size, all_sessions_.size());
  }
  DCHECK(all_sessions_.empty());
}

base::Value* QuicStreamFactory::QuicStreamFactoryInfoToValue() const {
  base::ListValue* list = new base::ListValue();

  for (SessionMap::const_iterator it = active_sessions_.begin();
       it != active_sessions_.end(); ++it) {
    const HostPortProxyPair& pair = it->first;
    QuicClientSession* session = it->second;
    const AliasSet& aliases = session_aliases_.find(session)->second;
    if (pair.first.Equals(aliases.begin()->first) &&
        pair.second == aliases.begin()->second) {
      list->Append(session->GetInfoAsValue(aliases));
    }
  }
  return list;
}

void QuicStreamFactory::OnIPAddressChanged() {
  CloseAllSessions(ERR_NETWORK_CHANGED);
  require_confirmation_ = true;
}

void QuicStreamFactory::OnCertAdded(const X509Certificate* cert) {
  CloseAllSessions(ERR_CERT_DATABASE_CHANGED);
}

void QuicStreamFactory::OnCACertChanged(const X509Certificate* cert) {
  // We should flush the sessions if we removed trust from a
  // cert, because a previously trusted server may have become
  // untrusted.
  //
  // We should not flush the sessions if we added trust to a cert.
  //
  // Since the OnCACertChanged method doesn't tell us what
  // kind of change it is, we have to flush the socket
  // pools to be safe.
  CloseAllSessions(ERR_CERT_DATABASE_CHANGED);
}

bool QuicStreamFactory::HasActiveSession(
    const HostPortProxyPair& host_port_proxy_pair) {
  return ContainsKey(active_sessions_, host_port_proxy_pair);
}

int QuicStreamFactory::CreateSession(
    const HostPortProxyPair& host_port_proxy_pair,
    bool is_https,
    CertVerifier* cert_verifier,
    const AddressList& address_list,
    const BoundNetLog& net_log,
    QuicClientSession** session) {
  QuicGuid guid = random_generator_->RandUint64();
  IPEndPoint addr = *address_list.begin();
  scoped_refptr<PortSuggester> port_suggester =
      new PortSuggester(host_port_proxy_pair.first, port_seed_);
  DatagramSocket::BindType bind_type = DatagramSocket::RANDOM_BIND;
#if defined(OS_WIN)
  // TODO(jar)bug=329255 Provide better implementation to avoid pop-up warning.
  bind_type = DatagramSocket::DEFAULT_BIND;
#endif
  scoped_ptr<DatagramClientSocket> socket(
      client_socket_factory_->CreateDatagramClientSocket(
          bind_type,
          base::Bind(&PortSuggester::SuggestPort, port_suggester),
          net_log.net_log(), net_log.source()));
  int rv = socket->Connect(addr);
  if (rv != OK)
    return rv;
  UMA_HISTOGRAM_COUNTS("Net.QuicEphemeralPortsSuggested",
                       port_suggester->call_count());
#if defined(OS_WIN)
  // TODO(jar)bug=329255 Provide better implementation to avoid pop-up warning.
  DCHECK_EQ(0u, port_suggester->call_count());
#else
  DCHECK_LE(1u, port_suggester->call_count());
#endif

  // We should adaptively set this buffer size, but for now, we'll use a size
  // that is more than large enough for a full receive window, and yet
  // does not consume "too much" memory.  If we see bursty packet loss, we may
  // revisit this setting and test for its impact.
  const int32 kSocketBufferSize(TcpReceiver::kReceiveWindowTCP);
  socket->SetReceiveBufferSize(kSocketBufferSize);
  // Set a buffer large enough to contain the initial CWND's worth of packet
  // to work around the problem with CHLO packets being sent out with the
  // wrong encryption level, when the send buffer is full.
  socket->SetSendBufferSize(kMaxPacketSize * 20); // Support 20 packets.

  scoped_ptr<QuicDefaultPacketWriter> writer(
      new QuicDefaultPacketWriter(socket.get()));

  if (!helper_.get()) {
    helper_.reset(new QuicConnectionHelper(
        base::MessageLoop::current()->message_loop_proxy().get(),
        clock_.get(), random_generator_));
  }

  QuicConnection* connection = new QuicConnection(guid, addr, helper_.get(),
                                                  writer.get(), false,
                                                  supported_versions_);
  writer->SetConnection(connection);
  connection->options()->max_packet_length = max_packet_length_;

  QuicCryptoClientConfig* crypto_config =
      GetOrCreateCryptoConfig(host_port_proxy_pair);
  DCHECK(crypto_config);

  *session = new QuicClientSession(
      connection, socket.Pass(), writer.Pass(), this,
      quic_crypto_client_stream_factory_, host_port_proxy_pair.first.host(),
      config_, crypto_config, net_log.net_log());
  all_sessions_.insert(*session);  // owning pointer
  if (is_https) {
    crypto_config->SetProofVerifier(
        new ProofVerifierChromium(cert_verifier, net_log));
  }
  return OK;
}

bool QuicStreamFactory::HasActiveJob(
    const HostPortProxyPair& host_port_proxy_pair) {
  return ContainsKey(active_jobs_, host_port_proxy_pair);
}

void QuicStreamFactory::ActivateSession(
    const HostPortProxyPair& host_port_proxy_pair,
    QuicClientSession* session) {
  DCHECK(!HasActiveSession(host_port_proxy_pair));
  active_sessions_[host_port_proxy_pair] = session;
  session_aliases_[session].insert(host_port_proxy_pair);
  DCHECK(!ContainsKey(ip_aliases_[session->connection()->peer_address()],
                      session));
  ip_aliases_[session->connection()->peer_address()].insert(session);
}

QuicCryptoClientConfig* QuicStreamFactory::GetOrCreateCryptoConfig(
    const HostPortProxyPair& host_port_proxy_pair) {
  QuicCryptoClientConfig* crypto_config;

  if (ContainsKey(all_crypto_configs_, host_port_proxy_pair)) {
    crypto_config = all_crypto_configs_[host_port_proxy_pair];
    DCHECK(crypto_config);
  } else {
    // TODO(rtenneti): if two quic_sessions for the same host_port_proxy_pair
    // share the same crypto_config, will it cause issues?
    crypto_config = new QuicCryptoClientConfig();
    crypto_config->SetDefaults();
    all_crypto_configs_[host_port_proxy_pair] = crypto_config;
    PopulateFromCanonicalConfig(host_port_proxy_pair, crypto_config);
  }
  return crypto_config;
}

void QuicStreamFactory::PopulateFromCanonicalConfig(
    const HostPortProxyPair& host_port_proxy_pair,
    QuicCryptoClientConfig* crypto_config) {
  const string server_hostname = host_port_proxy_pair.first.host();

  unsigned i = 0;
  for (; i < cannoncial_suffixes_.size(); ++i) {
    if (EndsWith(server_hostname, cannoncial_suffixes_[i], false)) {
      break;
    }
  }
  if (i == cannoncial_suffixes_.size())
    return;

  HostPortPair canonical_host_port(cannoncial_suffixes_[i],
                                   host_port_proxy_pair.first.port());
  if (!ContainsKey(canonical_hostname_to_origin_map_, canonical_host_port)) {
    // This is the first host we've seen which matches the suffix, so make it
    // canonical.
    canonical_hostname_to_origin_map_[canonical_host_port] =
        host_port_proxy_pair;
    return;
  }

  const HostPortProxyPair& canonical_host_port_proxy_pair =
      canonical_hostname_to_origin_map_[canonical_host_port];
  QuicCryptoClientConfig* canonical_crypto_config =
      all_crypto_configs_[canonical_host_port_proxy_pair];
  DCHECK(canonical_crypto_config);

  // Copy the CachedState for the canonical server from canonical_crypto_config
  // as the initial CachedState for the server_hostname in crypto_config.
  crypto_config->InitializeFrom(server_hostname,
                                canonical_host_port_proxy_pair.first.host(),
                                canonical_crypto_config);
  // Update canonical version to point at the "most recent" crypto_config.
  canonical_hostname_to_origin_map_[canonical_host_port] = host_port_proxy_pair;
}

}  // namespace net
