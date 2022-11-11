#include "processer.h"
#include "Log/LogHelper.h"
#include <fstream>
#include <QString>
#include <QDateTime>
#include <QDir>
#include<QDebug>

#define MORDERN  "PROCESS"

#define INPUT_FILE    "C:\\Users\\weiqi\\Desktop\\test\\hecheng\\sample1_in.pcm"
#define OUTPUT_FILE    "C:\\Users\\weiqi\\Desktop\\test\\hecheng\\sample1_out.pcm"

const double DURATION = 98.86;   //秒
const double BEGIN_EMPTY_DURATION = 8.9; //音频起始有8.9s 的空白
const double END_EMPTY_DURATION = 11.14; //音频尾部有11.14s 的空白

const int SAMPLERATE = 48000;
const int CHANNEL = 2;
const int CELL = 960;
const double THRESHOLD = 0.5;
const double PULSE_TIME = 0.5;
const double BLANK_1_LENGTH = 8.950;
const double BLANK_2_LENGTH = 11.820;
const double AUIDO_BLOCK = 78.040;


#define ERR   0

#if !(CELL%960)
#define IGNORE
#endif

#define ELEVOC_EXE_COMPRESS

QString createMultipleFolders(const QString path)
{
    QDir dir(path);
    if (dir.exists(path)) {
        return path;
    }

    QString parentDir = createMultipleFolders(path.mid(0, path.lastIndexOf('/')));
    QString dirName = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirName.isEmpty())
    {
        parentPath.mkpath(dirName);
    }

    return parentDir + "/" + dirName;
}

static std::string CreateDir(const QString& strDefaultPath = "")
{
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString date =current_date_time.toString("yyyyMMdd-hhmmsszzz");
    QString strTmp = "";

    if(strDefaultPath.isEmpty())
    {
        strTmp = "C:/EleDector/CalSnrRes/data/" + date;
        createMultipleFolders(strTmp);
    }
    else
    {
        strTmp = strDefaultPath + "/data/" + date;
        createMultipleFolders(strTmp);
    }

    return strTmp.toStdString();
}

static double Square(float buf[], const int len)
{
    //LOG_DEBUG(MORDERN,"Square+++,len = [%d]",len);
    double sum = 0;
    for(int i = 0;i<len;i+=2)
    {
        double e = buf[i];
        sum += e*e;
    }
    //LOG_DEBUG(MORDERN,"Square---,sum = [%.4f]",sum);
    return sum;
}

static double Diff_Square(float bufL[], float bufR[], const int len)
{
    //LOG_DEBUG(MORDERN,"Diff_Square+++");
    double sum = 0;
    for(int i = 0;i<len;i+=2)
    {
        double diff = bufL[i] - bufR[i];
        sum += diff*diff;
    }
    //LOG_DEBUG(MORDERN,"Diff_Square---,[%.4f]",sum);
    return sum;
}

CProcesser::CProcesser(int channels,const std::string& strIn, const std::string& strOut,const std::string& strDataLogPath)
{
    m_strLogPath = strDataLogPath;
    m_channels = channels;
    m_strIn = strIn;
    m_strOut = strOut;
    int err = 0;
    m_resampler_IC = elevoc_resampler_init(m_channels, 48000, 16000, 10, &err);
    m_resampler_IR = elevoc_resampler_init(m_channels, 48000, 16000, 10, &err);
    m_resampler_OR = elevoc_resampler_init(m_channels, 48000, 16000, 10, &err);
}

CProcesser::~CProcesser()
{
    if(m_resampler_IC != nullptr)
    {
        elevoc_resampler_destroy(m_resampler_IC);
    }
    if(m_resampler_IR != nullptr)
    {
        elevoc_resampler_destroy(m_resampler_IR);
    }

    if(m_resampler_OR != nullptr)
    {
        elevoc_resampler_destroy(m_resampler_OR);
    }
}

