#ifndef KALDI_H
#define KALDI_H

#include <kaldi/feat/wave-reader.h>
#include <kaldi/fstext/fstext-lib.h>
#include <kaldi/lat/lattice-functions.h>
#include <kaldi/nnet3/nnet-utils.h>
#include <kaldi/online2/online-endpoint.h>
#include <kaldi/online2/online-nnet2-feature-pipeline.h>
#include <kaldi/online2/online-nnet3-decoding.h>
#include <kaldi/online2/online-timing.h>
#include <kaldi/online2/onlinebin-util.h>
#include <kaldi/util/kaldi-thread.h>

class Kaldi {
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
  std::string clat_wspecifier;

  OnlineNnet2FeaturePipelineInfo * feature_info;
  TransitionModel * trans_model;
  nnet3::AmNnetSimple * am_nnet;

  public:
  Kaldi();

  void initialize();
};

#endif
