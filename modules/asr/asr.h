/* asr.h */

#ifndef ASR_H
#define ASR_H

#include "core/reference.h"
#include "servers/audio_server.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/effects/audio_effect_record.h"
#include "kaldi.h"

class ASR : public Reference {
  GDCLASS(ASR, Reference);

  static ASR *singleton;

  Ref<AudioEffectRecord> audioBusEffect;
  Ref<AudioStreamSample> recording;

  Kaldi * kaldi;

  protected:
  static void _bind_methods();

  public:
  ASR();

  static ASR *get_singleton();

  void initialize(StringName busName);
  void start();
  void stop();
};

#endif // ASR_H
