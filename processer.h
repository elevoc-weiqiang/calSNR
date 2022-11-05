#ifndef CPROCESSER_H
#define CPROCESSER_H

#include "speex_resampler.h"

#include <iostream>
class CProcesser
{
public:
    CProcesser();
    ~CProcesser();

    bool Init();
    bool Start();
private:
    void xcorr(float* r, float* x, float* y, int N);
    int calc_delay(const char* aec_far_name, const char* aec_near_name, int *DTime);

    long long calc_begin(FILE* pFile,long long currPos,long long tLe);
    bool calc_snr(FILE* pIFile,FILE* pOFile,double duration);

    void resamplerxx(SpeexResamplerState* resampler, int from_samplerate, int to_samplerate, float* inputp, float* outp, unsigned int inLen, unsigned int& outLen);
private:
    int m_iDelay = 0;
    bool m_bInit = false;

    SpeexResamplerState* m_resampler_IC = nullptr;
    SpeexResamplerState* m_resampler_IR = nullptr;
    SpeexResamplerState* m_resampler_OR = nullptr;

};

#endif // CPROCESSER_H
