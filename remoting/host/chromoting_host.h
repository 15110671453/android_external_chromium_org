// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CHROMOTING_HOST_H_
#define REMOTING_CHROMOTING_HOST_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/threading/thread.h"
#include "base/timer.h"
#include "remoting/base/encoder.h"
#include "remoting/host/access_verifier.h"
#include "remoting/host/capturer.h"
#include "remoting/host/client_session.h"
#include "remoting/host/desktop_environment.h"
#include "remoting/host/host_status_observer.h"
#include "remoting/jingle_glue/jingle_client.h"
#include "remoting/jingle_glue/jingle_thread.h"
#include "remoting/protocol/session_manager.h"
#include "remoting/protocol/connection_to_client.h"

class Task;

namespace remoting {

namespace protocol {
class ConnectionToClient;
class HostStub;
class InputStub;
class SessionConfig;
class CandidateSessionConfig;
}  // namespace protocol

class Capturer;
class ChromotingHostContext;
class DesktopEnvironment;
class Encoder;
class MutableHostConfig;
class ScreenRecorder;

// A class to implement the functionality of a host process.
//
// Here's the work flow of this class:
// 1. We should load the saved GAIA ID token or if this is the first
//    time the host process runs we should prompt user for the
//    credential. We will use this token or credentials to authenicate
//    and register the host.
//
// 2. We listen for incoming connection using libjingle. We will create
//    a ConnectionToClient object that wraps around linjingle for transport.
//    A ScreenRecorder is created with an Encoder and a Capturer.
//    A ConnectionToClient is added to the ScreenRecorder for transporting
//    the screen captures. An InputStub is created and registered with the
//    ConnectionToClient to receive mouse / keyboard events from the remote
//    client.
//    After we have done all the initialization we'll start the ScreenRecorder.
//    We'll then enter the running state of the host process.
//
// 3. When the user is disconnected, we will pause the ScreenRecorder
//    and try to terminate the threads we have created. This will allow
//    all pending tasks to complete. After all of that completed we
//    return to the idle state. We then go to step (2) if there a new
//    incoming connection.
class ChromotingHost : public base::RefCountedThreadSafe<ChromotingHost>,
                       public protocol::ConnectionToClient::EventHandler,
                       public ClientSession::EventHandler,
                       public JingleClient::Callback {
 public:
  // Factory methods that must be used to create ChromotingHost instances.
  // Default capturer and input stub are used if it is not specified.
  // Returned instance takes ownership of |access_verifier| and |environment|,
  // and adds a reference to |config|. It does NOT take ownership of |context|.
  static ChromotingHost* Create(ChromotingHostContext* context,
                                MutableHostConfig* config,
                                AccessVerifier* access_verifier);
  static ChromotingHost* Create(ChromotingHostContext* context,
                                MutableHostConfig* config,
                                DesktopEnvironment* environment,
                                AccessVerifier* access_verifier);

  // Asynchronously start the host process.
  //
  // After this is invoked, the host process will connect to the talk
  // network and start listening for incoming connections.
  //
  // This method can only be called once during the lifetime of this object.
  void Start();

  // Asynchronously shutdown the host process. |shutdown_task| is
  // called after shutdown is completed.
  void Shutdown(Task* shutdown_task);

  // Adds |observer| to the list of status observers. Doesn't take
  // ownership of |observer|, so |observer| must outlive this
  // object. All status observers must be added before the host is
  // started.
  void AddStatusObserver(HostStatusObserver* observer);

  ////////////////////////////////////////////////////////////////////////////
  // protocol::ConnectionToClient::EventHandler implementations
  virtual void OnConnectionOpened(protocol::ConnectionToClient* client);
  virtual void OnConnectionClosed(protocol::ConnectionToClient* client);
  virtual void OnConnectionFailed(protocol::ConnectionToClient* client);
  virtual void OnSequenceNumberUpdated(protocol::ConnectionToClient* client,
                                       int64 sequence_number);

  ////////////////////////////////////////////////////////////////////////////
  // JingleClient::Callback implementations
  virtual void OnStateChange(JingleClient* client, JingleClient::State state);

  ////////////////////////////////////////////////////////////////////////////
  // ClientSession::EventHandler implementations
  virtual void LocalLoginSucceeded(
      scoped_refptr<protocol::ConnectionToClient> client);
  virtual void LocalLoginFailed(
      scoped_refptr<protocol::ConnectionToClient> client);

  // Callback for ChromotingServer.
  void OnNewClientSession(
      protocol::Session* session,
      protocol::SessionManager::IncomingSessionResponse* response);

  // Sets desired configuration for the protocol. Ownership of the
  // |config| is transferred to the object. Must be called before Start().
  void set_protocol_config(protocol::CandidateSessionConfig* config);

  // TODO(wez): ChromotingHost shouldn't need to know about Me2Mom.
  void set_it2me(bool is_it2me) {
    is_it2me_ = is_it2me;
  }

  // Notify all active client sessions that local input has been detected, and
  // that remote input should be ignored for a short time.
  void LocalMouseMoved(const gfx::Point& new_pos);

  // Pause or unpause the session. While the session is paused, remote input
  // is ignored.
  void PauseSession(bool pause);

 private:
  friend class base::RefCountedThreadSafe<ChromotingHost>;
  friend class ChromotingHostTest;

  typedef std::vector<HostStatusObserver*> StatusObserverList;
  typedef std::vector<scoped_refptr<ClientSession> > ClientList;

  // Takes ownership of |access_verifier| and |environment|, and adds a
  // reference to |config|. Does NOT take ownership of |context|.
  ChromotingHost(ChromotingHostContext* context,
                 MutableHostConfig* config,
                 DesktopEnvironment* environment,
                 AccessVerifier* access_verifier);
  virtual ~ChromotingHost();

  enum State {
    kInitial,
    kStarted,
    kStopping,
    kStopped,
  };

  // This method is called if a client is disconnected from the host.
  void OnClientDisconnected(protocol::ConnectionToClient* client);

  // Creates encoder for the specified configuration.
  Encoder* CreateEncoder(const protocol::SessionConfig* config);

  std::string GenerateHostAuthToken(const std::string& encoded_client_token);

  bool HasAuthenticatedClients() const;

  void EnableCurtainMode(bool enable);

  void MonitorLocalInputs(bool enable);

  void ProcessPreAuthentication(
      const scoped_refptr<protocol::ConnectionToClient>& connection);

  // Show or hide the Disconnect window on the UI thread.  If |show| is false,
  // hide the window, ignoring the |username| parameter.
  void ShowDisconnectWindow(bool show, const std::string& username);

  // Show or hide the Continue Sharing window on the UI thread.
  void ShowContinueWindow(bool show);

  void StartContinueWindowTimer(bool start);

  void ContinueWindowTimerFunc();

  // The following methods are called during shutdown.
  void ShutdownJingleClient();
  void ShutdownSignallingDisconnected();
  void ShutdownRecorder();
  void ShutdownFinish();

  // The context that the chromoting host runs on.
  ChromotingHostContext* context_;

  scoped_refptr<MutableHostConfig> config_;

  scoped_ptr<DesktopEnvironment> desktop_environment_;

  scoped_ptr<SignalStrategy> signal_strategy_;

  // The libjingle client. This is used to connect to the talk network to
  // receive connection requests from chromoting client.
  scoped_refptr<JingleClient> jingle_client_;

  scoped_refptr<protocol::SessionManager> session_manager_;

  StatusObserverList status_observers_;

  scoped_ptr<AccessVerifier> access_verifier_;

  // The connections to remote clients.
  ClientList clients_;

  // Session manager for the host process.
  scoped_refptr<ScreenRecorder> recorder_;

  // Tracks the internal state of the host.
  // This variable is written on the main thread of ChromotingHostContext
  // and read by jingle thread.
  State state_;

  // Lock is to lock the access to |state_|.
  base::Lock lock_;

  // Configuration of the protocol.
  scoped_ptr<protocol::CandidateSessionConfig> protocol_config_;

  bool is_curtained_;
  bool is_monitoring_local_inputs_;

  // Timer controlling the "continue session" dialog. The timer is started when
  // a connection is made or re-confirmed. On expiry, inputs to the host are
  // blocked and the dialog is shown.
  base::OneShotTimer<ChromotingHost> continue_window_timer_;

  // Whether or not the host is running in "IT2Me" mode, in which connections
  // are pre-authenticated, and hence the local login challenge can be bypassed.
  bool is_it2me_;

  // Stores list of tasks that should be executed when we finish
  // shutdown. Used only while |state_| is set to kStopping.
  std::vector<Task*> shutdown_tasks_;

  DISALLOW_COPY_AND_ASSIGN(ChromotingHost);
};

}  // namespace remoting

#endif  // REMOTING_HOST_CHROMOTING_HOST_H_
