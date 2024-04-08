#ifndef SOUNDCOMPONENT_H
#define SOUNDCOMPONENT_H
#include <iostream>
#include "AL/al.h"
#include "AL/alc.h"
#include "dr_wav.h"
#include <vector>
#include <QVector3D>

class SoundComponent
{
public:
    SoundComponent(const char* soundfile);
    SoundComponent(const char* soundfile, QVector3D position, QVector3D soundSource);
    ~SoundComponent();
    void findDevice();
    void setListener(ALfloat posx, ALfloat posy, ALfloat posz);
    void setupMono(const char* soundfile, ALfloat srcx, ALfloat srcy, ALfloat srcz);
    void setupStereo(const char* soundfile);
    void playMono();
    void playStereo();
private:
    ALCdevice* mDevice;
    ALCcontext* mContext;
    const ALfloat forwardAndUpVectors[6] =
    {
       0.f, 1.f, 0.f,
       0.f, 0.f, 1.f
    };
    struct ReadWavData
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drwav_uint64 totalPCMFrameCount = 0;
        std::vector<uint16_t> pcmData;
        drwav_uint64 getTotalSamples() { return totalPCMFrameCount * channels; }
    };
    ALuint mMonoSoundBuffer;
    ALuint mStereoSoundBuffer;
    ALuint mMonoSource;
    ALuint mStereoSource;
    QVector3D soundVelocity {0.f, 0.f, 0.f};
};

#endif // SOUNDCOMPONENT_H