std::vector<float> CProcesser::CalPulseValue(const std::string& inPath,const std::string& outPath)
{
    std::vector<float> vreRet;
    std::string far_path = inPath;
    std::string near_path = outPath;
    // read far filec
    FILE* fp_far = fopen(far_path.c_str(), "rb");
    size_t farFile_size = 0;
    if (fp_far != NULL) {
        if (!fseek(fp_far, 0, SEEK_END)) {
            farFile_size = ftell(fp_far);
            fseek(fp_far, 0, SEEK_SET);
        }
    }
    else {
        //std::cout << "CalPulseValue calc_delay far file open error" << std::endl;
        LOG_DEBUG(MORDERN,"open file failed, %s",inPath.c_str());
        return std::vector<float>();
    }
    //read near file
    FILE* fp_near = fopen(near_path.c_str(), "rb");
    size_t nearFile_size = 0;
    if (fp_near != NULL) {
        if (!fseek(fp_near, 0, SEEK_END)) {
            nearFile_size = ftell(fp_near);
            fseek(fp_near, 0, SEEK_SET);
        }
    }
    else {
        LOG_DEBUG(MORDERN,"CalPulseValue open file failed, %s",outPath.c_str());
        //std::cout << "calc_delay near file open error" << std::endl;
        fclose(fp_far);
        return std::vector<float>();;
    }
    size_t fileSize = std::min(farFile_size, nearFile_size);

    float iVal = 0.0;
    while(true)
    {
        float *iBuf = new float[480*m_channels];
        fread(iBuf, sizeof(float), 480*m_channels, fp_far);
        for(int i = 0; i< 480*m_channels; i+= m_channels)
        {
            if(iVal < iBuf[i])
            {
                iVal = iBuf[i];
            }
        }

        fileSize -= 480*m_channels * sizeof(float);
        delete []iBuf;
        if(fileSize <= 480*m_channels * sizeof(float))
        {
            break;
        }
    }

    m_fPluseVal = iVal;
    LOG_DEBUG(MORDERN,"CalPulseValue Max Val = %f",iVal);
    qDebug()<<"CalPulseValue Max Val = "<<iVal;
    vreRet.push_back(iVal);
    fclose(fp_near);
    fclose(fp_far);
    return vreRet;
}

bool CProcesser::Init()
{
    CalPulseValue(m_strIn.c_str(), /*OUTPUT_FILE*/m_strOut.c_str());
    LOG_DEBUG(MORDERN,"Init begin");
    if(calc_delay(/*INPUT_FILE*/m_strIn.c_str(), /*OUTPUT_FILE*/m_strOut.c_str(), &m_iDelay) < 0)
    {
        emit signal_finished_init(false);
        return false;
    }
    m_iDelay /= m_channels;
    m_iDelay -= ERR;

    LOG_DEBUG(MORDERN,"calculate delay: %d ms",m_iDelay);
    m_bInit = true;
    LOG_DEBUG(MORDERN,"Init end");
    emit signal_finished_init(true);
    return true;
}

long long CProcesser::calc_begin(FILE* pFile,long long currPos,long long tLen)
{
    long long beg = -1;
    float *iBuf = new float[480*m_channels];
    //float iBuf[480*m_channels];
   // float iFrame[480];
    long long Len = tLen;

    long long count = 0;
    long long idx = 0;
    while(Len > 480*m_channels * sizeof(float))
    {
        fread(iBuf, sizeof(float), 480*m_channels, pFile);
        Len -= 480*m_channels * sizeof(float);

        for(int i = 0; i< 480*m_channels; i+= m_channels)
        {
            if(count > 1000 &&(iBuf[i] > THRESHOLD||m_fPluseVal == iBuf[i]))//||(iBuf[i]> 0&&abs(m_fPluseVal - iBuf[i]) < 0.05 )
            {
                LOG_DEBUG(MORDERN,"Cal begin = iBuf[%d] = %.3f",i,iBuf[i]);
                LOG_DEBUG(MORDERN,"Cal begin m_fPluseVal = %.3f",m_fPluseVal);
                beg = count*480*m_channels + i + 48000*m_channels * PULSE_TIME + currPos;
                LOG_DEBUG(MORDERN,"find begin pos, count = %d, begin index = %ud, time = %.2f s",count, beg, double(beg)/(48000*m_channels));
                break;
            }
        }

        if(beg>0)
        {
            break;
        }

        count++;
        //LOG_DEBUG(MORDERN,"count = [%d] ", count);
    }

    delete []iBuf;
    return beg;
}

