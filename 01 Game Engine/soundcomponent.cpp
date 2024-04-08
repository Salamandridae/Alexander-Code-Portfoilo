#include "soundcomponent.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"


//Taken from https://youtu.be/WvND0djMcfE and https://github.com/mattstone22133/OpenAL_TestProject
//OpenAL error checking
#define OpenAL_ErrorCheck(message)\
{\
    ALenum error = alGetError();\
    if(error != AL_NO_ERROR)\
    {\
        std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl;\
    }\
}

#define alec(FUNCTION_CALL)\
FUNCTION_CALL;\
OpenAL_ErrorCheck(FUNCTION_CALL)

//Stereo constructor, no position data
SoundComponent::SoundComponent(const char* soundfile)
{
    findDevice();
    setListener(0.f, 0.f, 0.f);
    setupStereo(soundfile);
}

//Mono constructor, with position data
SoundComponent::SoundComponent(const char* soundfile, QVector3D pos, QVector3D src)
{
    findDevice();
    setListener(pos.x(), pos.y(), pos.z());
    setupMono(soundfile, src.x(), src.y(), src.z());
}

SoundComponent::~SoundComponent()
{
    alec(alDeleteSources(1, &mMonoSource));
    alec(alDeleteSources(1, &mStereoSource));
    alec(alDeleteBuffers(1, &mMonoSoundBuffer));
    alec(alDeleteBuffers(1, &mStereoSoundBuffer));
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(mContext);
    alcCloseDevice(mDevice);
}

void SoundComponent::findDevice()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Find the default audio device
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ALchar* defaultDeviceString = alcGetString(/*device*/nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    mDevice = alcOpenDevice(defaultDeviceString);
    if(!mDevice)
    {
        std::cerr << "failed to get the default device for OpenAL" << std::endl;
    }
    std::cout << "OpenAL device: " << alcGetString(mDevice, ALC_DEVICE_SPECIFIER) << std::endl;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create an OpenAL media context from the device
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mContext = alcCreateContext(mDevice, /*attrlist*/ nullptr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Activate this context so that OpenAL state notifications are applied to the context
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(!alcMakeContextCurrent(mContext))
    {
        std::cerr << "failed to make the OpenAL context the current context" << std::endl;
        return;
    }
}

void SoundComponent::setListener(ALfloat posx, ALfloat posy, ALfloat posz)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a listener in 3d space (player/camera); (there always exists a listener, you just configure data on it)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    alec(alListener3f(AL_POSITION, posx, posy, posz));
    alec(alListener3f(AL_VELOCITY, soundVelocity.x(), soundVelocity.y(), soundVelocity.z()));
    alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));
}

void SoundComponent::setupMono(const char* soundfile, ALfloat srcx, ALfloat srcy, ALfloat srcz)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // load a mono file into a buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ReadWavData monoData;
    {
        drwav_int16* pSampleData = drwav_open_file_and_read_pcm_frames_s16(soundfile, &monoData.channels, &monoData.sampleRate, &monoData.totalPCMFrameCount, nullptr);
        if (pSampleData == NULL) {
            std::cerr << "failed to load audio file" << std::endl;
            drwav_free(pSampleData, nullptr); //todo use raii to clean this up
            return;
        }
        if (monoData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max()))
        {
            std::cerr << "too much data in file for 32bit addressed vector" << std::endl;
            drwav_free(pSampleData, nullptr);
            return;
        }
        monoData.pcmData.resize(size_t(monoData.getTotalSamples()));
        std::memcpy(monoData.pcmData.data(), pSampleData, monoData.pcmData.size() * /*twobytes_in_s16*/2);
        drwav_free(pSampleData, nullptr);
    }

    alec(alGenBuffers(1, &mMonoSoundBuffer));
    alec(alBufferData(mMonoSoundBuffer, monoData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, monoData.pcmData.data(), monoData.pcmData.size() * 2 /*two bytes per sample*/, monoData.sampleRate));

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // create a sound source that plays our mono sound (from the sound buffer)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    alec(alGenSources(1, &mMonoSource));
    alec(alSource3f(mMonoSource, AL_POSITION, srcx, srcy, srcz)); //Sound origin
    alec(alSource3f(mMonoSource, AL_VELOCITY, soundVelocity.x(), soundVelocity.y(), soundVelocity.z()));
    alec(alSourcef(mMonoSource, AL_PITCH, 1.f));
    alec(alSourcef(mMonoSource, AL_GAIN, 1.f));
    alec(alSourcei(mMonoSource, AL_LOOPING, AL_FALSE));
    alec(alSourcei(mMonoSource, AL_BUFFER, mMonoSoundBuffer));
}

void SoundComponent::setupStereo(const char* soundfile)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // load a stereo file into a buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ReadWavData stereoData;
    {
        drwav_int16* pSampleData = drwav_open_file_and_read_pcm_frames_s16(soundfile, &stereoData.channels, &stereoData.sampleRate, &stereoData.totalPCMFrameCount, nullptr);
        if (pSampleData == NULL) {
            std::cerr << "failed to load audio file" << std::endl;
            return;
        }
        if (stereoData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max()))
        {
            std::cerr << "too much data in file for 32bit addressed vector" << std::endl;
            return;
        }
        stereoData.pcmData.resize(size_t(stereoData.getTotalSamples()));
        std::memcpy(stereoData.pcmData.data(), pSampleData, stereoData.pcmData.size() * /*twobytes_in_s15*/2);
        drwav_free(pSampleData, nullptr);
    }

    alec(alGenBuffers(1, &mStereoSoundBuffer));
    alec(alBufferData(mStereoSoundBuffer, stereoData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, stereoData.pcmData.data(), stereoData.pcmData.size() * 2 /*two bytes per sample*/, stereoData.sampleRate));

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // create a sound source for our stereo sound; note 3d positioning doesn't work with stereo files because
    // stereo files are typically used for music. stereo files come out of both ears so it is hard to know
    // what the sound should be doing based on 3d position data.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    alec(alGenSources(1, &mStereoSource));
    //alec(alSource3f(mStereoSource, AL_POSITION, 0.f, 0.f, 1.f)); //NOTE: this does not work like mono sound positions!
    //alec(alSource3f(mStereoSource, AL_VELOCITY, 0.f, 0.f, 0.f));
    alec(alSourcef(mStereoSource, AL_PITCH, 1.f));
    alec(alSourcef(mStereoSource, AL_GAIN, 1.f));
    alec(alSourcei(mStereoSource, AL_LOOPING, AL_FALSE));
    alec(alSourcei(mStereoSource, AL_BUFFER, mStereoSoundBuffer));
}

void SoundComponent::playMono()
{
    ALint sourceState;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // play the mono sound source
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    alec(alSourcePlay(mMonoSource));
    alec(alGetSourcei(mMonoSource, AL_SOURCE_STATE, &sourceState));
}

void SoundComponent::playStereo()
{
    ALint sourceState;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // play the stereo sound source
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    alec(alSourcePlay(mStereoSource));
    alec(alGetSourcei(mStereoSource, AL_SOURCE_STATE, &sourceState));
}
