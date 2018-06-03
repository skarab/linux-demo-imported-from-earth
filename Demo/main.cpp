#include <stdlib.h>
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "../Data/data.h"
#include "filter_highpass.h"
#include "filter_lowpass.h"


const uint8_t* earth[] = {
        earth_1,
        earth_2,
        earth_3,
        earth_4,
        earth_5,
        earth_6,
        earth_7,
        earth_8,
        earth_9,
        earth_10,
        earth_11,
        earth_12,
        earth_13,
        earth_14,
        earth_15,
        earth_16,
        earth_17,
        earth_18,
        earth_19,
        earth_20
    };

const uint8_t* video[] = {
        video_1,
        video_2,
        video_3,
        video_4,
        video_5,
        video_6,
        video_7,
        video_8,
        video_9,
        video_10,
        video_11,
        video_12,
        video_13,
        video_14,
        video_15,
        video_16,
        video_17,
        video_18,
        video_19,
        video_20,
        video_21,
        video_22,
        video_23,
        video_24
        };

char* file_content;
bool LoadWAV(const char* path, unsigned int* length, short int** data)
{
    FILE* file = fopen(path, "rb");
    if (file==NULL)
        return false;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    file_content = (char*)malloc(file_size);
    if (file_content==NULL)
    {
        fclose(file);
        return false;
    }

    long result = fread(file_content, 1, file_size, file);
    fclose(file);
    if (result!=file_size || file_size<44)
        return false;

    unsigned int s = ((int*)file_content)[1];
    if ((memcmp(file_content, "RIFF", 4)!=0)
        || (s+8!=file_size)
        || (memcmp(file_content+8, "WAVE", 4)!=0)
        || (memcmp(file_content+12, "fmt ", 4)!=0))
    {
        printf("wav is corrupted\n");
        return false;
    }

    unsigned int format_size = *(int*)(file_content+16);
    short format = *(short*)(file_content+20);
    short channel = *(short*)(file_content+22);
    unsigned int sample_rate = *(int*)(file_content+24);
    unsigned int k = *(int*)(file_content+28);
    short byte_size = *(short*)(file_content+34);
    if ((format_size!=16)
        || (format!=1)              // raw
        || (channel!=2)             // stereo
        || (sample_rate!=44100)     // 44100Hz
        || (k!=44100*16*2/8)
        || (byte_size!=16))         // 16bits
    {
        printf("wav is unsupported\n");
        return false;
    }

    unsigned int sample_size = *(unsigned int*)(file_content+40);
    if ((memcmp(file_content+36, "data", 4)!=0)
        || (sample_size+44>file_size))
    {
        printf("wav is corrupted\n");
        return false;
    }

    *data = (short int*)(file_content+44);
    *length = sample_size/4;

    return length>0;
}

double GetTickCount(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}

