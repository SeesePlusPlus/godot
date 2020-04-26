/* asr.cpp */

#include "asr.h"

ASR::ASR() {
}

ASR * ASR::get_singleton() {
  if (!singleton) {
    singleton = new ASR();
  }

  return singleton;
}

void ASR::initialize(StringName busName) {
  auto audioServer = AudioServer::get_singleton();
  auto idx = audioServer->get_bus_index(busName);
  this->audioBusEffect = static_cast<Ref<AudioEffectRecord>>(audioServer->get_bus_effect(idx, 0));

  // initialize kaldi
}

void ASR::start() {
  if (this->audioBusEffect->is_recording_active()) {
    // we're already recording, skip this
    return;
  }

  this->audioBusEffect->set_recording_active(true);
}

void ASR::stop() {
  if (!this->audioBusEffect->is_recording_active()) {
    // we're already not recording, skip this
    return;
  }

  auto recording = this->audioBusEffect->get_recording();
  this->audioBusEffect->set_recording_active(false);

  auto data = recording->get_data();

  // feed data to kaldi
  for (int i = 0; i < data.size(); i++) {
    auto chunk = data.get(i);
  }
  // get/return response from kaldi
}

void ASR::_bind_methods() {
  ClassDB::bind_method(D_METHOD("initialize", "busName"), &ASR::initialize);
  ClassDB::bind_method(D_METHOD("start"), &ASR::start);
  ClassDB::bind_method(D_METHOD("stop"), &ASR::stop);
}