bool CProcesser::calc_snr(FILE* pIFile,FILE* pOFile,double duration)
{
    LOG_DEBUG(MORDERN,"calc_snr...");
    float *iBuf_clean = new float[480*m_channels];
    float *iBuf_clean_temp = new float[480*m_channels];
    float *iBuf_reverb = new float[480*m_channels];
    float *iBuf_reverb_temp = new float[480*m_channels];
    float *oBuf_reverb = new float[480*m_channels];
    float *oBuf_reverb_temp = new float[480*m_channels];
    long long sample = 48000*m_channels * (duration - BLANK_1_LENGTH - BLANK_2_LENGTH);
    unsigned int resample_outLen = 0;

    std::string path = CreateDir(QString::fromStdString(m_strLogPath));
    LOG_INFO("CProcesser","Save LogPath =%s",path.c_str());
    std::ofstream stream_IClean;
    std::ofstream stream_IReverb;
    std::ofstream stream_OReverb;
    std::ofstream data_log;

    std::string iClean_file = path + "/iClean.pcm";
    std::string iReverb_file = path + "/iReverb.pcm";
    std::string oReverb_file = path + "/oReverb.pcm";

    stream_IClean.open(iClean_file, std::ios::out | std::ios::binary);
    stream_IReverb.open(iReverb_file,std::ios::out | std::ios::binary);
    stream_OReverb.open(oReverb_file,std::ios::out | std::ios::binary);

    bool exit = true;
    double sum_iClean = 0;
    double sum_iReverb = 0;
    double sum_oReverb = 0;

    if (0 != fseek(pOFile, (duration + 1.0 + double(m_iDelay)/1000)*48000*m_channels*sizeof(float), SEEK_CUR))
    {
        exit = false;
        goto Exit;
    }

    LOG_DEBUG(MORDERN,"calc_snr begin...");
/*

    inputAudio32_voip.pcm  |-+-----------------+-----------------|
                              Iclean             Irever
    outputAudio32_voip.pcm |-+-----------------+-----------------|
                                                 ORever
*/

    int cnt = 0;
    while(sample >= 480*m_channels)
    {
        cnt++;
        fread(iBuf_clean_temp, sizeof(float), 480*m_channels, pIFile);
        //resample_outLen = CELL;
        resample_outLen = 160;
        resamplerxx(m_resampler_IC,SAMPLERATE, 16000, iBuf_clean_temp, iBuf_clean, 480, resample_outLen);
        if(stream_IClean.is_open() /*&& cnt*10 > 8471 && cnt*10 < 86586*/)
        {
            stream_IClean.write((char*)iBuf_clean,resample_outLen*m_channels*sizeof(float));
            //stream_IClean.write((char*)iBuf_clean_temp,480*m_channels*sizeof(float));
        }

        if (0 != fseek(pIFile, ((duration + 1.0)*48000*m_channels - 480*m_channels)*sizeof(float), SEEK_CUR))
        {
            exit = false;
            goto Exit;
        }

        fread(iBuf_reverb_temp, sizeof(float), 480*m_channels, pIFile);
        //resample_outLen = CELL;
        resample_outLen = 160;
        resamplerxx(m_resampler_IR,SAMPLERATE, 16000, iBuf_reverb_temp, iBuf_reverb, 480, resample_outLen);
        if (0 != fseek(pIFile, - (duration + 1.0)*48000*m_channels*sizeof(float), SEEK_CUR))
        {
            exit = false;
            goto Exit;
        }

        if(stream_IReverb.is_open()/*&& cnt*10 > 8471 && cnt*10 < 86586*/)
        {
            stream_IReverb.write((char*)iBuf_reverb,resample_outLen*m_channels*sizeof(float));
            //stream_IReverb.write((char*)iBuf_reverb_temp,480*m_channels*sizeof(float));
        }

        fread(oBuf_reverb_temp, sizeof(float), 480*m_channels, pOFile);
        //resample_outLen = CELL;
        resample_outLen = 160;
        resamplerxx(m_resampler_OR, SAMPLERATE, 16000, oBuf_reverb_temp, oBuf_reverb, 480, resample_outLen);
        if(stream_OReverb.is_open()/*&& cnt*10 > 8471 && cnt*10 < 86586*/)
        {
            stream_OReverb.write((char*)oBuf_reverb,resample_outLen*m_channels*sizeof(float));
            //stream_OReverb.write((char*)oBuf_reverb_temp,480*m_channels*sizeof(float));
        }

        sample -= 480*m_channels;

        //if(cnt*10 > 8471 && cnt*10 < 86586)
        {
            sum_iClean += Square(iBuf_clean,resample_outLen*m_channels);
            sum_iReverb += Diff_Square(iBuf_reverb,iBuf_clean,resample_outLen*m_channels);
            sum_oReverb += Diff_Square(oBuf_reverb,iBuf_clean,resample_outLen*m_channels);

            //sum_iClean += Square(iBuf_clean_temp,480*m_channels);
            //sum_iReverb += Diff_Square(iBuf_reverb_temp,iBuf_clean_temp,480*m_channels);
            //sum_oReverb += Diff_Square(oBuf_reverb_temp,iBuf_clean_temp,480*m_channels);
        }
    }



#ifndef IGNORE
    if(sample > 0)
    {
        fread(iBuf_clean_temp, sizeof(float), sample, pIFile);
        resample_outLen= (sample * 16000) / (SAMPLERATE*m_channels);
        //resample_outLen= sample / m_channels;
        resamplerxx(SAMPLERATE, 16000, iBuf_clean_temp, iBuf_clean, sample/2, resample_outLen);
        if(stream_IClean.is_open())
        {
            stream_IClean.write((char*)iBuf_clean,resample_outLen*sizeof(float));
        }

        if (0 != fseek(pIFile, (duration*96000 - sample)*sizeof(float), SEEK_CUR))
        {
            exit = false;
            goto Exit;
        }

        fread(iBuf_reverb_temp, sizeof(float), sample, pIFile);
        resample_outLen= (sample * 16000) / (SAMPLERATE*m_channels);
        //resample_outLen= sample / m_channels;
        resamplerxx(SAMPLERATE, 16000, iBuf_reverb_temp, iBuf_reverb, sample/2, resample_outLen);
        if(stream_IReverb.is_open())
        {
            stream_IReverb.write((char*)iBuf_reverb,resample_outLen*sizeof(float));
        }

        if (0 != fseek(pIFile, (-duration*96000)*sizeof(float), SEEK_CUR))
        {
            goto Exit;
            exit = false;
        }

        fread(oBuf_reverb, sizeof(float), sample, pOFile);
        if(stream_OReverb.is_open())
        {
            stream_OReverb.write((char*)oBuf_reverb,sample*sizeof(float));
        }

        sum_iClean += Square(iBuf_clean,sample);
        sum_iReverb += Diff_Square(iBuf_reverb,iBuf_clean,sample);
        sum_oReverb += Diff_Square(oBuf_reverb,iBuf_clean,sample);
    }
#endif

    /*if (0 != fseek(pIFile, (duration*48000*m_channels)*sizeof(float), SEEK_CUR))
    {
        exit = false;
    }*/

Exit:
    delete []iBuf_clean;
    delete []iBuf_clean_temp;
    delete []iBuf_reverb;
    delete []iBuf_reverb_temp;
    delete []oBuf_reverb;
    delete []oBuf_reverb_temp;
    if(stream_IClean.is_open())
    {
        stream_IClean.close();
    }
    if(stream_IReverb.is_open())
    {
        stream_IReverb.close();
    }
    if(stream_OReverb.is_open())
    {
        stream_OReverb.close();
    }


    if(exit && sum_iReverb != 0 /*&& sum_oReverb != 0*/)
    {
        double Ir = sum_iClean / sum_iReverb;
        double Or = sum_iClean / sum_oReverb;
        double iSNR = 10*log10(Ir);
        double OSNR = 10*log10(Or);
        m_CalRes._dInRes = iSNR;
        m_CalRes._dOutRes = OSNR;
        //将结果写入文件保存
        QString msg;
//        msg.sprintf("iSNR = [%f]dB",iSNR);
        msg.sprintf("iSNR = [%f]dB, oSNR = [%f]dB, THRESHOLD = %.2f",iSNR, OSNR,THRESHOLD);
        LOG_DEBUG(MORDERN,"%s",msg.toStdString().c_str());
        data_log.open(path+"/data_log.log",std::ios::out | std::ios::binary | std::ios::app);
        if(data_log.is_open())
        {
            data_log.write(msg.toStdString().c_str(),msg.length());
            data_log.close();
        }
    }

    return exit;
}

