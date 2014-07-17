// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Audio rendering unit utilizing an AudioRendererSink to output data.
//
// This class lives inside three threads during it's lifetime, namely:
// 1. Render thread
//    Where the object is created.
// 2. Media thread (provided via constructor)
//    All AudioDecoder methods are called on this thread.
// 3. Audio thread created by the AudioRendererSink.
//    Render() is called here where audio data is decoded into raw PCM data.
//
// AudioRendererImpl talks to an AudioRendererAlgorithm that takes care of
// queueing audio data and stretching/shrinking audio data when playback rate !=
// 1.0 or 0.0.

#ifndef MEDIA_FILTERS_AUDIO_RENDERER_IMPL_H_
#define MEDIA_FILTERS_AUDIO_RENDERER_IMPL_H_

#include <deque>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "media/base/audio_decoder.h"
#include "media/base/audio_renderer.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/decryptor.h"
#include "media/filters/audio_renderer_algorithm.h"
#include "media/filters/decoder_stream.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class AudioBufferConverter;
class AudioBus;
class AudioClock;
class AudioHardwareConfig;
class AudioSplicer;
class DecryptingDemuxerStream;

class MEDIA_EXPORT AudioRendererImpl
    : public AudioRenderer,
      NON_EXPORTED_BASE(public AudioRendererSink::RenderCallback) {
 public:
  // |task_runner| is the thread on which AudioRendererImpl will execute.
  //
  // |sink| is used as the destination for the rendered audio.
  //
  // |decoders| contains the AudioDecoders to use when initializing.
  //
  // |set_decryptor_ready_cb| is fired when the audio decryptor is available
  // (only applicable if the stream is encrypted and we have a decryptor).
  AudioRendererImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      AudioRendererSink* sink,
      ScopedVector<AudioDecoder> decoders,
      const SetDecryptorReadyCB& set_decryptor_ready_cb,
      AudioHardwareConfig* hardware_params);
  virtual ~AudioRendererImpl();

  // AudioRenderer implementation.
  virtual void Initialize(DemuxerStream* stream,
                          const PipelineStatusCB& init_cb,
                          const StatisticsCB& statistics_cb,
                          const TimeCB& time_cb,
                          const BufferingStateCB& buffering_state_cb,
                          const base::Closure& ended_cb,
                          const PipelineStatusCB& error_cb) OVERRIDE;
  virtual void StartRendering() OVERRIDE;
  virtual void StopRendering() OVERRIDE;
  virtual void Flush(const base::Closure& callback) OVERRIDE;
  virtual void Stop(const base::Closure& callback) OVERRIDE;
  virtual void SetPlaybackRate(float rate) OVERRIDE;
  virtual void StartPlayingFrom(base::TimeDelta timestamp) OVERRIDE;
  virtual void SetVolume(float volume) OVERRIDE;

 private:
  friend class AudioRendererImplTest;

  // Important detail: being in kPlaying doesn't imply that audio is being
  // rendered. Rather, it means that the renderer is ready to go. The actual
  // rendering of audio is controlled via Start/StopRendering().
  //
  //   kUninitialized
  //         | Initialize()
  //         |
  //         V
  //    kInitializing
  //         | Decoders initialized
  //         |
  //         V            Decoders reset
  //      kFlushed <------------------ kFlushing
  //         | StartPlayingFrom()         ^
  //         |                            |
  //         |                            | Flush()
  //         `---------> kPlaying --------'
  enum State {
    kUninitialized,
    kInitializing,
    kFlushing,
    kFlushed,
    kPlaying,
    kStopped,
  };

  // Callback from the audio decoder delivering decoded audio samples.
  void DecodedAudioReady(AudioBufferStream::Status status,
                         const scoped_refptr<AudioBuffer>& buffer);

  // Handles buffers that come out of |splicer_|.
  // Returns true if more buffers are needed.
  bool HandleSplicerBuffer_Locked(const scoped_refptr<AudioBuffer>& buffer);

  // Helper functions for AudioDecoder::Status values passed to
  // DecodedAudioReady().
  void HandleAbortedReadOrDecodeError(bool is_decode_error);

  void StartRendering_Locked();
  void StopRendering_Locked();

  // AudioRendererSink::RenderCallback implementation.
  //
  // NOTE: These are called on the audio callback thread!
  //
  // Render() fills the given buffer with audio data by delegating to its
  // |algorithm_|. Render() also takes care of updating the clock.
  // Returns the number of frames copied into |audio_bus|, which may be less
  // than or equal to the initial number of frames in |audio_bus|
  //
  // If this method returns fewer frames than the initial number of frames in
  // |audio_bus|, it could be a sign that the pipeline is stalled or unable to
  // stream the data fast enough.  In such scenarios, the callee should zero out
  // unused portions of their buffer to play back silence.
  //
  // Render() updates the pipeline's playback timestamp. If Render() is
  // not called at the same rate as audio samples are played, then the reported
  // timestamp in the pipeline will be ahead of the actual audio playback. In
  // this case |audio_delay_milliseconds| should be used to indicate when in the
  // future should the filled buffer be played.
  virtual int Render(AudioBus* audio_bus,
                     int audio_delay_milliseconds) OVERRIDE;
  virtual void OnRenderError() OVERRIDE;

  // Helper methods that schedule an asynchronous read from the decoder as long
  // as there isn't a pending read.
  //
  // Must be called on |task_runner_|.
  void AttemptRead();
  void AttemptRead_Locked();
  bool CanRead_Locked();
  void ChangeState_Locked(State new_state);

  // Returns true if the data in the buffer is all before |start_timestamp_|.
  // This can only return true while in the kPlaying state.
  bool IsBeforeStartTime(const scoped_refptr<AudioBuffer>& buffer);

  // Called upon AudioBufferStream initialization, or failure thereof (indicated
  // by the value of |success|).
  void OnAudioBufferStreamInitialized(bool succes);

  // Used to initiate the flush operation once all pending reads have
  // completed.
  void DoFlush_Locked();

  // Calls |decoder_|.Reset() and arranges for ResetDecoderDone() to get
  // called when the reset completes.
  void ResetDecoder();

  // Called when the |decoder_|.Reset() has completed.
  void ResetDecoderDone();

  // Called by the AudioBufferStream when a splice buffer is demuxed.
  void OnNewSpliceBuffer(base::TimeDelta);

  // Called by the AudioBufferStream when a config change occurs.
  void OnConfigChange();

  // Updates |buffering_state_| and fires |buffering_state_cb_|.
  void SetBufferingState_Locked(BufferingState buffering_state);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  scoped_ptr<AudioSplicer> splicer_;
  scoped_ptr<AudioBufferConverter> buffer_converter_;

  // Whether or not we expect to handle config changes.
  bool expecting_config_changes_;

  // The sink (destination) for rendered audio. |sink_| must only be accessed
  // on |task_runner_|. |sink_| must never be called under |lock_| or else we
  // may deadlock between |task_runner_| and the audio callback thread.
  scoped_refptr<media::AudioRendererSink> sink_;

  scoped_ptr<AudioBufferStream> audio_buffer_stream_;

  // Interface to the hardware audio params.
  const AudioHardwareConfig* const hardware_config_;

  // Cached copy of hardware params from |hardware_config_|.
  AudioParameters audio_parameters_;

  // Callbacks provided during Initialize().
  PipelineStatusCB init_cb_;
  TimeCB time_cb_;
  BufferingStateCB buffering_state_cb_;
  base::Closure ended_cb_;
  PipelineStatusCB error_cb_;

  // Callback provided to Flush().
  base::Closure flush_cb_;

  // After Initialize() has completed, all variables below must be accessed
  // under |lock_|. ------------------------------------------------------------
  base::Lock lock_;

  // Algorithm for scaling audio.
  scoped_ptr<AudioRendererAlgorithm> algorithm_;

  // Simple state tracking variable.
  State state_;

  BufferingState buffering_state_;

  // Keep track of whether or not the sink is playing and whether we should be
  // rendering.
  bool rendering_;
  bool sink_playing_;

  // Keep track of our outstanding read to |decoder_|.
  bool pending_read_;

  // Keeps track of whether we received and rendered the end of stream buffer.
  bool received_end_of_stream_;
  bool rendered_end_of_stream_;

  scoped_ptr<AudioClock> audio_clock_;

  base::TimeDelta start_timestamp_;

  // End variables which must be accessed under |lock_|. ----------------------

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<AudioRendererImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioRendererImpl);
};

}  // namespace media

#endif  // MEDIA_FILTERS_AUDIO_RENDERER_IMPL_H_
