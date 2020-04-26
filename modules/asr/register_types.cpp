/* register_types.cpp */

#include "register_types.h"

#include "asr.h"
#include "core/class_db.h"

void register_asr_types() {
  ClassDB::register_class<ASR>();
}

void unregister_asr_types() {
  // Nothing to do here in this example.
}
