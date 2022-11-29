#ifndef CPROCESSER_H
#define CPROCESSER_H

#include "speex_resampler.h"

#include <iostream>
#include <QObject>
const double BLANK_1_LENGTH = 8.950;
const double BLANK_2_LENGTH = 11.820;

//const double BLANK_1_LENGTH = 0.0;
//const double BLANK_2_LENGTH = 0.0;

struct WavPCMFileHeader_44
{
    struct RIFF {
        const    char rift[4] = { 'R','I', 'F', 'F' };
        uint32_t fileLength;
        const    char wave[4] = { 'W','A', 'V', 'E' };
    }riff;
    struct Format
    {
        const    char fmt[4] = { 'f','m', 't', ' ' };
        uint32_t blockSize = 16;
        uint16_t formatTag;
        uint16_t channels;
        uint32_t samplesPerSec;
        uint32_t avgBytesPerSec;
        uint16_t blockAlign;
        uint16_t  bitsPerSample;
    }format;
    struct  Data
    {
        const    char data[4] = { 'd','a', 't', 'a' };
        uint32_t dataLength;
    }data;
    WavPCMFileHeader_44() {}
    WavPCMFileHeader_44(int nCh, int  nSampleRate, int  bitsPerSample, int dataSize) {
        riff.fileLength = 36 + dataSize;
        format.formatTag = 3;
        format.channels = nCh;
        format.samplesPerSec = nSampleRate;
        format.avgBytesPerSec = nSampleRate * nCh * bitsPerSample / 8;
        format.blockAlign = nCh * bitsPerSample / 8;
        format.bitsPerSample = bitsPerSample;
        data.dataLength = dataSize;
    }
};


struct WavPCMFileHeader_46
{
    struct RIFF {
        const    char rift[4] = { 'R','I', 'F', 'F' };
        uint32_t fileLength;
        const    char wave[4] = { 'W','A', 'V', 'E' };
    }riff;
    struct Format
    {
        const    char fmt[4] = { 'f','m', 't', ' ' };
        uint32_t blockSize = 18;
        uint16_t formatTag;
        uint16_t channels;
        uint32_t samplesPerSec;
        uint32_t avgBytesPerSec;
        uint16_t blockAlign;
        uint16_t  bitsPerSample;
        uint16_t  blockData;
    }format;
    struct  Data
    {
        const    char data[4] = { 'd','a', 't', 'a' };
        uint32_t dataLength;
    }data;
    WavPCMFileHeader_46() {}
    WavPCMFileHeader_46(int nCh, int  nSampleRate, int  bitsPerSample, int dataSize) {
        riff.fileLength = 36 + dataSize;
        format.formatTag = 3;
        format.channels = nCh;
        format.samplesPerSec = nSampleRate;
        format.avgBytesPerSec = nSampleRate * nCh * bitsPerSample / 8;
        format.blockAlign = nCh * bitsPerSample / 8;
        format.bitsPerSample = bitsPerSample;
        data.dataLength = dataSize;
    }
};

struct ProcessRes
{
    double _dInRes = 0.0;
    double _dOutRes = 0.0;
};

class CProcesser:public QObject
{
    Q_OBJECT
public:
    CProcesser(int channels,const std::string& strIn, const std::string& strOut,const std::string& strDataLogPath,const double dblankHead = 0.0,const double dblankTail = 0.0);
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

    bool GetWavFileInf(const std::string& strPath,unsigned int& sampleRate,unsigned& channels,unsigned int&FileSize,unsigned int& iWaveHead);
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
    double m_dblankHead = BLANK_1_LENGTH;
    double m_dblankTail = BLANK_2_LENGTH;
    float m_noise_val = 0;
    bool m_isCalStatus =  false;
    unsigned int m_iInAudioFileSize;
    unsigned int m_iOutAudioFileSize;
    bool m_bWavFileIn = false;
    bool m_bWavFileOut = false;
    unsigned int m_iInWaveHead = 0;//wav文件的数据头时否为46个字节
    unsigned int m_iOutWaveHead = 0;
private slots:
    void Slot_finished_cal();

signals:

    void signal_finished_init(bool bSucessed);

    void signal_finished_cal(const double& inRes,const double&outRes);
};

#endif // CPROCESSER_H