void CProcesser::Slot_finished_cal()
{

}

bool CProcesser::Start()
{
    if(!m_bInit)
    {
        return false;
    }
    long long iFile_size = 0;
    long long oFile_size = 0;

    bool exit = true;

    FILE* pInput = fopen(/*INPUT_FILE*/m_strIn.c_str(),"rb");
    FILE* pOutput = fopen(/*OUTPUT_FILE*/m_strOut.c_str(),"rb");

    if(!pInput || !pOutput)
    {
        LOG_DEBUG(MORDERN,"Start Open pcm file failed");
        exit = false;
        goto Exit;
    }

    if (!fseek(pInput, 0, SEEK_END))
    {
        iFile_size = ftell(pInput);
        fseek(pInput, 0, SEEK_SET);
    }

    if (!fseek(pOutput, 0, SEEK_END))
    {
        oFile_size = ftell(pOutput);
        fseek(pOutput, 0, SEEK_SET);
    }

    long long iBeginFlag1 = 0;
    long long iBeginFlag2 = 0;
    long long curPos = 0;
    LOG_DEBUG(MORDERN,"iFile_size = %d",iFile_size);
    auto iFile_size_c = iFile_size;
    //while(true)
    {
        iBeginFlag1 = calc_begin(pInput,curPos,iFile_size_c);
        iBeginFlag2 = calc_begin(pInput,iBeginFlag1,iFile_size_c - iBeginFlag1);
        if(iBeginFlag1 < 0)
        {
            LOG_DEBUG(MORDERN,"iBeginFlag1 < 0, %d", iBeginFlag1);
            goto Exit;
        }
        else
        {
            if (0 != fseek(pInput, (iBeginFlag1 + BLANK_1_LENGTH*48000*m_channels) *sizeof(float), SEEK_SET))
            {
                LOG_DEBUG(MORDERN,"input file fseek failed");
                goto Exit;
            }

            if (0 != fseek(pOutput, (iBeginFlag1 + BLANK_1_LENGTH*48000*m_channels)*sizeof(float), SEEK_SET))
            {
                LOG_DEBUG(MORDERN,"output file fseek failed");
                goto Exit;
            }

            if(iBeginFlag2 < 0)//未找到第二个脉冲点则根据平移98.86s的数据来计算
            {
                if((iFile_size - iBeginFlag1*sizeof(float)) < DURATION * 2 * 48000*m_channels*sizeof(float))  //确保有两段音频的数据量
                {
                    LOG_ERROR(MORDERN,"too little data to split");
                }

                //切数据、处理
                if(!calc_snr(pInput,pOutput,DURATION))
                {
                    goto Exit;
                }
            }
            else
            {
                double DurationTmp = (double)(iBeginFlag2 - iBeginFlag1 )/(48000*m_channels) - 1.5;
                LOG_DEBUG(MORDERN,"DurationTmp %.3f",DurationTmp);
                //切数据、处理
                if(!calc_snr(pInput,pOutput,DurationTmp))
                {
                    goto Exit;
                }
            }
        }


#if 0
        if(iBeginFlag2 < 0)
        {
            LOG_DEBUG(MORDERN,"iBeginFlag2 < 0, %d", iBeginFlag2);
            goto Exit;
        }

        if (0 != fseek(pInput, iBeginFlag1*sizeof(float), SEEK_SET))
        {
            LOG_DEBUG(MORDERN,"input file fseek failed");
            goto Exit;
        }

        if (0 != fseek(pOutput, iBeginFlag1*sizeof(float), SEEK_SET))
        {
            LOG_DEBUG(MORDERN,"output file fseek failed");
            goto Exit;
        }

        //iFile_size -= iBegin*sizeof(float);
        if((iFile_size - iBeginFlag1*sizeof(float)) < DURATION * 2 * 48000*m_channels*sizeof(float))  //确保有两段音频的数据量
        {
            LOG_ERROR(MORDERN,"too little data to split");
        }

        //切数据、处理
        if(!calc_snr(pInput,pOutput,DURATION))
        {
            goto Exit;
        }

        //iFile_size_c -= DURATION * 2 * 48000*m_channels;
        //iBegin += DURATION * 2 * 48000*m_channels;
#endif
    }

Exit:
    fclose(pInput);
    fclose(pOutput);
    emit signal_finished_cal(m_CalRes._dInRes,m_CalRes._dOutRes);
    return exit;
}

