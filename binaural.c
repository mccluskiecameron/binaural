// binaural.c
// A simple demo to demonstrate the effect of placing an audio source at different
// positions relative to the listener.
// takes a wav file as input and plays it. An appropriate file can usually be generated thus:
//     ffmpeg -i [some audio file] output.wav
// when the program is running, give it different value of l (distance away of
// audio source) and θ (angle away from straight ahead) to hear the effect.

// code adapted from https://gist.github.com/armornick/3447121

#include <SDL.h>

void audio_callback(void *userdata, Uint8 *stream, int len);

// byte data remaining to be played
static uint8_t *audio_pos;
// bytes left in audio_pos
static uint32_t audio_len;

// described below
static int delay = 0;
static float balance = 1;

int main(int argc, char* argv[]){

    if(argc < 2){
        fprintf(stderr, "usage: %s wav_file\n", argv[0]);
        return 1;
    }

    if(SDL_Init(SDL_INIT_AUDIO) < 0){
        fprintf(stderr, "couldn't init sdl\n");
        return 1;
    }

    // total length of data
    static uint32_t wav_length;
    // byte data of audio
    static uint8_t *wav_buffer;
    // format of audio
    static SDL_AudioSpec wav_spec;

    if(SDL_LoadWAV(argv[1], &wav_spec, &wav_buffer, &wav_length) == NULL){
        fprintf(stderr, "invalid or missing file %s\n", argv[1]);
        return 1;
    }

    if (wav_spec.channels != 2 || wav_spec.format != (SDL_AUDIO_MASK_SIGNED | (SDL_AUDIO_MASK_BITSIZE & 16))){
        fprintf(stderr, "expected signed 16 bit 2 channel little endian audio\n");
        fprintf(stderr, "got: %d ch, %d bit, %s endian \n",
                wav_spec.channels,
                SDL_AUDIO_BITSIZE(wav_spec.format),
                SDL_AUDIO_ISBIGENDIAN(wav_spec.format)?"big":"little"
        );

        return 1;
    }

    audio_pos = wav_buffer;
    audio_len = wav_length;

    wav_spec.callback = audio_callback;
    wav_spec.userdata = NULL;

    if ( SDL_OpenAudio(&wav_spec, NULL) < 0 ){
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        exit(-1);
    }

    // play
    SDL_PauseAudio(0);

    // distance sound source is away from center of head
    float l = 10.;
    // angle sound source is away from straight ahead (degrees)
    float t = 10.;

    while ( audio_len > 0 ) {

        // speed of sound in air. commonly given as 300m/s + air temp in Celcius
        float v = 320;
        // distance between ears estimated as 20cm
        float d = .20;
        // angle away from straight ahead of sound origin, here computed in rad
        float θ = t / 180 * M_PI;
        // sides of the right angled triangle formed by the line extending
        // straight ahead (y) and the sound origin
        float y = l * cosf(θ);
        // shortest distance from the line pointing straight ahead and the sound origin
        float x = l * sinf(θ);
        // shortest distance from the line emerging straight ahead from either ear and the sound origin
        float x_r = fabsf(x - d/2);
        float x_l = fabsf(x + d/2);
        // distance from either ear and the sound origin
        float er = sqrtf(x_r * x_r + y * y);
        float el = sqrtf(x_l * x_l + y * y);

        // how many samples the sound is delayed from the right to the left ear
        delay = ((el - er) / v) * wav_spec.freq;
        // how much the sound is attenuated from the right to the left ear
        // computed as sqrt(er^2 / el^2), ie amplitude is the square root of power
        balance = er / el;

        printf("delay: %d; balance: %f\n", delay, balance);
        printf("l; θ:\n");

        if(scanf("%f %f", &l, &t) != 2)
            break;
    }

    SDL_CloseAudio();
    SDL_FreeWAV(wav_buffer);

}

void audio_callback(void *userdata, Uint8 *stream, int len) {

    if(audio_len <= 0)
        return;

    len = ( len > audio_len ? audio_len : len );

    int samples = len / 4;

    for(int i = 0; i < samples + abs(delay); i++){
        float merged;
        if(i*4 > audio_len)
            merged = 0;
        else
            merged = ((int16_t*)audio_pos)[i*2]/2 + ((int16_t*)audio_pos)[i*2 + 1]/2;
        float lsamp, rsamp;
        if(balance <= 1){
            rsamp = merged;
            lsamp = merged * balance;
        }else{
            lsamp = merged;
            rsamp = merged / balance;
        }
        if(delay >= 0){
            int ri = i - delay;
            if(i < samples)
                ((int16_t*)stream)[i*2] = lsamp;
            if(ri >= 0)
                ((int16_t*)stream)[ri*2+1] = rsamp;
        }else{
            int li = i + delay;
            if(i < samples)
                ((int16_t*)stream)[i*2+1] = rsamp;
            if(li >= 0)
                ((int16_t*)stream)[li*2] = lsamp;
        }

    }

    audio_pos += len;
    audio_len -= len;
}