int main (int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)<0)
    {
        printf("%s\n", SDL_GetError() );
        return -1;
    }
    atexit(SDL_Quit);

    SDL_Surface* screen = SDL_SetVideoMode(400, 300, 24, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN);
    if (!screen)
    {
        printf("%s\n", SDL_GetError());
        return -1;
    }
    SDL_WM_SetCaption("Demo", NULL);
    SDL_ShowCursor(0);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024)==-1)
    {
        printf("%s\n", Mix_GetError());
        return -1;
    }

    unsigned int wav_length;
    short int* wav_data;
    if (!LoadWAV("imported_from_earth.wav", &wav_length, &wav_data))
    {
        printf("Failed to load WAV\n");
        return -1;
    }

    FILTER_HIGHPASS highpass_filter;
    FILTER_HIGHPASS_Initialize(highpass_filter);

    FILTER_LOWPASS lowpass_filter;
    FILTER_LOWPASS_Initialize(lowpass_filter);

    Mix_Music* music = Mix_LoadMUS("imported_from_earth.wav");

    double start_time = GetTickCount();

    if ((music==NULL) || (Mix_PlayMusic(music, 0)==-1))
    {
        printf("%s\n", Mix_GetError());
        return -1;
    }

    float volume = 0.0f;
    float volume2 = 0.0f;
    bool quit = false;

    while (Mix_PlayingMusic())
    {
        double time = GetTickCount()-start_time;

        SDL_Event event;
        if (SDL_PollEvent(&event) && event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE)
        {
            quit = true;
            break;
        }

        SDL_LockSurface(screen);

        unsigned int wav_position = (unsigned int)(time*2.0*44100.0/1000.0);
        if (wav_position>=wav_length*2) wav_position = wav_length*2-1;

        int s = wav_data[wav_position];

        int highpass = s;
        FILTER_HIGHPASS_Compute(highpass, highpass_filter);
        if (highpass<0) highpass = -highpass;
        highpass = highpass*255/32767;
        highpass *= 10;
        if (highpass>255) highpass = 255;

        int lowpass = s;
        FILTER_LOWPASS_Compute(lowpass, lowpass_filter);
        if (lowpass<0) lowpass = -lowpass;
        lowpass = lowpass*255/32767;
        lowpass *= 100;
        if (lowpass>255) lowpass = 255;

        if (s<0) s = -s;
        s = s*4*255/32767;

        volume = volume*0.9f+s*0.1f;
        volume2 = volume2*0.5f+s*0.5f;
        s = (unsigned int)volume;

        if (s==0) s = 1;

        const uint8_t* highpass_bitmap = earth[((int)(time/100.0))%20];
        const uint8_t* lowpass_bitmap = video[((int)(time/200.0))%24];

        int size = 20;
        if (lowpass%10>0) size = 40;

        int mode = 0;
        if (lowpass>200) mode = (mode+1)%2;
        if (highpass>200) mode = (mode+1)%2;

        uint8_t* framebuffer = (uint8_t*)screen->pixels;
        uint32_t noise[size];
        for (int i=0 ; i<size ; ++i)
            noise[i] = rand()%s;
        int k = 0;
        for (int i=0 ; i<screen->w*screen->h ; ++i)
        {
            int hp_r = *highpass_bitmap++ * highpass/255;
            int hp_g = *highpass_bitmap++ * highpass/255;
            int hp_b = *highpass_bitmap++ * highpass/255;

            int lp_r = 255- *lowpass_bitmap++ * lowpass/255;
            int lp_g = 255- *lowpass_bitmap++ * lowpass/255;
            int lp_b = 255- *lowpass_bitmap++ * lowpass/255;

            uint32_t c = rand()%s;

            if (mode==0)
            {
                framebuffer[i*3+0] = noise[(k-(size-1)+s)%size]+hp_r+lp_r;
                framebuffer[i*3+1] = c+hp_g+lp_g;
                framebuffer[i*3+2] = noise[(k-(size-11)+(int)volume2/2)%size]+hp_b+lp_b;
            }
            else
            {
                framebuffer[i*3+0] = c+hp_r+lp_r;
                framebuffer[i*3+1] = noise[(k-(size-11)+(int)volume2/2)%size]+hp_g+lp_g;
                framebuffer[i*3+2] = noise[(k-(size-1)+s)%size]+hp_b*lp_b;
            }
            noise[k++] = c;
            if (k==size) k = 0;
        }

        SDL_UnlockSurface(screen);
        SDL_Flip(screen);
    }

    if (!quit)
    {
        start_time = GetTickCount();

        while (1)
        {
            double time = GetTickCount()-start_time;
            if (time>10000.0)
                break;

            SDL_LockSurface(screen);
            uint8_t* framebuffer = (uint8_t*)screen->pixels;
            memcpy(framebuffer, credits, 400*300*3);
            SDL_UnlockSurface(screen);
            SDL_Flip(screen);
        }
    }

    Mix_HaltMusic();
    Mix_FreeMusic(music);
    Mix_CloseAudio();



    return 0;
}