void CProcesser::xcorr(float* r, float* x, float* y, int N) {
    float sxy = 0;
    int delay, i, j;
    for (delay = -N + 1; delay < N; delay++) {
        sxy = 0;
        for (i = 0; i < N; i++) {
            j = i + delay;
            if ((j < 0) || (j >= N))
                continue;
            else
                sxy += (x[j] * y[i]);
        }
        r[delay + N - 1] = sxy;
    }
}

int CProcesser::calc_delay(const char* aec_far_name, const char* aec_near_name,int *DTime) {
    int delay = 0;
    std::string far_path = aec_far_name;
    std::string near_path = aec_near_name;
    // read far filec
    FILE* fp_far = fopen(far_path.c_str(), "rb");
    size_t farFile_size = 0;
    if (fp_far != NULL) {
        if (!fseek(fp_far, 0, SEEK_END)) {
            farFile_size = ftell(fp_far);
            fseek(fp_far, 0, SEEK_SET);
        }
    }
    else {
        //std::cout << "calc_delay far file open error" << std::endl;
        LOG_DEBUG(MORDERN,"open file failed, %s",aec_far_name);
        return -1;
    }
    //read near file
    FILE* fp_near = fopen(near_path.c_str(), "rb");
    size_t nearFile_size = 0;
    if (fp_near != NULL) {
        if (!fseek(fp_near, 0, SEEK_END)) {
            nearFile_size = ftell(fp_near);
            fseek(fp_near, 0, SEEK_SET);
        }
    }
    else {
        LOG_DEBUG(MORDERN,"open file failed, %s",aec_near_name);
        //std::cout << "calc_delay near file open error" << std::endl;
        fclose(fp_far);
        return -1;
    }
    size_t fileSize = std::min(farFile_size, nearFile_size);
    //printf("calcdelay read file success : %d\n", fileSize);

    size_t once_read = 480*4;
    size_t left_size = fileSize;
    float readFarBuffer[480];
    float readNearBuffer[480];
    float output_linear[480];
    float output_nlp[480];
    float sample_far = 0;
    int frame_num = -1;
    int delay_length = 48000;
    int delay_num = delay_length / 480;
    float far_sig[48000];
    float near_sig[48000];
    int count = 0;
    while (true) {
        count++;
        if (frame_num >= delay_num || left_size < once_read) {
            break;
        }
        fread(readFarBuffer, sizeof(float), 480, fp_far);
        fread(readNearBuffer, sizeof(float), 480, fp_near);
        float vad = 0;
        for (int i = 0; i < 480; i++)
        {
            sample_far = readFarBuffer[i];
            vad += abs(sample_far);
        }
        if (vad > 10 && (count * 5 > 20000)) {
            LOG_DEBUG(MORDERN,"delay calc start count = %d ms",count*5);
            std::cout << "delay calc start count = " << count << std::endl;
            frame_num = 0;
        }
        if (frame_num != -1) {
            memcpy(far_sig + frame_num * 480, readFarBuffer, sizeof(float) * 480);
            memcpy(near_sig + frame_num * 480, readNearBuffer, sizeof(float) * 480);
            frame_num++;
        }
        left_size -= once_read;
    }

    float r[96000 - 1];
    xcorr(r, far_sig, near_sig, delay_length);
    float maxV = 0;
    int maxId = 0;
    int index_i = 0;
    for (int i = -delay_length + 1; i < delay_length; ++i) {
        index_i = i + delay_length - 1;
        if (r[index_i] > maxV) {
            maxV = r[index_i];
            maxId = i;
        }
    }
    float res = ((float)maxId / (float)delay_length) * 1000;
    delay = res < 0 ? -(int)(res - 0.5) : -(int)(res + 0.5);
    fclose(fp_far);
    fclose(fp_near);

    *DTime = delay;

    return 0;
}

void CProcesser::resamplerxx(SpeexResamplerState* resampler,int from_samplerate, int to_samplerate, float* inputp, float* outp, unsigned int inLen, unsigned int& outLen)
{
    //LOG_DEBUG(MORDERN,"resamplerxx before from_samplerate = [%d], to_samplerate = [%d], inLen = [%ud], outLen = [%ud]",from_samplerate, to_samplerate,inLen, outLen);
    if (resampler == nullptr)
    {
        int err = 0;
        resampler = elevoc_resampler_init(m_channels, from_samplerate, to_samplerate, 10, &err);
    }
    //elevoc_resampler_set_rate(resampler, from_samplerate, to_samplerate);
    elevoc_resampler_process_interleaved_float(resampler, inputp, &inLen, outp, &outLen);
}
