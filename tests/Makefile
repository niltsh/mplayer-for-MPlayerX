include ../config.mak

BROKEN_SAMPLES= \
    h264-conformance/CABA3_TOSHIBA_E.264 \
    h264-conformance/CAPA1_TOSHIBA_B.264 \
    h264-conformance/CI1_FT_B.264 \
    h264-conformance/FM1_FT_E.264 \
    lmlm4/LMLM4_CIFat30fps.divx \
    pva/PVA_test-partial.pva \

AUDIO_ONLY_SAMPLES = \
    aac/% ac3/% amrnb/% amrwb/% atrac1/% atrac3/% bink/binkaudio% \
    creative/% dts/% duck/%-audio-only.avi eac3/% gsm/% imc/% \
    lossless-audio/% mp3-conformance/% musepack/% nellymoser/% \
    qt-surge-suite/% real/ra% sipr/% truespeech/% vorbis/% \
    vqf/% w64/% wmapro/% wmavoice/% \

ALLSAMPLES_FULLPATH=$(wildcard $(FATE_SAMPLES)/*/*.*)
ALLSAMPLES=$(patsubst $(FATE_SAMPLES)/%,%,$(ALLSAMPLES_FULLPATH))
SAMPLES:=$(filter-out $(BROKEN_SAMPLES),$(ALLSAMPLES))
SAMPLES:=$(filter-out $(AUDIO_ONLY_SAMPLES),$(SAMPLES))
RESULTS=$(patsubst %,res/%.md5,$(SAMPLES))

fatetest: $(RESULTS)

res/%.md5: ../mplayer$(EXESUF) $(FATE_SAMPLES)/%
	@./faterun.sh $*

clean:
	rm -rf res

.PHONY: fatetest clean
