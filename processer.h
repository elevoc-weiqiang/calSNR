﻿#ifndef CPROCESSER_H
#define CPROCESSER_H

#include "speex_resampler.h"

#include <iostream>
#include <QObject>
struct ProcessRes
{
    double _dInRes = 0.0;
    double _dOutRes = 0.0;
};

class CProcesser:public QObject
{
    Q_OBJECT
public:
    CProcesser(int channels,const std::string& strIn, const std::string& strOut,const std::string& strDataLogPath);
    ~CProcesser();

    bool Init();

    bool Start();
private:
    void xcorr(float* r, float* x, float* y, int N);
    int calc_delay(const char* aec_far_name, const char* aec_near_name, int *DTime);
    bool calc_snr(FILE* pIFile,FILE* pOFile,double duration);

    void resamplerxx(SpeexResamplerState* resampler, int from_samplerate, int to_samplerate, float* inputp, float* outp, unsigned int inLen, unsigned int& outLen);

    bool findLabel(const std::string& inPath,long long &L1, long long &L2);
    bool validate(FILE* pIFile, long long currPos, float rVal);
private:
    int m_iDelay = 0;
    bool m_bInit = false;

    SpeexResamplerState* m_resampler_IC = nullptr;
    SpeexResamplerState* m_resampler_IR = nullptr;
    SpeexResamplerState* m_resampler_OR = nullptr;

    std::string m_strIn;
    std::string m_strOut;
    std::string m_strLogPath = "";
    int m_channels;
    ProcessRes m_CalRes;

    float m_noise_val = 0;
private slots:
    void Slot_finished_cal();
signals:

    void signal_finished_init(bool bSucessed);

    void signal_finished_cal(const double& inRes,const double&outRes);
};

#endif // CPROCESSER_H
