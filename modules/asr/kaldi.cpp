#include "kaldi.h"

Kaldi::Kaldi() {
  this->word_syms_rxfilename = "/opt/rocklen/results/graph/words.txt";

  this->nnet3_rxfilename = "exp/tdnn_7b_chain_online/final.mdl";
  this->fst_rxfilename = "/opt/rocklen/results/graph/HCLG.fst";
  this->spk2utt_rspecifier = "ark:echo utterance-id1 utterance-id1|";
  this->wav_rspecifier = "scp:echo utterance-id1 wingardium-leviosa.wav";
  this->clat_wspecifier = "ark:/dev/null";
  
  this->chunk_length_secs = 0.18;
  this->do_endpointing = false;
  this->online = false;
}

void Kaldi::initialize() {
  this->feature_info = new OnlineNnet2FeaturePipelineInfo(this->feature_opts);
  if (!this->online) {
    this->feature_info->ivector_extractor_info.use_most_recent_ivector = true;
    this->feature_info->ivector_extractor_info.greedy_ivector_extractor = true;
    this->chunk_length_secs = -1.0;
  }

  if (this->feature_info->global_cmvn_stats_rxfilename != "")
    ReadKaldiObject(this->feature_info->global_cmvn_stats_rxfilename,
                  &this->global_cmvn_stats);

  this->trans_model = new TransitionModel();
  this->am_nnet = new nnet3::AmNnetSimple();
  {
    bool binary;
    Input ki(this->nnet3_rxfilename, &binary);
    this->trans_model->Read(ki.Stream(), binary);
    this->am_nnet->Read(ki.Stream(), binary);
    SetBatchnormTestMode(true, &(this->am_nnet->GetNnet()));
    SetDropoutTestMode(true, &(this->am_nnet->GetNnet()));
    nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(this->am_nnet->GetNnet()));
  }

  // this object contains precomputed stuff that is used by all decodable
  // objects. It takes a pointer to am_nnet because if it has iVectors it has
  // to modify the nnet to accept iVectors at intervals.
  this->decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(
    this->decodable_opts,
    this->am_nnet
  );

  this->decode_fst = ReadFstKaldiGeneric(this->fst_rxfilename);

  this->word_syms = NULL;
  if (this->word_syms_rxfilename != "")
    if (!(this->word_syms = fst::SymbolTable::ReadText(this->word_syms_rxfilename)))
      KALDI_ERR << "Could not read symbol table from file "
                << this->word_syms_rxfilename;

  this->spk2utt_reader = new SequentialTokenVectorReader(this->spk2utt_rspecifier);
  this->clat_writer = new CompactLatticeWriter(this->clat_wspecifier);
}

int Kaldi::decode() {
  try {
    /* ------------ Config / Setup ------------ */

    int32 num_done = 0, num_err = 0;
    double tot_like = 0.0;
    int64 num_frames = 0;

    RandomAccessTableReader<WaveHolder> wav_reader(this->wav_rspecifier);

    OnlineTimingStats timing_stats;

    for (; !this->spk2utt_reader->Done(); this->spk2utt_reader->Next()) {
      std::string spk = this->spk2utt_reader->Key();
      const std::vector<std::string> &uttlist = this->spk2utt_reader->Value();

      OnlineIvectorExtractorAdaptationState adaptation_state(
          this->feature_info->ivector_extractor_info);
      OnlineCmvnState cmvn_state(global_cmvn_stats);

      for (size_t i = 0; i < uttlist.size(); i++) {
        std::string utt = uttlist[i];
        if (!wav_reader.HasKey(utt)) {
          KALDI_WARN << "Did not find audio for utterance " << utt;
          num_err++;
          continue;
        }
        const WaveData &wave_data = wav_reader.Value(utt);
        // get the data for channel zero (if the signal is not mono, we only
        // take the first channel).
        SubVector<BaseFloat> data(wave_data.Data(), 0);

        OnlineNnet2FeaturePipeline feature_pipeline(*this->feature_info);
        feature_pipeline.SetAdaptationState(adaptation_state);
        feature_pipeline.SetCmvnState(cmvn_state);

        OnlineSilenceWeighting silence_weighting(
            *this->trans_model,
            this->feature_info->silence_weighting_config,
            this->decodable_opts.frame_subsampling_factor);

        SingleUtteranceNnet3Decoder decoder(this->decoder_opts, *this->trans_model,
                                            *this->decodable_info,
                                            *this->decode_fst, &feature_pipeline);
        OnlineTimer decoding_timer(utt);

        BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length;
        if (chunk_length_secs > 0) {
          chunk_length = int32(samp_freq * chunk_length_secs);
          if (chunk_length == 0) chunk_length = 1;
        } else {
          chunk_length = std::numeric_limits<int32>::max();
        }

        int32 samp_offset = 0;
        std::vector<std::pair<int32, BaseFloat> > delta_weights;

        while (samp_offset < data.Dim()) {
          int32 samp_remaining = data.Dim() - samp_offset;
          int32 num_samp = chunk_length < samp_remaining ? chunk_length
                                                         : samp_remaining;

          SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
          feature_pipeline.AcceptWaveform(samp_freq, wave_part);

          samp_offset += num_samp;
          decoding_timer.WaitUntil(samp_offset / samp_freq);
          if (samp_offset == data.Dim()) {
            // no more input. flush out last frames
            feature_pipeline.InputFinished();
          }

          if (silence_weighting.Active() &&
              feature_pipeline.IvectorFeature() != NULL) {
            silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
            silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),
                                              &delta_weights);
            feature_pipeline.IvectorFeature()->UpdateFrameWeights(delta_weights);
          }

          decoder.AdvanceDecoding();

          if (this->do_endpointing && decoder.EndpointDetected(this->endpoint_opts)) {
            break;
          }
        }
        decoder.FinalizeDecoding();

        CompactLattice clat;
        bool end_of_utterance = true;
        decoder.GetLattice(end_of_utterance, &clat);

        decoding_timer.OutputStats(&timing_stats);

        // In an application you might avoid updating the adaptation state if
        // you felt the utterance had low confidence.  See lat/confidence.h
        feature_pipeline.GetAdaptationState(&adaptation_state);
        feature_pipeline.GetCmvnState(&cmvn_state);

        // we want to output the lattice with un-scaled acoustics.
        BaseFloat inv_acoustic_scale =
            1.0 / decodable_opts.acoustic_scale;
        ScaleLattice(AcousticLatticeScale(inv_acoustic_scale), &clat);

        this->clat_writer->Write(utt, clat);
        KALDI_LOG << "Decoded utterance " << utt;
        num_done++;
      }
    }
    timing_stats.Print(online);

    KALDI_LOG << "Decoded " << num_done << " utterances, "
              << num_err << " with errors.";
    KALDI_LOG << "Overall likelihood per frame was " << (tot_like / num_frames)
              << " per frame over " << num_frames << " frames.";
    delete this->decode_fst;
    delete this->word_syms; // will delete if non-NULL.
    return (num_done != 0 ? 0 : 1);
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
}
