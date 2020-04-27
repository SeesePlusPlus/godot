#ifndef KALDI_H
#define KALDI_H

#include "feat/wave-reader.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "nnet3/nnet-utils.h"
#include "online2/online-endpoint.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-timing.h"
#include "online2/onlinebin-util.h"
#include "util/kaldi-thread.h"

using namespace kaldi;
using namespace fst;

class Kaldi {
  typedef kaldi::int32 int32;
  typedef kaldi::int64 int64;

  std::string word_syms_rxfilename; // Symbol table for words [for debug output]

  // feature_opts includes configuration for the iVector adaptation,
  // as well as the basic features.
  OnlineNnet2FeaturePipelineConfig feature_opts;
  nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
  LatticeFasterDecoderConfig decoder_opts;
  OnlineEndpointConfig endpoint_opts;

  BaseFloat chunk_length_secs; // Length of chunk size in seconds, that we process
  bool do_endpointing;
  bool online;

  // sequential args from 1 and forward
  std::string nnet3_rxfilename;
  std::string fst_rxfilename;
  std::string spk2utt_rspecifier;
  std::string wav_rspecifier;
  std::string clat_wspecifier;

  fst::SymbolTable * word_syms;
  OnlineNnet2FeaturePipelineInfo * feature_info;
  Matrix<double> global_cmvn_stats;
  TransitionModel * trans_model;
  nnet3::DecodableNnetSimpleLoopedInfo * decodable_info;
  nnet3::AmNnetSimple * am_nnet;
  SequentialTokenVectorReader * spk2utt_reader;
  CompactLatticeWriter * clat_writer;

  fst::Fst<fst::StdArc> * decode_fst;

  public:
  Kaldi();

  void initialize();
  int decode();
};

#endif
